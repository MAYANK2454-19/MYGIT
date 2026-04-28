/*
 * ============================================
 *          MYGIT - Log Command
 *          "mygit log"
 * ============================================
 *
 * PURPOSE:
 *   Show the complete history of commits
 *   Newest commit first → oldest last
 *
 * DATA STRUCTURES USED:
 *   1. Array    → Temporarily store commits while reading
 *   2. Linked List → Connect commits with pointers
 *   3. Traversal   → Walk the list from head to NULL
 *
 * THIS IS WHERE WE FINALLY USE A REAL LINKED LIST IN MEMORY!
 *
 * ALGORITHM:
 *   1. Read all commits from file into array
 *   2. Allocate memory for each commit node
 *   3. Connect nodes with pointers (newest → oldest)
 *   4. Traverse and print
 *   5. Free all allocated memory
 */

#include "mygit.h"


/*
 * FUNCTION: parse_commit_block
 * ────────────────────────────
 * Reads ONE commit block from the file
 * and fills a Commit struct with the data.
 *
 * A commit block looks like this in commits.dat:
 *   COMMIT:2
 *   MSG:Added feature
 *   TIME:2025-01-15 14:30:00
 *   BRANCH:main
 *   PARENT:1
 *   FILES:hello.txt,test.txt
 *   HASHES:193485797,874291053
 *   END
 *
 * We read line by line until we hit "END"
 *
 * PARAMETERS:
 *   fp     → File pointer (already opened, positioned at start of block)
 *   commit → Pointer to commit struct to fill in
 *
 * RETURNS:
 *   1 → Successfully read a commit block
 *   0 → No more commits (reached end of file)
 */
int parse_commit_block(FILE* fp, Commit* commit) {

    char line[MAX_LINE];

    /*
     * Initialize file_count to 0
     * We'll increment it as we find files
     */
    commit->file_count = 0;
    commit->parent = NULL;
    commit->next = NULL;

    /*
     * Flag to track if we found a real commit
     * (vs just reading comment lines or empty lines)
     */
    int found_commit = 0;

    /*
     * Read line by line until "END" or end of file
     */
    while (fgets(line, sizeof(line), fp)) {

        /* Remove newline at end of line */
        line[strcspn(line, "\n\r")] = '\0';

        /* Skip empty lines and comments */
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        /*
         * "END" means this commit block is finished!
         * Return 1 to say "yes we found a complete commit"
         */
        if (strcmp(line, "END") == 0) {
            return found_commit;
        }

        /*
         * Now parse each line based on its prefix
         *
         * Each line is: "KEY:VALUE"
         * We check what KEY it is and store the VALUE
         *
         * sscanf pattern:
         *   "COMMIT:%d"      → reads an integer after "COMMIT:"
         *   "MSG:%[^\n]"     → reads everything after "MSG:"
         *   "PARENT:%d"      → reads an integer after "PARENT:"
         *
         * %[^\n] means "read everything that is NOT a newline"
         * This lets us capture the full message including spaces!
         */

        /* ── COMMIT ID ── */
        if (sscanf(line, "COMMIT:%d", &commit->id) == 1) {
            found_commit = 1;   /* We found at least one real commit field */
            continue;
        }

        /* ── MESSAGE ── */
        if (strncmp(line, "MSG:", 4) == 0) {
            /*
             * strncmp compares first N characters
             * strncmp(line, "MSG:", 4) checks if line STARTS with "MSG:"
             *
             * Then we copy everything AFTER "MSG:" (starting at index 4)
             * line + 4 means "skip the first 4 characters"
             *
             * line     = "MSG:Added feature"
             * line + 4 = "Added feature"       ← skip "MSG:"
             */
            strncpy(commit->message, line + 4, MAX_MESSAGE - 1);
            commit->message[MAX_MESSAGE - 1] = '\0';
            continue;
        }

        /* ── TIMESTAMP ── */
        if (strncmp(line, "TIME:", 5) == 0) {
            strncpy(commit->timestamp, line + 5, 63);
            commit->timestamp[63] = '\0';
            continue;
        }

        /* ── BRANCH ── */
        if (strncmp(line, "BRANCH:", 7) == 0) {
            strncpy(commit->branch, line + 7, MAX_BRANCH_NAME - 1);
            commit->branch[MAX_BRANCH_NAME - 1] = '\0';
            continue;
        }

        /* ── PARENT ID ── */
        if (sscanf(line, "PARENT:%d", &commit->parent_id) == 1) {
            continue;
        }

        /* ── FILES ── */
        if (strncmp(line, "FILES:", 6) == 0) {
            /*
             * line = "FILES:hello.txt,test.txt"
             * line + 6 = "hello.txt,test.txt"
             *
             * We need to split by comma ','
             * Using strtok just like Day 2!
             *
             * But strtok modifies the string,
             * so we copy it first
             */
            char files_copy[MAX_LINE];
            strncpy(files_copy, line + 6, MAX_LINE - 1);
            files_copy[MAX_LINE - 1] = '\0';

            /*
             * Split by comma:
             * "hello.txt,test.txt" → "hello.txt" then "test.txt"
             */
            char* token = strtok(files_copy, ",");

            while (token != NULL && commit->file_count < 10) {
                strncpy(commit->filenames[commit->file_count],
                        token,
                        MAX_FILENAME - 1);
                commit->filenames[commit->file_count][MAX_FILENAME - 1] = '\0';
                commit->file_count++;

                /*
                 * strtok(NULL, ",") continues splitting the SAME string
                 * First call:  strtok("hello.txt,test.txt", ",") → "hello.txt"
                 * Second call: strtok(NULL, ",")                 → "test.txt"
                 * Third call:  strtok(NULL, ",")                 → NULL (done!)
                 */
                token = strtok(NULL, ",");
            }
            continue;
        }

        /* ── HASHES ── */
        if (strncmp(line, "HASHES:", 7) == 0) {
            /*
             * Same pattern as FILES but for numbers
             * "193485797,874291053" → 193485797 then 874291053
             */
            char hashes_copy[MAX_LINE];
            strncpy(hashes_copy, line + 7, MAX_LINE - 1);
            hashes_copy[MAX_LINE - 1] = '\0';

            char* token = strtok(hashes_copy, ",");
            int idx = 0;

            while (token != NULL && idx < 10) {
                commit->file_hashes[idx] = strtoul(token, NULL, 10);
                idx++;
                token = strtok(NULL, ",");
            }
            continue;
        }
    }

    /*
     * If we reach here without hitting "END",
     * we've reached the end of the file.
     * Return whatever we found.
     */
    return found_commit;
}


