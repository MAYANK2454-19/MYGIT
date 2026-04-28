/*
 * ============================================
 *          MYGIT - Diff Command
 *          "mygit diff <filename>"
 * ============================================
 *
 * PURPOSE:
 *   Show line-by-line differences between:
 *   → The COMMITTED version of a file
 *   → The CURRENT version on disk
 *
 * DATA STRUCTURES USED:
 *   1. 2D Arrays  → Store lines of each file version
 *   2. Integers   → Track line counts and positions
 *   3. Strings    → Individual lines for comparison
 *
 * ALGORITHM: Naive line-by-line diff
 *   Compare each line position
 *   Mark changed/added/removed lines
 *
 * OUTPUT FORMAT (like real git diff):
 *   - removed line    ← shown in RED
 *   + added line      ← shown in GREEN
 *     unchanged line  ← shown normally
 */

#include "mygit.h"


/*
 * FUNCTION: split_into_lines
 * ───────────────────────────
 * Takes a big string of text and splits it
 * into an array of individual lines.
 *
 * INPUT:  "Hello\nWorld\nBye\n"
 * OUTPUT: lines[0] = "Hello"
 *         lines[1] = "World"
 *         lines[2] = "Bye"
 *         returns 3 (number of lines)
 *
 * PARAMETERS:
 *   content    → The big string to split
 *   lines      → 2D array to store lines in
 *   max_lines  → Maximum number of lines
 *   max_len    → Maximum length of each line
 *
 * RETURNS:
 *   Number of lines found
 *
 * HOW IT WORKS:
 *   We walk through the content character by character.
 *   When we find '\n', that's the end of a line.
 *   We copy everything before '\n' into lines[].
 *   Repeat until end of content.
 */
int split_into_lines(const char* content,
                     char lines[][MAX_LINE],
                     int max_lines,
                     int max_len) {

    int line_count = 0;   /* How many lines we've found */
    int char_pos   = 0;   /* Position within current line */

    /*
     * Initialize all lines to empty strings
     * Good practice: always initialize arrays!
     */
    for (int i = 0; i < max_lines; i++) {
        lines[i][0] = '\0';
    }

    /*
     * Walk through content character by character
     *
     * i is our position in the content string
     * content[i] is the current character
     *
     * We stop when:
     *   content[i] == '\0'  (end of string)
     *   line_count >= max_lines (too many lines)
     */
    int i = 0;
    while (content[i] != '\0' && line_count < max_lines) {

        if (content[i] == '\n') {
            /*
             * Found end of a line!
             *
             * Terminate the current line with '\0'
             * Then move to the next line slot.
             *
             * Example:
             *   We've been collecting "Hello"
             *   We hit '\n'
             *   lines[0] = "Hello\0"  ← complete!
             *   line_count becomes 1
             *   char_pos resets to 0 for next line
             */
            lines[line_count][char_pos] = '\0';
            line_count++;
            char_pos = 0;

        } else if (content[i] == '\r') {
            /*
             * Windows uses \r\n for line endings.
             * \r is "carriage return" — we just skip it!
             * The \n after it will trigger the line save.
             */
            /* skip \r — do nothing */

        } else {
            /*
             * Regular character — add to current line
             *
             * But check: is there room?
             * (max_len - 1) to leave space for '\0'
             */
            if (char_pos < max_len - 1) {
                lines[line_count][char_pos] = content[i];
                char_pos++;
            }
        }

        i++;  /* Move to next character */
    }

    /*
     * Handle the LAST LINE if it doesn't end with '\n'
     *
     * Example: "Hello\nWorld"  (no \n at the end!)
     *
     * After the loop:
     *   line_count = 1 (for "Hello")
     *   char_pos = 5   (we collected "World" but didn't save it!)
     *
     * If char_pos > 0, there's an unsaved line fragment.
     * Save it now!
     */
    if (char_pos > 0 && line_count < max_lines) {
        lines[line_count][char_pos] = '\0';
        line_count++;
    }

    return line_count;
}


/*
 * FUNCTION: get_committed_hash_for_file
 * ──────────────────────────────────────
 * Finds the HASH of a file from the last commit.
 *
 * WHY?
 *   We need the hash to find the blob file:
 *   hash = 193485797
 *   blob = .mygit/objects/193485797.blob
 *   That blob is the OLD version we compare against!
 *
 * HOW:
 *   1. Find the latest commit on current branch
 *   2. Read its file list
 *   3. Find our filename in the list
 *   4. Return the matching hash
 *
 * PARAMETERS:
 *   filename → File we're looking for
 *
 * RETURNS:
 *   Hash value (non-zero) if found
 *   0 if file not found in any commit
 */
