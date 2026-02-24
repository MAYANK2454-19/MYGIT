/*
 * ============================================
 *          MYGIT - Commit Command
 *          "mygit commit <message>"
 * ============================================
 * 
 * PURPOSE:
 *   Take everything in the staging area and
 *   create a permanent SNAPSHOT (commit).
 * 
 *   Like pressing "SAVE" in a video game.
 *   You can always come back to this point!
 * 
 * DATA STRUCTURES USED:
 *   1. Linked List → Chain of commits (each points to parent)
 *   2. Array       → Storing filenames and hashes
 *   3. Hash values → Identifying file versions
 * 
 * FILE FORMAT (commits.dat):
 *   COMMIT:1
 *   MSG:Initial commit
 *   TIME:2025-01-15 14:30:00
 *   BRANCH:main
 *   PARENT:-1
 *   FILES:hello.txt,test.txt
 *   HASHES:193485797,874291053
 *   END
 */

#include "mygit.h"


/*
 * FUNCTION: count_staged_files
 * ────────────────────────────
 * Counts how many files are in the staging area.
 * 
 * WHY?
 *   If staging area is EMPTY, there's nothing to commit!
 *   We should warn the user instead of creating an empty commit.
 * 
 * HOW?
 *   Read staging.dat line by line.
 *   Skip comments (lines starting with #).
 *   Skip empty lines.
 *   Count everything else.
 * 
 * RETURNS:
 *   Number of staged files (0, 1, 2, ...)
 */
int count_staged_files(void) {

    FILE* fp = fopen(STAGING_FILE, "r");

    /* If staging file doesn't exist, 0 files staged */
    if (!fp) {
        return 0;
    }

    int count = 0;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {

        /* Remove newline at end */
        line[strcspn(line, "\n\r")] = '\0';

        /* Skip comments and empty lines */
        if (line[0] == '#' || strlen(line) == 0) {
            continue;
        }

        /*
         * If we reach here, this line is a real entry
         * like "hello.txt|193485797"
         * So count it!
         */
        count++;
    }

    fclose(fp);
    return count;
}


/*
 * FUNCTION: read_staged_files
 * ───────────────────────────
 * Reads the staging area and fills a Commit struct
 * with the filenames and hashes.
 * 
 * ANALOGY:
 *   The shopkeeper reads his waiting notebook
 *   and writes all the info onto the final receipt.
 * 
 * PARAMETERS:
 *   commit → Pointer to a Commit struct that we'll fill in
 *            (We use a POINTER so we can MODIFY the original)
 * 
 * Remember from class:
 *   void foo(int x)    → gets a COPY, can't change original
 *   void foo(int* x)   → gets ADDRESS, CAN change original
 *   
 *   Same idea here:
 *   void foo(Commit c)  → gets COPY of commit (useless!)
 *   void foo(Commit* c) → gets ADDRESS, we fill in the real one ✓
 * 
 * RETURNS:
 *   0  → Success (commit struct is now filled)
 *   -1 → Error
 */