/*
 * FUNCTION: load_all_commits
 * ──────────────────────────
 * Reads ALL commits from commits.dat
 * and builds a LINKED LIST in memory!
 *
 * THIS IS THE KEY FUNCTION OF DAY 4!
 *
 * HOW IT WORKS:
 *   1. Allocate memory for each commit (malloc!)
 *   2. Read each commit block from file
 *   3. Store pointers in an array
 *   4. Connect pointers: newest → oldest
 *   5. Return the HEAD of the linked list
 *
 * RETURNS:
 *   Pointer to the HEAD of the list (newest commit)
 *   NULL if no commits exist
 */
Commit* load_all_commits(void) {

    FILE* fp = fopen(COMMITS_FILE, "r");

    if (!fp) {
        return NULL;   /* No commits file → no commits */
    }

    /*
     * We'll store pointers to commits in this array
     * Maximum 1000 commits (enough for our project!)
     *
     * Each element is a Commit* (pointer to a Commit)
     */
    Commit* commits[1000];
    int count = 0;

    /*
     * Read commits one by one until file is done
     *
     * MEMORY ALLOCATION with malloc!
     *
     * malloc = "memory allocate"
     * Asks the operating system: "Give me X bytes of memory"
     * Returns a POINTER to that memory
     *
     * sizeof(Commit) = how many bytes one Commit struct needs
     *
     * ANALOGY:
     *   malloc = "Reserve a parking spot"
     *   free   = "Leave the parking spot (give it back)"
     *   sizeof = "How big is my car?"
     *
     * Without free → MEMORY LEAK!
     * Like reserving a parking spot and never leaving!
     */
    while (count < 1000) {

        /*
         * Allocate memory for ONE commit node
         *
         * malloc returns void* (generic pointer)
         * We cast it to Commit* with (Commit*)
         *
         * If malloc fails (out of memory), it returns NULL
         */
        Commit* new_node = (Commit*)malloc(sizeof(Commit));

        if (new_node == NULL) {
            printf(RED "✗ Out of memory!\n" RESET);
            break;
        }

        /*
         * Try to read the next commit block from file
         * parse_commit_block fills in new_node's fields
         *
         * Returns 1 if found a commit, 0 if end of file
         */
        if (parse_commit_block(fp, new_node) == 0) {
            /*
             * No more commits found
             * Free this node (we don't need it)
             * Break out of loop
             */
            free(new_node);
            break;
        }

        /* Store the pointer in our array */
        commits[count] = new_node;
        count++;
    }

    fclose(fp);

    /* If no commits were found, return NULL */
    if (count == 0) {
        return NULL;
    }

    /*
     * NOW: Connect the commits into a LINKED LIST!
     *
     * They're currently in ARRAY form (oldest first):
     *   commits[0] = Commit#1
     *   commits[1] = Commit#2
     *   commits[2] = Commit#3
     *
     * We want a linked list (newest first):
     *   Commit#3 → Commit#2 → Commit#1 → NULL
     *
     * Strategy:
     *   Go through array from LAST to FIRST
     *   Connect each node to the one before it
     *
     * SORTING:
     *   Actually, let's sort by ID (descending) first
     *   using a simple BUBBLE SORT
     */

    /*
     * ── BUBBLE SORT (descending by ID) ──
     *
     * Bubble sort compares PAIRS of adjacent elements
     * If they're in the wrong order → SWAP them
     * Repeat until fully sorted
     *
     * Example: [3, 1, 2] sorted descending → [3, 2, 1]
     *
     * Round 1:
     *   Compare 3,1 → 3>1 → OK, no swap    [3, 1, 2]
     *   Compare 1,2 → 1<2 → SWAP!          [3, 2, 1]
     *
     * Round 2:
     *   Compare 3,2 → 3>2 → OK, no swap    [3, 2, 1]
     *   Compare 2,1 → 2>1 → OK, no swap    [3, 2, 1]
     *
     * Done! Array is sorted descending!
     *
     * Time complexity: O(n²) — fine for our small list
     */
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (commits[j]->id < commits[j+1]->id) {
                /* SWAP these two pointers */
                Commit* temp = commits[j];
                commits[j] = commits[j+1];
                commits[j+1] = temp;
            }
        }
    }

    /*
     * ── CONNECT POINTERS ──
     *
     * After sorting: commits[0]=Commit#3, commits[1]=Commit#2, commits[2]=Commit#1
     *
     * Connect each to the next:
     *   commits[0]->next = commits[1]   (Commit#3 → Commit#2)
     *   commits[1]->next = commits[2]   (Commit#2 → Commit#1)
     *   commits[2]->next = NULL         (Commit#1 → end)
     */
    for (int i = 0; i < count - 1; i++) {
        commits[i]->next = commits[i + 1];
    }

    /* Last node points to NULL (end of list) */
    commits[count - 1]->next = NULL;

    /*
     * Return the HEAD of the linked list
     * HEAD = first element = newest commit = commits[0]
     */
    return commits[0];
}


