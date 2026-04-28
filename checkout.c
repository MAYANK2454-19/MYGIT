/*
 * ============================================
 *          MYGIT - Checkout Command
 *          "mygit checkout <commit_id>"
 * ============================================
 *
 * PURPOSE:
 *   Restore files from a previous commit.
 *   "Time travel" to any point in history!
 *
 * DATA STRUCTURES USED:
 *   1. Commit struct  → Hold the target commit's data
 *   2. Arrays         → Store file names and hashes
 *   3. File I/O       → Read blobs, write restored files
 *
 * HOW IT WORKS:
 *   1. Find the commit with the given ID
 *   2. For each file in that commit:
 *      a. Find its blob (hash.blob in objects/)
 *      b. Read the blob content
 *      c. Write it back to the working directory
 *   3. Update the branch ref to this commit
 *   4. Tell the user what was restored
 *
 * SAFETY:
 *   → We warn user before overwriting files
 *   → We check blobs exist before starting
 *   → We show exactly what was restored
 */

#include "mygit.h"


/*
 * FUNCTION: find_commit_by_id
 * ────────────────────────────
 * Searches commits.dat for a commit with the given ID.
 * Fills in the provided Commit struct if found.
 *
 * PARAMETERS:
 *   target_id → The commit ID we're looking for (e.g. 2)
 *   commit    → Pointer to Commit struct to fill in
 *
 * RETURNS:
 *   1 → Found! commit struct is filled in.
 *   0 → Not found. No commit with that ID exists.
 *
 * HOW:
 *   Read commits.dat line by line.
 *   When we find "COMMIT:2", start reading that block.
 *   Fill in the commit struct field by field.
 *   Stop when we hit "END".
 */
int find_commit_by_id(int target_id, Commit* commit) {

    FILE* fp = fopen(COMMITS_FILE, "r");

    if (!fp) {
        printf(RED
               "  ✗ No commits found!\n"
               RESET);
        return 0;
    }

    int found      = 0;  /* Are we inside the right block? */
    int current_id = -1; /* ID of block we're reading */
    int got_it     = 0;  /* Did we complete reading the commit? */

    char line[MAX_LINE];

    /* Initialize the commit struct */
    commit->file_count = 0;
    commit->parent     = NULL;
    commit->next       = NULL;

    while (fgets(line, sizeof(line), fp)) {

        /* Remove newline */
        line[strcspn(line, "\n\r")] = '\0';

        /* Skip empty lines and comments */
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        /* Detect new commit block */
        int rid;
        if (sscanf(line, "COMMIT:%d", &rid) == 1) {

            /*
             * If we were already reading our target commit
             * and we hit a NEW commit block,
             * something went wrong (no END marker found).
             * Break just in case.
             */
            if (found && got_it) break;

            current_id = rid;
            found = (current_id == target_id);

            if (found) {
                /* Store the ID */
                commit->id = target_id;
            }
            continue;
        }

        /* Only process lines if we're in the right block */
        if (!found) continue;

        /* ── Parse each field ── */

        /* Message */
        if (strncmp(line, "MSG:", 4) == 0) {
            strncpy(commit->message,
                    line + 4,
                    MAX_MESSAGE - 1);
            commit->message[MAX_MESSAGE - 1] = '\0';
            continue;
        }

        /* Timestamp */
        if (strncmp(line, "TIME:", 5) == 0) {
            strncpy(commit->timestamp,
                    line + 5, 63);
            commit->timestamp[63] = '\0';
            continue;
        }

        /* Branch */
        if (strncmp(line, "BRANCH:", 7) == 0) {
            strncpy(commit->branch,
                    line + 7,
                    MAX_BRANCH_NAME - 1);
            commit->branch[MAX_BRANCH_NAME - 1] = '\0';
            continue;
        }

        /* Parent ID */
        if (sscanf(line, "PARENT:%d",
                   &commit->parent_id) == 1) {
            continue;
        }

        /* Files list */
        if (strncmp(line, "FILES:", 6) == 0) {

            char copy[MAX_LINE];
            strncpy(copy, line + 6, MAX_LINE - 1);
            copy[MAX_LINE - 1] = '\0';

            char* tok = strtok(copy, ",");

            while (tok && commit->file_count < 10) {
                strncpy(
                    commit->filenames[commit->file_count],
                    tok,
                    MAX_FILENAME - 1);
                commit->filenames[commit->file_count]
                    [MAX_FILENAME - 1] = '\0';
                commit->file_count++;
                tok = strtok(NULL, ",");
            }
            continue;
        }

        /* Hashes list */
        if (strncmp(line, "HASHES:", 7) == 0) {

            char copy[MAX_LINE];
            strncpy(copy, line + 7, MAX_LINE - 1);
            copy[MAX_LINE - 1] = '\0';

            char* tok = strtok(copy, ",");
            int idx = 0;

            while (tok && idx < 10) {
                commit->file_hashes[idx] =
                    strtoul(tok, NULL, 10);
                idx++;
                tok = strtok(NULL, ",");
            }
            continue;
        }

        /* END marker — commit block is complete! */
        if (strcmp(line, "END") == 0 && found) {
            got_it = 1;
            break;
        }
    }

    fclose(fp);
    return got_it;
}