int read_staged_files(Commit* commit) {

    FILE* fp = fopen(STAGING_FILE, "r");

    if (!fp) {
        return -1;
    }

    /*
     * Start with 0 files.
     * We'll increment this as we read each entry.
     */
    commit->file_count = 0;

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {

        /* Remove newline */
        line[strcspn(line, "\n\r")] = '\0';

        /* Skip comments and empty lines */
        if (line[0] == '#' || strlen(line) == 0) {
            continue;
        }

        /*
         * Safety check: Don't exceed our array size!
         * 
         * Our Commit struct has:
         *   char filenames[10][MAX_FILENAME]
         *   unsigned long file_hashes[10]
         * 
         * So we can store at most 10 files per commit.
         * If user staged more than 10, we stop.
         * 
         * In a real project, you'd use dynamic arrays or
         * linked lists. But 10 is enough for our demo!
         */
        if (commit->file_count >= 10) {
            printf(YELLOW "⚠ Maximum 10 files per commit. Extra files skipped.\n" RESET);
            break;   /* "break" exits the while loop immediately */
        }

        /*
         * Parse the line: "hello.txt|193485797"
         * 
         * We need to split it at the '|' character
         * Left side  → filename  ("hello.txt")
         * Right side → hash      (193485797)
         * 
         * We'll use strtok (learned in Day 2!)
         */

        /* Make a copy because strtok MODIFIES the string */
        char temp[MAX_LINE];
        strcpy(temp, line);

        /* Get the filename (part before '|') */
        char* filename = strtok(temp, "|");

        /* Get the hash string (part after '|') */
        char* hash_str = strtok(NULL, "|");

        /*
         * strtok returns NULL if there's nothing left.
         * We need BOTH parts to be valid.
         * 
         * && means AND → Both conditions must be true
         */
        if (filename && hash_str) {

            /*
             * Store the filename in our commit's array
             * 
             * commit->filenames[0] = "hello.txt"
             * commit->filenames[1] = "test.txt"
             * ...
             * 
             * commit->file_count tells us which SLOT to use
             * First file → slot 0
             * Second file → slot 1
             * etc.
             */
            strcpy(commit->filenames[commit->file_count], filename);

            /*
             * Convert hash from STRING to NUMBER
             * 
             * hash_str = "193485797" (this is TEXT, not a number!)
             * 
             * strtoul = "string to unsigned long"
             * Converts "193485797" → 193485797
             * 
             * strtoul(STRING, END_POINTER, BASE)
             *   STRING      → "193485797"
             *   END_POINTER → NULL (we don't care where it stopped)
             *   BASE        → 10 (decimal number system)
             * 
             * Other bases:
             *   Base 2  → Binary  (0, 1)
             *   Base 10 → Decimal (0-9)     ← we use this
             *   Base 16 → Hex     (0-9, A-F)
             */
            commit->file_hashes[commit->file_count] = strtoul(hash_str, NULL, 10);

            /* Move to next slot */
            commit->file_count++;
        }
    }

    fclose(fp);
    return 0;
}


/*
 * FUNCTION: get_last_commit_id_on_branch
 * ──────────────────────────────────────
 * Finds the most recent commit on the current branch.
 * This becomes the PARENT of our new commit.
 * 
 * ANALOGY:
 *   "What was the LAST receipt number?"
 *   That receipt becomes the 'previous' for our new receipt.
 * 
 * HOW:
 *   Read refs/main file → it contains the latest commit ID
 * 
 * RETURNS:
 *   Commit ID (positive number), or 0 if no commits yet
 */
int get_last_commit_id_on_branch(const char* branch) {

    /*
     * Build path to branch reference file
     * 
     * branch = "main"
     * ref_path = ".mygit/refs/main"
     * 
     * This file contains just ONE number:
     * the ID of the latest commit on this branch
     */
    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path), "%s/%s", REFS_DIR, branch);

    /* Read the file content */
    char content[64];

    if (read_file(ref_path, content, sizeof(content)) < 0) {
        return 0;   /* File doesn't exist → no commits yet */
    }

    /*
     * atoi = "ASCII to integer"
     * Converts "5" → 5
     * Converts "0" → 0
     * 
     * Simple but works for our case!
     * 
     * content = "3"  → atoi returns 3
     * content = "0"  → atoi returns 0
     */
    return atoi(content);
}


/*
 * FUNCTION: save_commit
 * ─────────────────────
 * Writes the commit to commits.dat file.
 * 
 * THIS IS THE KEY MOMENT!
 * We're adding a new NODE to our linked list (on disk).
 * 
 * The file format:
 *   COMMIT:2
 *   MSG:Added new feature
 *   TIME:2025-01-15 14:30:00
 *   BRANCH:main
 *   PARENT:1
 *   FILES:hello.txt,test.txt
 *   HASHES:193485797,874291053
 *   END
 * 
 * WHY THIS FORMAT?
 *   → Each field on its own line → easy to read with fgets
 *   → Key:Value format → easy to parse with sscanf
 *   → "END" marker → we know where each commit ends
 *   → Human-readable → you can open the file and understand it!
 * 
 * PARAMETERS:
 *   commit → Pointer to the filled Commit struct
 * 
 * RETURNS:
 *   0 → Success
 *   -1 → Error
 */