/*
 * FUNCTION: free_commit_list
 * ──────────────────────────
 * Frees ALL memory we allocated with malloc.
 *
 * RULE: Every malloc() MUST have a matching free()!
 *
 * ANALOGY:
 *   malloc = Borrow a book from library
 *   free   = Return the book
 *
 *   If you never return books, the library runs out!
 *   If you never free memory, the computer runs out of RAM!
 *   This is called a MEMORY LEAK.
 *
 * HOW:
 *   Walk the linked list from head to NULL
 *   Free each node as we go
 *
 * IMPORTANT: Save the NEXT pointer BEFORE freeing!
 *   After free(current), current's memory is gone!
 *   We can't read current->next anymore!
 *   So we save it first.
 */
void free_commit_list(Commit* head) {

    Commit* current = head;

    while (current != NULL) {

        /*
         * Save next pointer BEFORE freeing current!
         *
         * BAD (will crash!):
         *   free(current);
         *   current = current->next;  ← current is already freed!
         *
         * GOOD:
         *   Commit* next = current->next;  ← save first
         *   free(current);                  ← now free safely
         *   current = next;                 ← use saved pointer
         */
        Commit* next = current->next;
        free(current);
        current = next;
    }
}


/*
 * FUNCTION: print_commit
 * ──────────────────────
 * Prints ONE commit in a beautiful format.
 *
 * PARAMETERS:
 *   commit      → The commit to print
 *   show_files  → 1 = show file list, 0 = hide it (compact mode)
 */