/*
 * FUNCTION: restore_file_from_blob
 * ─────────────────────────────────
 * Reads a blob from .mygit/objects/
 * and writes its content back to the working directory.
 *
 * This is the CORE of checkout!
 * "Bring back the old version of a file."
 *
 * PARAMETERS:
 *   filename → Name to write the file as (e.g. "hello.txt")
 *   hash     → Hash of the committed content
 *              (used to find the blob file)
 *
 * RETURNS:
 *   1 → Success
 *   0 → Failed (blob not found or write error)
 */
int restore_file_from_blob(const char* filename,
                           unsigned long hash) {

    /*
     * Build path to the blob file
     *
     * hash = 193485797
     * blob_path = ".mygit/objects/193485797.blob"
     */
    char blob_path[MAX_PATH];
    snprintf(blob_path, sizeof(blob_path),
             "%s/%lu.blob",
             OBJECTS_DIR,
             hash);

    /*
     * Check if blob exists
     *
     * WHY CHECK FIRST?
     * If blob doesn't exist, we can't restore!
     * Better to warn user now than to
     * silently fail or crash later.
     */
    if (!file_exists(blob_path)) {
        printf(RED
               "    ✗ Blob not found for '%s' "
               "(hash: %lu)\n"
               RESET,
               filename, hash);
        return 0;
    }

    /*
     * Read the blob content into memory
     */
    char content[MAX_CONTENT];

    if (read_file(blob_path, content,
                  MAX_CONTENT) < 0) {
        printf(RED
               "    ✗ Could not read blob for '%s'\n"
               RESET, filename);
        return 0;
    }

    /*
     * Write the content back to the working directory
     *
     * This OVERWRITES the current file!
     *
     * We use write_file() which opens in "w" mode
     * "w" mode = create if not exists, overwrite if exists
     *
     * After this call:
     *   hello.txt in current directory = old committed version
     */
    if (write_file(filename, content) != 0) {
        printf(RED
               "    ✗ Could not restore '%s'\n"
               RESET, filename);
        return 0;
    }

    return 1;  /* Success! */
}


/*
 * FUNCTION: show_available_commits
 * ──────────────────────────────────
 * Prints all available commit IDs.
 * Called when user gives invalid commit ID.
 *
 * Helps user pick a valid ID to checkout.
 */
void show_available_commits(void) {

    FILE* fp = fopen(COMMITS_FILE, "r");

    if (!fp) {
        printf(YELLOW
               "  No commits found.\n"
               RESET);
        return;
    }

    printf(CYAN
           "\n  Available commits:\n"
           RESET);

    char line[MAX_LINE];
    int current_id = -1;
    char current_msg[MAX_MESSAGE] = "";
    char current_branch[MAX_BRANCH_NAME] = "";

    while (fgets(line, sizeof(line), fp)) {

        line[strcspn(line, "\n\r")] = '\0';

        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        int rid;
        if (sscanf(line, "COMMIT:%d", &rid) == 1) {
            current_id = rid;
            continue;
        }

        if (strncmp(line, "MSG:", 4) == 0) {
            strncpy(current_msg, line + 4,
                    MAX_MESSAGE - 1);
            current_msg[MAX_MESSAGE - 1] = '\0';
            continue;
        }

        if (strncmp(line, "BRANCH:", 7) == 0) {
            strncpy(current_branch, line + 7,
                    MAX_BRANCH_NAME - 1);
            current_branch[MAX_BRANCH_NAME - 1] = '\0';
            continue;
        }

        if (strcmp(line, "END") == 0
            && current_id != -1) {
            /*
             * Print this commit's summary
             * User can use this ID for checkout
             */
            printf(YELLOW "    #%d" RESET
                   " [%s] %s\n",
                   current_id,
                   current_branch,
                   current_msg);
            current_id = -1;
        }
    }

    fclose(fp);
    printf("\n");
}


/*
 * FUNCTION: get_current_commit_id
 * ─────────────────────────────────
 * Returns the ID of the commit we're currently on.
 *
 * Used to:
 *   1. Warn if user tries to checkout current commit
 *   2. Show "returning from" info
 */
int get_current_commit_id(void) {

    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, branch);

    char id_str[20];
    if (read_file(ref_path, id_str,
                  sizeof(id_str)) < 0) {
        return 0;
    }

    return atoi(id_str);
}