unsigned long get_committed_hash_for_file(
                  const char* filename) {

    /* Get current branch */
    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    /* Get latest commit ID on this branch */
    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, branch);

    char id_str[20];
    if (read_file(ref_path, id_str, sizeof(id_str)) < 0) {
        return 0;  /* No commits yet */
    }

    int target_id = atoi(id_str);
    if (target_id <= 0) return 0;

    /* Open commits file */
    FILE* fp = fopen(COMMITS_FILE, "r");
    if (!fp) return 0;

    int found      = 0;
    int current_id = -1;
    unsigned long result_hash = 0;

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {

        line[strcspn(line, "\n\r")] = '\0';

        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        /* Detect commit block */
        int rid;
        if (sscanf(line, "COMMIT:%d", &rid) == 1) {
            current_id = rid;
            found = (current_id == target_id);
            continue;
        }

        if (!found) continue;

        /*
         * Parse FILES and HASHES lines together.
         *
         * We need to match FILENAME position with HASH position.
         *
         * FILES:  hello.txt,test.txt,main.c
         * HASHES: 193485797,874291053,123456789
         *
         * If hello.txt is at position 0 in FILES,
         * its hash is at position 0 in HASHES.
         *
         * Strategy:
         *   When we find FILES line → find position of our filename
         *   When we find HASHES line → get hash at that position
         */

        /* Store file list temporarily */
        static char file_list[MAX_LINE];
        static int  file_position = -1;

        if (strncmp(line, "FILES:", 6) == 0) {
            strncpy(file_list, line + 6, MAX_LINE - 1);
            file_list[MAX_LINE - 1] = '\0';

            /*
             * Find position of our filename in the list
             *
             * "hello.txt,test.txt,main.c"
             *  position 0  position 1  position 2
             *
             * We use strtok to split and count
             */
            char copy[MAX_LINE];
            strcpy(copy, file_list);

            char* tok = strtok(copy, ",");
            int pos = 0;
            file_position = -1;

            while (tok != NULL) {
                if (strcmp(tok, filename) == 0) {
                    file_position = pos;  /* Found it! */
                    break;
                }
                pos++;
                tok = strtok(NULL, ",");
            }
        }

        if (strncmp(line, "HASHES:", 7) == 0
            && file_position >= 0) {

            /*
             * Get hash at file_position
             *
             * "193485797,874291053,123456789"
             * position 0: 193485797
             * position 1: 874291053
             * position 2: 123456789
             */
            char copy[MAX_LINE];
            strncpy(copy, line + 7, MAX_LINE - 1);
            copy[MAX_LINE - 1] = '\0';

            char* tok = strtok(copy, ",");
            int pos = 0;

            while (tok != NULL) {
                if (pos == file_position) {
                    result_hash = strtoul(tok, NULL, 10);
                    break;
                }
                pos++;
                tok = strtok(NULL, ",");
            }
        }

        /* End of commit block */
        if (strcmp(line, "END") == 0 && found) {
            break;
        }
    }

    fclose(fp);
    return result_hash;
}


/*
 * FUNCTION: print_diff_header
 * ────────────────────────────
 * Prints the header section of diff output.
 * Shows what we're comparing and a divider.
 */
void print_diff_header(const char* filename,
                       int old_lines,
                       int new_lines) {

    printf("\n");
    printf(CYAN
           "  diff --mygit a/%s b/%s\n"
           RESET, filename, filename);
    printf(CYAN
           "  --- a/%s (committed version, %d lines)\n"
           RESET, filename, old_lines);
    printf(CYAN
           "  +++ b/%s (current version,   %d lines)\n"
           RESET, filename, new_lines);
    printf(CYAN
           "  ════════════════════════════════════════\n"
           RESET);
}


/*
 * FUNCTION: print_diff_stats
 * ───────────────────────────
 * Prints summary at the end:
 *   "3 additions, 2 deletions"
 */