int save_commit(Commit* commit) {

    /*
     * Open commits.dat in APPEND mode ("a")
     * 
     * We APPEND because we want to ADD new commits
     * without DELETING old ones!
     * 
     * "w" → Would erase all old commits! BAD!
     * "a" → Adds to the end. Old commits stay safe! GOOD!
     */
    FILE* fp = fopen(COMMITS_FILE, "a");

    if (!fp) {
        printf(RED "✗ Could not open commits file\n" RESET);
        return -1;
    }

    /*
     * Write the commit header
     * Each fprintf writes one line
     */
    fprintf(fp, "COMMIT:%d\n", commit->id);
    fprintf(fp, "MSG:%s\n", commit->message);
    fprintf(fp, "TIME:%s\n", commit->timestamp);
    fprintf(fp, "BRANCH:%s\n", commit->branch);
    fprintf(fp, "PARENT:%d\n", commit->parent_id);

    /*
     * Write filenames as comma-separated list
     * 
     * If we have: filenames[0]="hello.txt", filenames[1]="test.txt"
     * We write:   FILES:hello.txt,test.txt
     * 
     * The tricky part: commas go BETWEEN items, not after the last one!
     * 
     *   hello.txt,test.txt     ← CORRECT (comma between)
     *   hello.txt,test.txt,    ← WRONG (extra comma at end)
     * 
     * Solution:
     *   For the FIRST item (i=0): just write the name
     *   For items after that (i>0): write comma THEN name
     */
    fprintf(fp, "FILES:");
    for (int i = 0; i < commit->file_count; i++) {
        if (i > 0) {
            fprintf(fp, ",");   /* Comma before 2nd, 3rd, etc. */
        }
        fprintf(fp, "%s", commit->filenames[i]);
    }
    fprintf(fp, "\n");   /* End the FILES line */

    /*
     * Write hashes as comma-separated list (same pattern)
     */
    fprintf(fp, "HASHES:");
    for (int i = 0; i < commit->file_count; i++) {
        if (i > 0) {
            fprintf(fp, ",");
        }
        fprintf(fp, "%lu", commit->file_hashes[i]);
    }
    fprintf(fp, "\n");

    /*
     * Write the END marker
     * 
     * This tells our reader: "This commit entry is complete"
     * Without it, we wouldn't know where one commit ends
     * and the next begins!
     */
    fprintf(fp, "END\n");

    fclose(fp);
    return 0;
}


/*
 * FUNCTION: update_branch_ref
 * ───────────────────────────
 * Updates the branch pointer to the new commit.
 * 
 * BEFORE: refs/main contains "1" (pointing to commit #1)
 * AFTER:  refs/main contains "2" (now pointing to commit #2)
 * 
 * ANALOGY:
 *   The board in the shop says:
 *   "main branch → last receipt was #1"
 *   
 *   After new commit:
 *   "main branch → last receipt was #2"  ← UPDATED!
 * 
 * WHY?
 *   When we make the NEXT commit, we need to know
 *   what the current latest commit is.
 *   That becomes the new commit's PARENT.
 */
void update_branch_ref(const char* branch, int commit_id) {

    /* Build path: ".mygit/refs/main" */
    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path), "%s/%s", REFS_DIR, branch);

    /*
     * Convert commit_id from NUMBER to STRING
     * 
     * commit_id = 2
     * We need to write "2" to the file (as text)
     * 
     * snprintf converts: 2 → "2"
     */
    char id_str[20];
    snprintf(id_str, sizeof(id_str), "%d", commit_id);

    /* Write to file (overwrites old value) */
    write_file(ref_path, id_str);
}


/*
 * FUNCTION: clear_staging_area
 * ────────────────────────────
 * Empties the staging area after a successful commit.
 * 
 * WHY?
 *   After committing, the staging area should be CLEAN.
 *   Like clearing your shopping cart after checkout!
 *   
 *   If we don't clear it, next commit would include
 *   the same files again — that's wrong!
 * 
 * HOW?
 *   Simply overwrite staging.dat with just the header comment.
 */
void clear_staging_area(void) {

    /*
     * "w" mode OVERWRITES the entire file
     * We write only the comment header
     * All staged file entries are GONE
     * 
     * BEFORE:
     *   # MyGit Staging Area
     *   hello.txt|193485797
     *   test.txt|874291053
     * 
     * AFTER:
     *   # MyGit Staging Area
     *   (that's it! clean!)
     */
    FILE* fp = fopen(STAGING_FILE, "w");

    if (fp) {
        fprintf(fp, "# MyGit Staging Area\n");
        fclose(fp);
    }
}