/*
 * FUNCTION: print_checkout_summary
 * ─────────────────────────────────
 * Shows a beautiful summary of what was restored.
 */
void print_checkout_summary(Commit* commit,
                             int from_id) {

    printf("\n");
    printf(GREEN
           "  ✅ Checkout successful!\n\n"
           RESET);

    printf(CYAN
           "  ┌─────────────────────────────────────────┐\n"
           RESET);
    printf(CYAN "  │" RESET
           " Restored to Commit #%-20d"
           CYAN "│\n" RESET,
           commit->id);
    printf(CYAN "  │" RESET
           " Message : %-30s"
           CYAN "│\n" RESET,
           commit->message);
    printf(CYAN "  │" RESET
           " Branch  : %-30s"
           CYAN "│\n" RESET,
           commit->branch);
    printf(CYAN "  │" RESET
           " Time    : %-30s"
           CYAN "│\n" RESET,
           commit->timestamp);
    printf(CYAN "  │" RESET
           " From    : Commit #%-22d"
           CYAN "│\n" RESET,
           from_id);
    printf(CYAN
           "  └─────────────────────────────────────────┘\n"
           RESET);

    printf("\n");
    printf(GREEN
           "  Files restored (%d):\n"
           RESET,
           commit->file_count);

    for (int i = 0; i < commit->file_count; i++) {
        printf(GREEN "    ✓ " RESET
               "%s\n",
               commit->filenames[i]);
    }

    printf("\n");
    printf(YELLOW
           "  ⚠  Your working directory now reflects\n"
           "     the state at Commit #%d\n\n"
           RESET,
           commit->id);
}


/*
 * ═══════════════════════════════════════════════
 * MAIN FUNCTION: mygit_checkout
 * ═══════════════════════════════════════════════
 *
 * Called when user types: mygit checkout <target>
 *
 * target can be:
 *   → A commit ID number: "mygit checkout 2"
 *   → "latest" to go back to newest:  "mygit checkout latest"
 *
 * THE TIME TRAVEL ALGORITHM:
 *   1. Parse the target commit ID
 *   2. Find that commit in history
 *   3. Verify all blobs exist
 *   4. Warn user (destructive operation!)
 *   5. Restore each file from its blob
 *   6. Update branch reference
 *   7. Clear staging area (fresh start)
 *   8. Show summary
 */