void print_diff_stats(int additions, int deletions,
                      int unchanged) {

    printf(CYAN
           "  ════════════════════════════════════════\n"
           RESET);

    printf("  ");

    if (additions > 0) {
        printf(GREEN "+%d addition%s  " RESET,
               additions,
               additions == 1 ? "" : "s");
    }

    if (deletions > 0) {
        printf(RED "-%d deletion%s  " RESET,
               deletions,
               deletions == 1 ? "" : "s");
    }

    if (unchanged > 0) {
        printf(CYAN "%d unchanged" RESET, unchanged);
    }

    printf("\n\n");
}


/*
 * FUNCTION: perform_diff
 * ──────────────────────
 * THE CORE DIFF ALGORITHM!
 *
 * Compares two arrays of lines and prints differences.
 *
 * ALGORITHM (Naive line-by-line):
 *
 *   For each line position i:
 *
 *   Case 1: Both files have a line at position i
 *     → Compare them
 *     → Same?     → print normally (unchanged)
 *     → Different → print OLD in red (-), NEW in green (+)
 *
 *   Case 2: Only OLD file has a line at position i
 *     → Line was DELETED
 *     → Print in RED (-)
 *
 *   Case 3: Only NEW file has a line at position i
 *     → Line was ADDED
 *     → Print in GREEN (+)
 *
 * PARAMETERS:
 *   old_lines  → Array of lines from committed version
 *   old_count  → Number of lines in old version
 *   new_lines  → Array of lines from current version
 *   new_count  → Number of lines in new version
 */
void perform_diff(char old_lines[][MAX_LINE], int old_count,
                  char new_lines[][MAX_LINE], int new_count) {

    int additions = 0;
    int deletions = 0;
    int unchanged = 0;

    /*
     * We need to go through ALL line positions.
     * The maximum position is whichever file is LONGER.
     *
     * Example:
     *   old has 3 lines, new has 5 lines
     *   We go from position 0 to 4 (max = 5)
     */
    int max_lines = old_count > new_count
                    ? old_count
                    : new_count;

    for (int i = 0; i < max_lines; i++) {

        /*
         * CASE 1: Both files have a line here
         *
         * i < old_count → old file has a line at position i
         * i < new_count → new file has a line at position i
         */
        if (i < old_count && i < new_count) {

            if (strcmp(old_lines[i], new_lines[i]) == 0) {
                /*
                 * Lines are IDENTICAL → unchanged
                 * Print with spaces for alignment
                 * Line number shown for reference
                 */
                printf("  " RESET "%4d  │  %s\n",
                       i + 1,
                       new_lines[i]);
                unchanged++;

            } else {
                /*
                 * Lines are DIFFERENT!
                 *
                 * Show OLD line in RED with -
                 * Show NEW line in GREEN with +
                 *
                 * This is the classic diff format!
                 */
                printf(RED
                       "  %4d  │- %s\n"
                       RESET,
                       i + 1,
                       old_lines[i]);

                printf(GREEN
                       "  %4d  │+ %s\n"
                       RESET,
                       i + 1,
                       new_lines[i]);

                additions++;
                deletions++;
            }
        }

        /*
         * CASE 2: Only OLD file has a line here
         *
         * This means the line was DELETED in the new version!
         *
         * i >= new_count → new file ran out of lines
         * i < old_count  → but old file still has lines here
         */
        else if (i < old_count) {
            /*
             * Line DELETED → show in RED with -
             */
            printf(RED
                   "  %4d  │- %s\n"
                   RESET,
                   i + 1,
                   old_lines[i]);
            deletions++;
        }

        /*
         * CASE 3: Only NEW file has a line here
         *
         * This means a NEW line was ADDED!
         *
         * i >= old_count → old file ran out of lines
         * i < new_count  → but new file still has lines here
         */
        else {
            /*
             * Line ADDED → show in GREEN with +
             */
            printf(GREEN
                   "  %4d  │+ %s\n"
                   RESET,
                   i + 1,
                   new_lines[i]);
            additions++;
        }
    }

    /* Print summary */
    print_diff_stats(additions, deletions, unchanged);
}


/*
 * ═══════════════════════════════════════════════
 * MAIN FUNCTION: mygit_diff
 * ═══════════════════════════════════════════════
 *
 * This runs when user types: mygit diff hello.txt
 *
 * STEPS:
 *   1. Find the committed hash for this file
 *   2. Load the OLD version from blob storage
 *   3. Load the NEW (current) version from disk
 *   4. Split both into lines
 *   5. Run the diff algorithm
 *   6. Display results
 */