void print_commit(Commit* commit, int show_files) {

    /*
     * Different colors for different things:
     *   YELLOW  → Commit ID and branch
     *   WHITE   → Message
     *   CYAN    → Metadata (time, parent)
     *   GREEN   → Files
     */

    printf(YELLOW "  ●  Commit #%d" RESET, commit->id);
    printf(" [" MAGENTA "%s" RESET "]\n", commit->branch);

    printf("  │  " GREEN "Message : " RESET "%s\n", commit->message);
    printf("  │  " CYAN  "Time    : " RESET "%s\n", commit->timestamp);

    if (commit->parent_id == -1) {
        printf("  │  " CYAN "Parent  : " RESET "(root commit — no parent)\n");
    } else {
        printf("  │  " CYAN "Parent  : " RESET "#%d\n", commit->parent_id);
    }

    if (show_files && commit->file_count > 0) {
        printf("  │  " CYAN "Files   : " RESET "%d file(s)\n", commit->file_count);
        for (int i = 0; i < commit->file_count; i++) {
            printf("  │    " GREEN "→ " RESET "%s\n", commit->filenames[i]);
        }
    }

    printf("  │\n");
}


/*
 * ═══════════════════════════════════════════════
 * MAIN FUNCTION: mygit_log
 * ═══════════════════════════════════════════════
 *
 * This is what runs when user types: mygit log
 *
 * THE LINKED LIST TRAVERSAL:
 *
 *   head → Commit#3 → Commit#2 → Commit#1 → NULL
 *
 *   current = head        → print Commit#3
 *   current = current->next → print Commit#2
 *   current = current->next → print Commit#1
 *   current = current->next → NULL → STOP!
 *
 * This is TEXTBOOK linked list traversal!
 */
int mygit_log(void) {

    /*
     * ──────────────────────────────────
     * STEP 1: Load all commits into memory
     * ──────────────────────────────────
     *
     * load_all_commits reads commits.dat
     * builds a linked list
     * returns the HEAD pointer
     */
    Commit* head = load_all_commits();

    /*
     * If head is NULL → no commits exist
     */
    if (head == NULL) {
        printf(YELLOW "\n  ⚠ No commits yet!\n" RESET);
        printf("  Start with: mygit add <file> → mygit commit \"message\"\n\n");
        return 0;
    }

    /*
     * ──────────────────────────────────
     * STEP 2: Count total commits
     * ──────────────────────────────────
     *
     * Walk the list to count nodes
     * This is our first TRAVERSAL!
     */
    int total = 0;
    Commit* temp = head;

    while (temp != NULL) {
        total++;
        temp = temp->next;
    }

    /*
     * Print the header
     */
    printf("\n");
    printf(CYAN "  ╔══════════════════════════════════════════╗\n" RESET);
    printf(CYAN "  ║" YELLOW "         📜 COMMIT HISTORY                " CYAN "║\n" RESET);
    printf(CYAN "  ║" RESET "         Total: %-3d commits              " CYAN "  ║\n" RESET, total);
    printf(CYAN "  ╚══════════════════════════════════════════╝\n" RESET);
    printf("\n");

    /*
     * ──────────────────────────────────
     * STEP 3: TRAVERSE THE LINKED LIST!
     * ──────────────────────────────────
     *
     * THIS IS THE MOMENT!
     * Classic linked list traversal:
     *
     *   current = head         (start at first node)
     *   while not NULL:
     *     process current node
     *     move to next node
     *
     * We start at head (newest commit)
     * Follow ->next until we reach NULL (oldest commit)
     */
    Commit* current = head;

    while (current != NULL) {

        /* Print this commit */
        print_commit(current, 1);

        /*
         * If there's a next commit,
         * print an arrow connecting them
         */
        if (current->next != NULL) {
            printf("  ↓\n");
        }

        /*
         * MOVE TO NEXT NODE!
         * This is the KEY LINE of linked list traversal.
         *
         * current = current->next
         *
         * "current" now points to the NEXT commit in the chain.
         *
         * Visual:
         *   Before: current → [Commit#3] → [Commit#2] → [Commit#1] → NULL
         *   After:             [Commit#3] → current → [Commit#2] → ... wait this is wrong
         *
         * Actually:
         *   Before: current points to Commit#3
         *   After:  current points to Commit#2
         *
         * "Move the pointer to where it's pointing at"
         */
        current = current->next;
    }

    /* Print the end marker */
    printf("  " YELLOW "◉" RESET " (beginning of history)\n\n");

    /*
     * ──────────────────────────────────
     * STEP 4: FREE THE MEMORY!
     * ──────────────────────────────────
     *
     * We're done with the list.
     * Give back all the memory we borrowed!
     *
     * Every malloc must have a free.
     */
    free_commit_list(head);

    return 0;
    
}
/*
 * FUNCTION: mygit_show
 * ─────────────────────
 * Shows detailed info about ONE specific commit.
 *
 * Like "git show <commit_id>"
 *
 * PARAMETERS:
 *   commit_id → Which commit to show
 */
