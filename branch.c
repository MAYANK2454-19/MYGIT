/*
 * ============================================
 *          MYGIT - Branch Command
 * ============================================
 */

#include "mygit.h"


/* ════════════════════════════════════════════
 * STRUCT: BranchNode (Linked List Node)
 * ════════════════════════════════════════════
 */
typedef struct BranchNode {
    char name[MAX_BRANCH_NAME];
    int  commit_id;
    struct BranchNode* next;
} BranchNode;


/* ════════════════════════════════════════════
 * FUNCTION: branch_exists
 * Returns 1 if branch exists, 0 if not
 * ════════════════════════════════════════════
 */
int branch_exists(const char* name) {

    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, name);

    return file_exists(ref_path);
}


/* ════════════════════════════════════════════
 * FUNCTION: get_branch_commit_id
 * Returns commit ID that branch points to
 * ════════════════════════════════════════════
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


/* ════════════════════════════════════════════
 * FUNCTION: load_all_branches
 * Reads refs/ folder and builds linked list
 * of all branches
 * ════════════════════════════════════════════
 */
BranchNode* load_all_branches(void) {

    BranchNode* head = NULL;
    BranchNode* tail = NULL;

#ifdef _WIN32

    WIN32_FIND_DATA fd;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path),
             "%s/*", REFS_DIR);

    HANDLE hFind = FindFirstFile(search_path, &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    do {
        if (fd.dwFileAttributes
            & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        if (fd.cFileName[0] == '.') {
            continue;
        }

        BranchNode* node =
            (BranchNode*)malloc(sizeof(BranchNode));

        if (!node) continue;

        strncpy(node->name,
                fd.cFileName,
                MAX_BRANCH_NAME - 1);
        node->name[MAX_BRANCH_NAME - 1] = '\0';

        node->commit_id =
            get_branch_commit_id(fd.cFileName);

        node->next = NULL;

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


/* ════════════════════════════════════════════
 * FUNCTION: free_branch_list
 * Frees all malloc'd branch nodes
 * ════════════════════════════════════════════
 */
void free_branch_list(BranchNode* head) {

    BranchNode* current = head;

    while (current != NULL) {
        BranchNode* next = current->next;
        free(current);
        current = next;
    }
}


/* ════════════════════════════════════════════
 * FUNCTION: mygit_list_branches
 * Shows all branches, marks current with *
 * ════════════════════════════════════════════
 */
int mygit_list_branches(void) {

    char current[MAX_BRANCH_NAME];
    get_current_branch(current, sizeof(current));

    printf("\n");
    printf(CYAN "  Branches:\n" RESET);
    printf(CYAN "  -----------------------------\n"
           RESET);

    BranchNode* head = load_all_branches();

    if (head == NULL) {
        printf(YELLOW
               "  No branches found!\n\n"
               RESET);
        return 0;
    }

    /* Traverse the linked list */
    BranchNode* node = head;
    int count = 0;

    while (node != NULL) {

        int is_current =
            (strcmp(node->name, current) == 0);

        if (is_current) {
            printf(GREEN "  * %-20s" RESET, node->name);
            printf(GREEN " -> commit #%d (current)\n"
                   RESET, node->commit_id);
        } else {
            printf("    %-20s", node->name);
            printf(" -> commit #%d\n",
                   node->commit_id);
        }

        count++;
        node = node->next;
    }

    printf(CYAN "  -----------------------------\n"
           RESET);
    printf("  Total: %d branch%s\n\n",
           count,
           count == 1 ? "" : "es");

    free_branch_list(head);
    return 0;
}


/* ════════════════════════════════════════════
 * FUNCTION: mygit_branch_create
 * Creates a new branch at current commit
 * ════════════════════════════════════════════
 */
int mygit_branch_create(const char* branch_name) {

    printf("\n");

    /* Validate name — no spaces */
    if (strchr(branch_name, ' ') != NULL) {
        printf(RED
               "  ✗ Branch name cannot "
               "contain spaces!\n\n"
               RESET);
        return -1;
    }

    /* Validate length */
    if (strlen(branch_name) >= MAX_BRANCH_NAME) {
        printf(RED
               "  ✗ Branch name too long! "
               "Max %d chars.\n\n"
               RESET,
               MAX_BRANCH_NAME - 1);
        return -1;
    }

    /* Check not already existing */
    if (branch_exists(branch_name)) {
        printf(RED
               "  ✗ Branch '%s' already exists!\n\n"
               RESET, branch_name);
        return -1;
    }

    /* Get current branch and commit */
    char current_branch[MAX_BRANCH_NAME];
    get_current_branch(current_branch,
                       sizeof(current_branch));

    int current_commit =
        get_branch_commit_id(current_branch);

    if (current_commit <= 0) {
        printf(RED
               "  ✗ No commits yet! Make at least\n"
               "    one commit before branching.\n\n"
               RESET);
        return -1;
    }

    /* Create the branch file in refs/ */
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

    /* Show success */
    printf(GREEN "  Branch created!\n\n" RESET);
    printf(CYAN  "  +--------------------------------+\n" RESET);
    printf(CYAN "  |" RESET " New branch  : %-16s" CYAN "|\n" RESET, branch_name);
    printf(CYAN "  |" RESET " From branch : %-16s" CYAN "|\n" RESET, current_branch);
    printf(CYAN "  |" RESET " At commit   : #%-15d" CYAN "|\n" RESET, current_commit);
    printf(CYAN  "  +--------------------------------+\n\n" RESET);
    printf("  To switch: "
           YELLOW "mygit branch switch %s\n\n"
           RESET, branch_name);

    return 0;
}


/* ════════════════════════════════════════════
 * FUNCTION: switch_branch
 * Switches to a different branch and
 * restores its files
 * ════════════════════════════════════════════
 */
int switch_branch(const char* branch_name) {

    printf("\n");

    /* Check branch exists */
    if (!branch_exists(branch_name)) {
        printf(RED
               "  ✗ Branch '%s' does not exist!\n"
               RESET, branch_name);
        printf("  Create it: "
               YELLOW "mygit branch %s\n\n"
               RESET, branch_name);
        return -1;
    }

    /* Check not already on it */
    char current[MAX_BRANCH_NAME];
    get_current_branch(current, sizeof(current));

    if (strcmp(current, branch_name) == 0) {
        printf(YELLOW
               "  Already on branch '%s'!\n\n"
               RESET, branch_name);
        return 0;
    }

    /* Get commit IDs */
    int target_commit =
        get_branch_commit_id(branch_name);
    int current_commit =
        get_branch_commit_id(current);

    /* Restore files if different commit */
    if (target_commit != current_commit
        && target_commit > 0) {

        printf(CYAN "  Switching files...\n" RESET);

        Commit target_state;

        if (find_commit_by_id(target_commit,
                              &target_state)) {

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

    /* Update HEAD to new branch */
    if (write_file(HEAD_FILE, branch_name) != 0) {
        printf(RED
               "  ✗ Failed to update HEAD!\n\n"
               RESET);
        return -1;
    }

    /* Clear staging area */
    FILE* sf = fopen(STAGING_FILE, "w");
    if (sf) {
        fprintf(sf, "# MyGit Staging Area\n");
        fclose(sf);
    }

    /* Show success */
    printf("\n");
    printf(GREEN "  Switched to branch '%s'\n\n"
           RESET, branch_name);
    printf(CYAN  "  +--------------------------------+\n" RESET);
    printf(CYAN "  |" RESET " Current branch: %-14s" CYAN "|\n" RESET, branch_name);
    printf(CYAN "  |" RESET " At commit     : #%-13d" CYAN "|\n" RESET, target_commit);
    printf(CYAN "  |" RESET " From branch   : %-14s" CYAN "|\n" RESET, current);
    printf(CYAN  "  +--------------------------------+\n\n" RESET);

    return 0;
}


/* ════════════════════════════════════════════
 * FUNCTION: mygit_branch
 * Entry point — called from main.c
 * Only used for "mygit branch <name>" (create)
 * Switch is handled separately in main.c
 * ════════════════════════════════════════════
 */
int mygit_branch(const char* branch_name) {
    return mygit_branch_create(branch_name);
}