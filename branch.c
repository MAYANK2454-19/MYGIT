/*
 * ============================================
 *          MYGIT - Branch Command
 *          "mygit branch"
 *          "mygit branch <name>"
 *          "mygit branch switch <name>"
 * ============================================
 *
 * PURPOSE:
 *   Create and manage parallel lines of development.
 *   Each branch is an independent version of history.
 *
 * DATA STRUCTURES USED:
 *   1. Linked List → List of all branches
 *   2. Files       → Each branch = one file in refs/
 *   3. Strings     → Branch names and commit IDs
 *
 * KEY INSIGHT:
 *   A branch is JUST a file containing a commit ID!
 *   refs/main contains "3"
 *   refs/dev  contains "4"
 *   That's the ENTIRE branch implementation!
 *
 * HEAD file:
 *   Contains the CURRENT branch name.
 *   "main" means we're on the main branch.
 *   When we commit, we update refs/[HEAD].
 */

#include "mygit.h"


/*
 * STRUCT: BranchNode
 * ──────────────────
 * A node in our linked list of branches.
 *
 * We use a linked list because:
 *   → We don't know how many branches exist
 *   → Easy to add new branches dynamically
 *   → Can traverse all branches easily
 */
typedef struct BranchNode {
    char name[MAX_BRANCH_NAME];  /* "main", "dev", etc. */
    int  commit_id;              /* Which commit it points to */
    struct BranchNode* next;     /* Next branch in list */
} BranchNode;


/*
 * FUNCTION: branch_exists
 * ────────────────────────
 * Checks if a branch already exists.
 *
 * HOW:
 *   Each branch = a file in .mygit/refs/
 *   If the file exists → branch exists!
 *
 * PARAMETERS:
 *   name → Branch name to check ("dev", "main", etc.)
 *
 * RETURNS:
 *   1 → Branch exists
 *   0 → Branch does not exist
 */
int branch_exists(const char* name) {

    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, name);

    return file_exists(ref_path);
}


/*
 * FUNCTION: get_branch_commit_id
 * ───────────────────────────────
 * Gets the commit ID that a branch points to.
 *
 * PARAMETERS:
 *   name → Branch name ("main", "dev", etc.)
 *
 * RETURNS:
 *   Commit ID (positive number)
 *   0 if branch doesn't exist or has no commits
 */
int get_branch_commit_id(const char* name) {

    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, name);

    char content[20];
    if (read_file(ref_path, content,
                  sizeof(content)) < 0) {
        return 0;
    }

    return atoi(content);
}


/*
 * FUNCTION: load_all_branches
 * ────────────────────────────
 * Reads ALL branches from the refs/ folder
 * and builds a LINKED LIST of BranchNodes.
 *
 * THIS IS TODAY'S LINKED LIST IMPLEMENTATION!
 *
 * HOW:
 *   Open the refs/ directory.
 *   Each FILE in refs/ = one branch.
 *   Read each file → get commit ID.
 *   Create a BranchNode → add to linked list.
 *
 * RETURNS:
 *   Head of the linked list (first branch)
 *   NULL if no branches found
 */