int mygit_show(int commit_id) {

    if (commit_id <= 0) {
        printf(RED
               "\n  ✗ Invalid commit ID!\n\n"
               RESET);
        return -1;
    }

    /* Load all commits */
    Commit* head = load_all_commits();

    if (!head) {
        printf(YELLOW
               "\n  No commits found!\n\n"
               RESET);
        return 0;
    }

    /* Search for our commit */
    Commit* current = head;
    Commit* found   = NULL;

    while (current != NULL) {
        if (current->id == commit_id) {
            found = current;
            break;
        }
        current = current->next;
    }

    if (!found) {
        printf(RED
               "\n  ✗ Commit #%d not found!\n\n"
               RESET, commit_id);
        free_commit_list(head);
        return -1;
    }

    /* Display detailed info */
    printf("\n");
    printf(CYAN
"  +==========================================+\n"
           RESET);
    printf(CYAN "  |" YELLOW
"              Commit Details                "
           CYAN "|\n" RESET);
    printf(CYAN
"  +==========================================+\n"
           RESET);

    printf(CYAN "  |" RESET
           " ID      : %-30d"
           CYAN "|\n" RESET, found->id);
    printf(CYAN "  |" RESET
           " Message : %-30s"
           CYAN "|\n" RESET, found->message);
    printf(CYAN "  |" RESET
           " Branch  : %-30s"
           CYAN "|\n" RESET, found->branch);
    printf(CYAN "  |" RESET
           " Time    : %-30s"
           CYAN "|\n" RESET, found->timestamp);

    if (found->parent_id == -1) {
        printf(CYAN "  |" RESET
               " Parent  : %-30s"
               CYAN "|\n" RESET,
               "(root - no parent)");
    } else {
        char parent_str[20];
        snprintf(parent_str, sizeof(parent_str),
                 "Commit #%d", found->parent_id);
        printf(CYAN "  |" RESET
               " Parent  : %-30s"
               CYAN "|\n" RESET, parent_str);
    }

    printf(CYAN
"  +==========================================+\n"
           RESET);

    printf(CYAN "  |" YELLOW
"                  Files                     "
           CYAN "|\n" RESET);
    printf(CYAN
"  +==========================================+\n"
           RESET);

    for (int i = 0; i < found->file_count; i++) {
        printf(CYAN "  |" RESET
               " %-2d. %-36s"
               CYAN "|\n" RESET,
               i + 1,
               found->filenames[i]);

        /* Show file content from blob */
        char blob_path[MAX_PATH];
        snprintf(blob_path, sizeof(blob_path),
                 "%s/%lu.blob",
                 OBJECTS_DIR,
                 found->file_hashes[i]);

        if (file_exists(blob_path)) {
            char content[MAX_CONTENT];
            read_file(blob_path, content,
                      MAX_CONTENT);

            /* Count lines */
            int lines = 0;
            for (int j = 0; content[j]; j++) {
                if (content[j] == '\n') lines++;
            }
            if (strlen(content) > 0
                && content[strlen(content)-1]
                   != '\n') {
                lines++;
            }

            char info[50];
            snprintf(info, sizeof(info),
                     "   (%d lines, hash:%lu)",
                     lines,
                     found->file_hashes[i]);

            /* Truncate if too long */
            if (strlen(info) > 40) {
                info[40] = '\0';
            }

            printf(CYAN "  |" RESET
                   " %-40s"
                   CYAN "|\n" RESET, info);
        }
    }

    printf(CYAN
"  +==========================================+\n\n"
           RESET);

    free_commit_list(head);
    return 0;
}