int mygit_diff(const char* filename) {

    /*
     * ─────────────────────────────
     * STEP 1: Check file exists
     * ─────────────────────────────
     */
    if (!file_exists(filename)) {
        printf(RED
               "\n  ✗ File not found: '%s'\n\n"
               RESET, filename);
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 2: Find committed hash
     * ─────────────────────────────
     *
     * We need to know WHICH blob to compare against.
     * The blob is named by the hash of the committed content.
     */
    unsigned long committed_hash =
        get_committed_hash_for_file(filename);

    if (committed_hash == 0) {
        /*
         * File has never been committed!
         * Nothing to compare against.
         *
         * Show entire file as "new" (all green)
         */
        printf(YELLOW
               "\n  ⚠ '%s' has no committed version.\n"
               RESET, filename);
        printf("  All content is new:\n\n");

        char content[MAX_CONTENT];
        if (read_file(filename, content, MAX_CONTENT) >= 0) {

            /* Split into lines and show all as additions */
            char new_lines[MAX_LINES][MAX_LINE];
            int new_count = split_into_lines(
                                content,
                                new_lines,
                                MAX_LINES,
                                MAX_LINE);

            for (int i = 0; i < new_count; i++) {
                printf(GREEN
                       "  %4d  │+ %s\n"
                       RESET,
                       i + 1,
                       new_lines[i]);
            }
            printf("\n");
            printf(GREEN
                   "  +%d line%s (all new)\n\n"
                   RESET,
                   new_count,
                   new_count == 1 ? "" : "s");
        }
        return 0;
    }

    /*
     * ─────────────────────────────
     * STEP 3: Load OLD version from blob
     * ─────────────────────────────
     *
     * Build the blob path from the hash:
     *   hash = 193485797
     *   path = .mygit/objects/193485797.blob
     */
    char blob_path[MAX_PATH];
    snprintf(blob_path, sizeof(blob_path),
             "%s/%lu.blob",
             OBJECTS_DIR,
             committed_hash);

    char old_content[MAX_CONTENT];

    if (read_file(blob_path, old_content,
                  MAX_CONTENT) < 0) {
        printf(RED
               "\n  ✗ Could not read committed version.\n\n"
               RESET);
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 4: Load NEW version from disk
     * ─────────────────────────────
     */
    char new_content[MAX_CONTENT];

    if (read_file(filename, new_content,
                  MAX_CONTENT) < 0) {
        printf(RED
               "\n  ✗ Could not read current file.\n\n"
               RESET);
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 5: Quick check — did anything change?
     * ─────────────────────────────
     *
     * Hash both versions and compare.
     * If same hash → files are identical → no diff!
     *
     * This is O(n) but avoids running the full diff
     * algorithm on identical files.
     */
    unsigned long old_hash = hash_content(old_content);
    unsigned long new_hash = hash_content(new_content);

    if (old_hash == new_hash) {
        printf(GREEN
               "\n  ✓ No changes in '%s'\n"
               RESET, filename);
        printf("  File is identical to committed version.\n\n");
        return 0;
    }

    /*
     * ─────────────────────────────
     * STEP 6: Split into lines
     * ─────────────────────────────
     *
     * Convert the big content strings into
     * arrays of individual lines.
     *
     * old_content → old_lines[] array
     * new_content → new_lines[] array
     *
     * We use static here to avoid large stack allocation.
     * These are BIG arrays (MAX_LINES × MAX_LINE bytes)!
     *
     * "static" local variables are stored in a special
     * memory area (not the stack).
     * Stack has limited size (~1-8 MB).
     * Static variables don't use the stack!
     */
    static char old_lines[MAX_LINES][MAX_LINE];
    static char new_lines[MAX_LINES][MAX_LINE];

    int old_count = split_into_lines(
                        old_content,
                        old_lines,
                        MAX_LINES,
                        MAX_LINE);

    int new_count = split_into_lines(
                        new_content,
                        new_lines,
                        MAX_LINES,
                        MAX_LINE);

    /*
     * ─────────────────────────────
     * STEP 7: Print header
     * ─────────────────────────────
     */
    print_diff_header(filename, old_count, new_count);

    /*
     * ─────────────────────────────
     * STEP 8: Run the diff!
     * ─────────────────────────────
     */
    perform_diff(old_lines, old_count,
                 new_lines, new_count);

    return 0;
}