BranchNode* load_all_branches(void) {

    BranchNode* head = NULL;  /* Head of our linked list */
    BranchNode* tail = NULL;  /* Tail for easy appending */

    #ifdef _WIN32
    /*
     * Windows version:
     * Search for ALL files in refs/ folder
     * Each file = one branch
     */
    WIN32_FIND_DATA fd;
    char search_path[MAX_PATH];

    /*
     * Build search pattern: ".mygit/refs/*"
     * The * means "find all files"
     */
    snprintf(search_path, sizeof(search_path),
             "%s/*", REFS_DIR);

    HANDLE hFind = FindFirstFile(search_path, &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;  /* refs/ folder is empty */
    }

    do {
        /* Skip directories (. and ..) */
        if (fd.dwFileAttributes
            & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        /* Skip hidden files */
        if (fd.cFileName[0] == '.') {
            continue;
        }

        /*
         * This file IS a branch!
         * fd.cFileName = branch name (e.g. "main")
         *
         * Create a new BranchNode
         */
        BranchNode* node =
            (BranchNode*)malloc(sizeof(BranchNode));

        if (!node) continue;  /* malloc failed, skip */

        /* Copy branch name */
        strncpy(node->name,
                fd.cFileName,
                MAX_BRANCH_NAME - 1);
        node->name[MAX_BRANCH_NAME - 1] = '\0';

        /* Get commit ID from the file */
        node->commit_id =
            get_branch_commit_id(fd.cFileName);

        node->next = NULL;

        /*
         * Add to linked list
         *
         * If list is empty → this is head AND tail
         * If list has nodes → append to tail
         *
         * We append to TAIL to maintain order.
         * (Adding to head would reverse the order)
         */
        if (head == NULL) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }

    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);

    #else
    /*
     * Linux/Mac version:
     * Use opendir/readdir to list files in refs/
     */
    DIR* dir = opendir(REFS_DIR);
    if (!dir) return NULL;

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {

        if (entry->d_name[0] == '.') continue;

        BranchNode* node =
            (BranchNode*)malloc(sizeof(BranchNode));

        if (!node) continue;

        strncpy(node->name,
                entry->d_name,
                MAX_BRANCH_NAME - 1);
        node->name[MAX_BRANCH_NAME - 1] = '\0';

        node->commit_id =
            get_branch_commit_id(entry->d_name);

        node->next = NULL;

        if (head == NULL) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    closedir(dir);
    #endif

    return head;
}


/*
 * FUNCTION: free_branch_list
 * ───────────────────────────
 * Frees all memory used by the branch linked list.
 *
 * SAME PATTERN as free_commit_list from Day 4!
 * Save next pointer BEFORE freeing current node.
 */
void free_branch_list(BranchNode* head) {

    BranchNode* current = head;

    while (current != NULL) {
        BranchNode* next = current->next;
        free(current);
        current = next;
    }
}


/*
 * FUNCTION: mygit_list_branches
 * ──────────────────────────────
 * Shows all existing branches.
 * Marks the CURRENT branch with a star (*).
 *
 * OUTPUT EXAMPLE:
 *   Branches:
 *   * main  (commit #3)   ← current branch
 *     dev   (commit #2)
 *     test  (commit #1)
 */
int mygit_list_branches(void) {

    /* Get current branch name */
    char current[MAX_BRANCH_NAME];
    get_current_branch(current, sizeof(current));

    printf("\n");
    printf(CYAN "  Branches:\n" RESET);
    printf(CYAN "  ─────────────────────────────\n"
           RESET);

    /*
     * Load all branches into linked list
     */
    BranchNode* head = load_all_branches();

    if (head == NULL) {
        printf(YELLOW
               "  No branches found!\n\n"
               RESET);
        return 0;
    }

    /*
     * TRAVERSE THE LINKED LIST!
     * Classic pattern: current = head, follow ->next
     */
    BranchNode* node = head;
    int count = 0;

    while (node != NULL) {

        /*
         * Is this the CURRENT branch?
         * If yes → show with star and green color
         * If no  → show normally
         */
        int is_current =
            (strcmp(node->name, current) == 0);

        if (is_current) {
            printf(GREEN
                   "  * %-20s"
                   RESET,
                   node->name);
            printf(GREEN
                   " → commit #%d  (current)\n"
                   RESET,
                   node->commit_id);
        } else {
            printf("    %-20s", node->name);
            printf(" → commit #%d\n",
                   node->commit_id);
        }

        count++;
        node = node->next;  /* Move to next branch */
    }

    printf(CYAN
           "  ─────────────────────────────\n"
           RESET);
    printf("  Total: %d branch%s\n\n",
           count,
           count == 1 ? "" : "es");

    /* Free the linked list */
    free_branch_list(head);

    return 0;
}


/*
 * FUNCTION: mygit_branch (CREATE a new branch)
 * ─────────────────────────────────────────────
 * Creates a new branch pointing to the
 * CURRENT commit.
 *
 * HOW:
 *   1. Check branch doesn't already exist
 *   2. Find current commit ID
 *   3. Create refs/<name> file with commit ID
 *   4. Done! Branch created.
 *
 * NEW BRANCH points to same commit as current branch.
 * Like making a photocopy of your current state.
 *
 * PARAMETERS:
 *   branch_name → Name for the new branch
 */
int mygit_branch(const char* branch_name) {

    printf("\n");

    /*
     * ─────────────────────────────
     * STEP 1: Validate branch name
     * ─────────────────────────────
     *
     * Branch names should not contain spaces
     * or special characters.
     *
     * Simple check: no spaces allowed.
     */
    if (strchr(branch_name, ' ') != NULL) {
        printf(RED
               "  ✗ Branch name cannot "
               "contain spaces!\n\n"
               RESET);
        return -1;
    }

    /*
     * Check it's not too long
     */
    if (strlen(branch_name) >= MAX_BRANCH_NAME) {
        printf(RED
               "  ✗ Branch name too long! "
               "Max %d characters.\n\n"
               RESET,
               MAX_BRANCH_NAME - 1);
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 2: Check if already exists
     * ─────────────────────────────
     */
    if (branch_exists(branch_name)) {
        printf(RED
               "  ✗ Branch '%s' already exists!\n"
               RESET, branch_name);
        printf("  Use a different name.\n\n");
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 3: Get current commit ID
     * ─────────────────────────────
     *
     * New branch starts from WHERE WE ARE NOW.
     * Read the current branch's commit ID.
     */
    char current_branch[MAX_BRANCH_NAME];
    get_current_branch(current_branch,
                       sizeof(current_branch));

    int current_commit =
        get_branch_commit_id(current_branch);

    if (current_commit <= 0) {
        printf(RED
               "  ✗ No commits yet!\n"
               RESET);
        printf("  Make at least one commit "
               "before creating a branch.\n\n");
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 4: Create branch file
     * ─────────────────────────────
     *
     * Create .mygit/refs/<branch_name>
     * Write the current commit ID into it.
     *
     * This ONE file IS the branch!
     */
    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, branch_name);

    char id_str[20];
    snprintf(id_str, sizeof(id_str),
             "%d", current_commit);

    if (write_file(ref_path, id_str) != 0) {
        printf(RED
               "  ✗ Failed to create branch!\n\n"
               RESET);
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 5: Tell the user!
     * ─────────────────────────────
     */
    printf(GREEN
           "  Branch created!\n\n"
           RESET);

    printf(CYAN
           "  +--------------------------------+\n"
           RESET);
    printf(CYAN "  |" RESET
           " New branch  : %-16s"
           CYAN "|\n" RESET,
           branch_name);
    printf(CYAN "  |" RESET
           " From branch : %-16s"
           CYAN "|\n" RESET,
           current_branch);
    printf(CYAN "  |" RESET
           " At commit   : #%-15d"
           CYAN "|\n" RESET,
           current_commit);
    printf(CYAN
           "  +--------------------------------+\n"
           RESET);

    printf("\n");
    printf("  To switch to it: "
           YELLOW
           "mygit branch switch %s\n\n"
           RESET,
           branch_name);

    return 0;
}


/*
 * FUNCTION: switch_branch
 * ────────────────────────
 * Switches the current branch.
 *
 * HOW:
 *   1. Check target branch exists
 *   2. Check staging area is empty
 *      (warn if there are staged files)
 *   3. Get the target branch's commit ID
 *   4. Restore files from that commit
 *   5. Update HEAD to point to new branch
 *
 * PARAMETERS:
 *   branch_name → Branch to switch to
 */
int switch_branch(const char* branch_name) {

    printf("\n");

    /*
     * ─────────────────────────────
     * STEP 1: Check branch exists
     * ─────────────────────────────
     */
    if (!branch_exists(branch_name)) {
        printf(RED
               "  ✗ Branch '%s' does not exist!\n"
               RESET, branch_name);
        printf("  Create it first: "
               YELLOW
               "mygit branch %s\n\n"
               RESET,
               branch_name);

        /* Show available branches */
        printf("  Available branches:\n");
        mygit_list_branches();
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 2: Check not already on it
     * ─────────────────────────────
     */
    char current[MAX_BRANCH_NAME];
    get_current_branch(current, sizeof(current));

    if (strcmp(current, branch_name) == 0) {
        printf(YELLOW
               "  Already on branch '%s'!\n\n"
               RESET,
               branch_name);
        return 0;
    }

    /*
     * ─────────────────────────────
     * STEP 3: Get target commit ID
     * ─────────────────────────────
     */
    int target_commit =
        get_branch_commit_id(branch_name);

    int current_commit =
        get_branch_commit_id(current);

    /*
     * ─────────────────────────────
     * STEP 4: Restore files
     * ─────────────────────────────
     *
     * If target branch is at a DIFFERENT commit,
     * we need to restore those files.
     *
     * If both branches are at SAME commit,
     * no file changes needed (just update HEAD).
     */
    if (target_commit != current_commit
        && target_commit > 0) {

        printf(CYAN
               "  Switching files...\n"
               RESET);

        /*
         * We reuse find_commit_by_id from checkout.c!
         * This is why we declared it in mygit.h
         *
         * Find the commit on the target branch
         * and restore its files.
         */
        Commit target_state;

        if (find_commit_by_id(target_commit,
                              &target_state)) {

            /* Restore each file */
            for (int i = 0;
                 i < target_state.file_count;
                 i++) {

                char blob_path[MAX_PATH];
                snprintf(blob_path,
                         sizeof(blob_path),
                         "%s/%lu.blob",
                         OBJECTS_DIR,
                         target_state.file_hashes[i]);

                if (file_exists(blob_path)) {

                    char content[MAX_CONTENT];
                    read_file(blob_path,
                              content,
                              MAX_CONTENT);
                    write_file(
                        target_state.filenames[i],
                        content);

                    printf(GREEN "    + " RESET
                           "Restored: %s\n",
                           target_state.filenames[i]);
                }
            }
        }
    }

    /*
     * ─────────────────────────────
     * STEP 5: Update HEAD
     * ─────────────────────────────
     *
     * This is the KEY STEP!
     * Write new branch name to HEAD file.
     *
     * BEFORE: HEAD contains "main"
     * AFTER:  HEAD contains "dev"
     *
     * Now all future commits go to "dev" branch!
     */
    if (write_file(HEAD_FILE, branch_name) != 0) {
        printf(RED
               "  ✗ Failed to update HEAD!\n\n"
               RESET);
        return -1;
    }

    /*
     * ─────────────────────────────
     * STEP 6: Clear staging area
     * ─────────────────────────────
     *
     * Fresh start on new branch!
     */
    FILE* sf = fopen(STAGING_FILE, "w");
    if (sf) {
        fprintf(sf, "# MyGit Staging Area\n");
        fclose(sf);
    }

    /*
     * ─────────────────────────────
     * STEP 7: Tell the user!
     * ─────────────────────────────
     */
    printf("\n");
    printf(GREEN
           "  Switched to branch '%s'\n\n"
           RESET,
           branch_name);

    printf(CYAN
           "  +--------------------------------+\n"
           RESET);
    printf(CYAN "  |" RESET
           " Current branch: %-14s"
           CYAN "|\n" RESET,
           branch_name);
    printf(CYAN "  |" RESET
           " At commit    : #%-14d"
           CYAN "|\n" RESET,
           target_commit);
    printf(CYAN "  |" RESET
           " From branch  : %-14s"
           CYAN "|\n" RESET,
           current);
    printf(CYAN
           "  +--------------------------------+\n\n"
           RESET);

    return 0;
}


/*
 * ═══════════════════════════════════════════════
 * ENTRY POINT: mygit_branch
 * ═══════════════════════════════════════════════
 *
 * Called from main.c when user types:
 *   mygit branch          → list branches
 *   mygit branch dev      → create branch
 *   mygit branch switch x → switch branch
 *
 * Wait — main.c only passes ONE argument!
 * How do we handle "branch switch <name>"?
 *
 * We handle it HERE:
 *   If branch_name starts with "switch "
 *   → extract the actual name and switch
 *
 * BUT actually we need to update main.c too
 * to pass the third argument!
 */
int mygit_branch(const char* branch_name) {

    /*
     * Check if user is doing "branch switch <name>"
     *
     * strncmp(str, "switch", 6) checks if
     * branch_name STARTS WITH "switch"
     *
     * If yes → call switch_branch with the rest
     */
    if (strncmp(branch_name, "switch", 6) == 0) {

        /*
         * branch_name = "switch"
         * But we need the NAME after "switch"
         *
         * The actual branch name comes from
         * argv[3] in main.c
         *
         * We'll handle this in main.c update below.
         * For now, this catches the "switch" keyword.
         */
        printf(RED
               "  ✗ Usage: mygit branch switch"
               " <name>\n\n"
               RESET);
        return -1;
    }

    /* Otherwise create a new branch */
    return mygit_branch_create(branch_name);
}


/*
 * Rename our creation function to avoid
 * conflict with the entry point
 */
int mygit_branch_create(const char* branch_name) {

    printf("\n");

    if (strchr(branch_name, ' ') != NULL) {
        printf(RED
               "  ✗ Branch name cannot "
               "contain spaces!\n\n"
               RESET);
        return -1;
    }

    if (strlen(branch_name) >= MAX_BRANCH_NAME) {
        printf(RED
               "  ✗ Branch name too long!\n\n"
               RESET);
        return -1;
    }

    if (branch_exists(branch_name)) {
        printf(RED
               "  ✗ Branch '%s' already exists!\n\n"
               RESET, branch_name);
        return -1;
    }

    char current_branch[MAX_BRANCH_NAME];
    get_current_branch(current_branch,
                       sizeof(current_branch));

    int current_commit =
        get_branch_commit_id(current_branch);

    if (current_commit <= 0) {
        printf(RED
               "  ✗ No commits yet!\n\n"
               RESET);
        return -1;
    }

    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, branch_name);

    char id_str[20];
    snprintf(id_str, sizeof(id_str),
             "%d", current_commit);

    if (write_file(ref_path, id_str) != 0) {
        printf(RED
               "  ✗ Failed to create branch!\n\n"
               RESET);
        return -1;
    }

    printf(GREEN "  Branch created!\n\n" RESET);
    printf(CYAN  "  +--------------------------------+\n" RESET);
    printf(CYAN "  |" RESET " New branch  : %-16s" CYAN "|\n" RESET, branch_name);
    printf(CYAN "  |" RESET " From branch : %-16s" CYAN "|\n" RESET, current_branch);
    printf(CYAN "  |" RESET " At commit   : #%-15d" CYAN "|\n" RESET, current_commit);
    printf(CYAN  "  +--------------------------------+\n" RESET);
    printf("\n");
    printf("  To switch: " YELLOW "mygit branch switch %s\n\n" RESET, branch_name);

    return 0;
}