/*
 * ═══════════════════════════════════════════════
 * MAIN FUNCTION: mygit_commit
 * ═══════════════════════════════════════════════
 * 
 * This is what runs when user types:
 *   mygit commit "Added new feature"
 * 
 * THE LINKED LIST CONNECTION:
 *   Each commit is a NODE.
 *   parent_id is the NEXT pointer.
 *   The chain goes: newest → oldest
 *   
 *   In memory linked list:  node->next = previous_node
 *   In our file:             commit.parent_id = previous_commit.id
 *   
 *   Same concept! Just stored in a FILE instead of RAM.
 */
int mygit_commit(const char* message) {

    /*
     * ──────────────────────────────────
     * STEP 1: Check if anything is staged
     * ──────────────────────────────────
     * 
     * No staged files = Nothing to commit!
     * 
     * Like going to checkout with an empty cart.
     * The cashier says: "You have nothing to buy!"
     */
    int staged_count = count_staged_files();

    if (staged_count == 0) {
        printf(YELLOW "⚠ Nothing to commit!\n" RESET);
        printf("  Stage files first with: mygit add <filename>\n");
        return -1;
    }

    /*
     * ──────────────────────────────────
     * STEP 2: Create a Commit struct
     * ──────────────────────────────────
     * 
     * This is us creating a NEW NODE for our linked list!
     * 
     * We declare it as a LOCAL variable (on the stack).
     * We'll fill in all its fields step by step.
     */
    Commit new_commit;

    /*
     * ──────────────────────────────────
     * STEP 3: Fill in the commit ID
     * ──────────────────────────────────
     * 
     * Each commit gets a unique number.
     * First commit = 1, second = 2, etc.
     * 
     * get_next_commit_id reads existing commits
     * and returns max_id + 1
     */
    new_commit.id = get_next_commit_id();

    /*
     * ──────────────────────────────────
     * STEP 4: Fill in the message
     * ──────────────────────────────────
     * 
     * Copy the user's message into our struct.
     * 
     * strncpy is SAFER than strcpy:
     *   strcpy  → copies everything (might overflow!)
     *   strncpy → copies at most N characters (safe!)
     * 
     * strncpy(DESTINATION, SOURCE, MAX_CHARS)
     * 
     * Like filling a glass:
     *   strcpy  → keeps pouring even if glass is full (overflows!)
     *   strncpy → stops pouring when glass is full (safe!)
     */
    strncpy(new_commit.message, message, MAX_MESSAGE - 1);

    /*
     * strncpy has a quirk: it might NOT add '\0' at the end
     * if the source string is exactly MAX_MESSAGE-1 or longer.
     * 
     * So we MANUALLY add '\0' at the last position.
     * Better safe than sorry!
     */
    new_commit.message[MAX_MESSAGE - 1] = '\0';

    /*
     * ──────────────────────────────────
     * STEP 5: Fill in the timestamp
     * ──────────────────────────────────
     * 
     * Record WHEN this commit was made.
     * get_timestamp fills the buffer with current date/time.
     */
    get_timestamp(new_commit.timestamp, sizeof(new_commit.timestamp));

    /*
     * ──────────────────────────────────
     * STEP 6: Fill in the branch name
     * ──────────────────────────────────
     * 
     * Read the HEAD file to know which branch we're on.
     * Usually "main" unless user created other branches.
     */
    get_current_branch(new_commit.branch, sizeof(new_commit.branch));

    /*
     * ──────────────────────────────────
     * STEP 7: Fill in the parent ID
     * ──────────────────────────────────
     * 
     * THIS IS THE LINKED LIST POINTER!
     * 
     * "Who was the last commit on this branch?"
     * That commit becomes our PARENT.
     * 
     * If this is the FIRST commit, parent = -1
     * (like NULL in a linked list)
     * 
     * If last commit was #3, parent = 3
     * (our new commit points BACK to #3)
     */
    int last_id = get_last_commit_id_on_branch(new_commit.branch);

    if (last_id == 0) {
        new_commit.parent_id = -1;   /* First commit ever! No parent. */
    } else {
        new_commit.parent_id = last_id;   /* Point to previous commit */
    }

    /*
     * ──────────────────────────────────
     * STEP 8: Fill in the file list
     * ──────────────────────────────────
     * 
     * Read staged files from staging.dat
     * and put them into our commit struct.
     * 
     * Remember: we pass a POINTER (&new_commit)
     * so the function can MODIFY our struct!
     * 
     * Without &: function gets a COPY → changes are lost
     * With &:    function gets the ADDRESS → changes stick!
     */
    if (read_staged_files(&new_commit) != 0) {
        printf(RED "✗ Failed to read staging area\n" RESET);
        return -1;
    }

    /*
     * Initialize the linked list pointers to NULL
     * We're not using these for file storage,
     * but it's good practice to initialize ALL fields.
     * 
     * Uninitialized pointers contain GARBAGE values
     * and can cause crashes if accidentally used!
     */
    new_commit.parent = NULL;
    new_commit.next = NULL;

    /*
     * ──────────────────────────────────
     * STEP 9: Save the commit!
     * ──────────────────────────────────
     * 
     * Write everything to commits.dat
     * This is like inserting a node into our linked list file!
     */
    if (save_commit(&new_commit) != 0) {
        printf(RED "✗ Failed to save commit\n" RESET);
        return -1;
    }

    /*
     * ──────────────────────────────────
     * STEP 10: Update the branch pointer
     * ──────────────────────────────────
     * 
     * Tell the branch: "Your latest commit is now #X"
     * 
     * refs/main: "1" → refs/main: "2"
     */
    update_branch_ref(new_commit.branch, new_commit.id);

    /*
     * ──────────────────────────────────
     * STEP 11: Clear the staging area
     * ──────────────────────────────────
     * 
     * Shopping cart is now EMPTY after checkout!
     */
    clear_staging_area();

    /*
     * ──────────────────────────────────
     * STEP 12: Tell the user!
     * ──────────────────────────────────
     * 
     * Show a nice summary of what was committed.
     */
    printf("\n");
    printf(GREEN "✅ Commit successful!\n" RESET);
    printf("\n");

    /* Draw a nice box around the commit info */
    printf(CYAN "  ┌─────────────────────────────────────────┐\n" RESET);
    printf(CYAN "  │" YELLOW " Commit #%d" CYAN, new_commit.id);

    /*
     * Padding: We want the box to look aligned
     * 
     * "Commit #1"  = 9 characters  → need 30 spaces after
     * "Commit #12" = 10 characters → need 29 spaces after
     * 
     * We calculate how many spaces to add:
     *   Total box width (inner) = 39
     *   Text length varies
     *   Padding = 39 - text length
     * 
     * But for simplicity, let's just use a fixed format:
     */
    /* Calculate the length of commit id for padding */
    int id_digits = 0;
    int temp_id = new_commit.id;
    do {
        id_digits++;
        temp_id /= 10;
    } while (temp_id > 0);

    /* Print spaces to align the right border */
    for (int i = 0; i < 30 - id_digits; i++) {
        printf(" ");
    }
    printf("│\n" RESET);

    printf(CYAN "  │" RESET " Message: %-30s" CYAN "│\n" RESET, new_commit.message);
    printf(CYAN "  │" RESET " Branch:  %-30s" CYAN "│\n" RESET, new_commit.branch);
    printf(CYAN "  │" RESET " Time:    %-30s" CYAN "│\n" RESET, new_commit.timestamp);
    printf(CYAN "  │" RESET " Parent:  %-30d" CYAN "│\n" RESET, new_commit.parent_id);

    printf(CYAN "  │" RESET " Files:   %-30d" CYAN "│\n" RESET, new_commit.file_count);

    /* List each file */
    for (int i = 0; i < new_commit.file_count; i++) {
        printf(CYAN "  │" RESET "   → %-35s" CYAN "│\n" RESET, new_commit.filenames[i]);
    }

    printf(CYAN "  └─────────────────────────────────────────┘\n" RESET);
    printf("\n");

    return 0;   /* Success! */
}