int mygit_checkout(const char* target) {

    printf("\n");

    /*
     * ─────────────────────────────
     * STEP 1: Parse the target
     * ─────────────────────────────
     *
     * User might type:
     *   "2"       → checkout commit #2
     *   "latest"  → checkout the newest commit
     */
    int target_id;

    if (strcmp(target, "latest") == 0) {
        /*
         * "latest" means go to the newest commit
         * on the current branch.
         *
         * Read refs/main to find the latest ID.
         */
        char branch[MAX_BRANCH_NAME];
        get_current_branch(branch, sizeof(branch));

        char ref_path[MAX_PATH];
        snprintf(ref_path, sizeof(ref_path),
                 "%s/%s", REFS_DIR, branch);

        char id_str[20];
        if (read_file(ref_path, id_str,
                      sizeof(id_str)) < 0) {
            printf(RED
                   "  ✗ No commits found!\n\n"
                   RESET);
            return -1;
        }
        target_id = atoi(id_str);

    } else {
        /*
         * User gave a number → convert it
         *
         * atoi = "ASCII to integer"
         * "2" → 2
         * "42" → 42
         *
         * If user types "abc" → atoi returns 0
         * We check for this!
         */
        target_id = atoi(target);

        if (target_id <= 0) {
            printf(RED
                   "  ✗ Invalid commit ID: '%s'\n"
                   RESET, target);
            printf("  Please use a number: "
                   "mygit checkout 2\n");
            show_available_commits();
            return -1;
        }
    }

    /*
     * ─────────────────────────────
     * STEP 2: Check if already here
     * ─────────────────────────────
     *
     * If user is already on this commit,
     * no need to do anything!
     */
    int current_id = get_current_commit_id();

    if (current_id == target_id) {
        printf(YELLOW
               "  Already on commit #%d!\n\n"
               RESET, target_id);
        return 0;
    }

    /*
     * ─────────────────────────────
     * STEP 3: Find the commit
     * ─────────────────────────────
     *
     * Search commits.dat for commit with target_id.
     * Fill in our local 'target' commit struct.
     */
    Commit target_commit;

    if (!find_commit_by_id(target_id,
                           &target_commit)) {
        printf(RED
               "  ✗ Commit #%d not found!\n"
               RESET, target_id);
        show_available_commits();
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 4: Verify all blobs exist
     * ─────────────────────────────
     *
     * BEFORE we start overwriting files,
     * check that ALL blobs are available.
     *
     * WHY CHECK FIRST?
     *   If we restore 3 out of 4 files and then
     *   the 4th blob is missing, we're in a BROKEN STATE!
     *   Half old, half new. Nightmare!
     *
     *   By checking ALL blobs FIRST,
     *   we ensure complete restoration or nothing.
     *   This is called an ATOMIC operation:
     *   either ALL succeeds or NOTHING changes.
     */
    printf(CYAN
           "  Verifying blobs...\n"
           RESET);

    int all_blobs_ok = 1;  /* Assume all good until proven otherwise */

    for (int i = 0; i < target_commit.file_count; i++) {

        char blob_path[MAX_PATH];
        snprintf(blob_path, sizeof(blob_path),
                 "%s/%lu.blob",
                 OBJECTS_DIR,
                 target_commit.file_hashes[i]);

        if (file_exists(blob_path)) {
            printf(GREEN "    ✓ " RESET
                   "%s (hash: %lu)\n",
                   target_commit.filenames[i],
                   target_commit.file_hashes[i]);
        } else {
            printf(RED "    ✗ " RESET
                   "%s — BLOB MISSING!\n",
                   target_commit.filenames[i]);
            all_blobs_ok = 0;  /* Found a problem! */
        }
    }

    if (!all_blobs_ok) {
        printf(RED
               "\n  ✗ Checkout aborted: "
               "some blobs are missing!\n\n"
               RESET);
        return -1;
    }

    printf(GREEN
           "  All blobs verified!\n\n"
           RESET);

    /*
     * ─────────────────────────────
     * STEP 5: Warn the user
     * ─────────────────────────────
     *
     * Checkout OVERWRITES current files!
     * Any unsaved changes will be LOST!
     *
     * We warn the user and ask for confirmation.
     *
     * fflush(stdout) forces the printf to display
     * BEFORE we wait for input.
     * Without it, the text might not show immediately!
     */
    printf(YELLOW
           "  ⚠  WARNING: This will overwrite "
           "your current files!\n"
           RESET);
    printf(YELLOW
           "  ⚠  Any uncommitted changes will "
           "be LOST!\n\n"
           RESET);
    printf("  Restore to Commit #%d "
           "(\"%s\")? [y/n]: ",
           target_id,
           target_commit.message);

    fflush(stdout);  /* Force display before waiting for input */

    /*
     * Read user's answer
     *
     * We read ONE character: 'y' or 'n'
     *
     * getchar() reads one character from keyboard
     */
    char answer = getchar();

    /*
     * Clear the input buffer
     *
     * When user types 'y' and presses Enter,
     * the buffer has: ['y', '\n']
     *
     * getchar() reads 'y'.
     * But '\n' is still in the buffer!
     * Next time we call getchar() or scanf,
     * it would read '\n' immediately — Bug!
     *
     * We clear the buffer by reading until '\n':
     */
    int ch;
    while ((ch = getchar()) != '\n'
           && ch != EOF);

    printf("\n");

    if (answer != 'y' && answer != 'Y') {
        printf(YELLOW
               "  Checkout cancelled.\n\n"
               RESET);
        return 0;
    }

    /*
     * ─────────────────────────────
     * STEP 6: Restore all files!
     * ─────────────────────────────
     *
     * For each file in the target commit:
     *   Read its blob → Write to working directory
     *
     * This is the ACTUAL time travel!
     */
    printf(CYAN
           "  Restoring files...\n"
           RESET);

    int restored = 0;

    for (int i = 0; i < target_commit.file_count; i++) {

        char* fname = target_commit.filenames[i];
        unsigned long hash = target_commit.file_hashes[i];

        if (restore_file_from_blob(fname, hash)) {
            printf(GREEN "    ✓ Restored: " RESET
                   "%s\n", fname);
            restored++;
        }
    }

    /*
     * ─────────────────────────────
     * STEP 7: Update branch reference
     * ─────────────────────────────
     *
     * Tell the branch: "You're now at commit #X"
     *
     * Before: refs/main = "3"
     * After:  refs/main = "2"  (we went back!)
     */
    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, branch);

    char id_str[20];
    snprintf(id_str, sizeof(id_str),
             "%d", target_id);
    write_file(ref_path, id_str);

    /*
     * ─────────────────────────────
     * STEP 8: Clear staging area
     * ─────────────────────────────
     *
     * After checkout, staging area should be clean.
     * We're at a different point in history.
     * Old staged files don't make sense anymore.
     */
    FILE* staging = fopen(STAGING_FILE, "w");
    if (staging) {
        fprintf(staging, "# MyGit Staging Area\n");
        fclose(staging);
    }

    /*
     * ─────────────────────────────
     * STEP 9: Show summary!
     * ─────────────────────────────
     */
    print_checkout_summary(&target_commit, current_id);

    return 0;
}