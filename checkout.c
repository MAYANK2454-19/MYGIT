#include "mygit.h"

int find_commit_by_id(int target_id, Commit* commit) {

    FILE* fp = fopen(COMMITS_FILE, "r");

    if (!fp) {
        printf(RED "  ✗ No commits found!\n" RESET);
        return 0;
    }

    int found      = 0;
    int current_id = -1;
    int got_it     = 0;

    char line[MAX_LINE];

    commit->file_count = 0;
    commit->parent     = NULL;
    commit->next       = NULL;

    while (fgets(line, sizeof(line), fp)) {

        line[strcspn(line, "\n\r")] = '\0';

        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        int rid;
        if (sscanf(line, "COMMIT:%d", &rid) == 1) {
            if (found && got_it) break;
            current_id = rid;
            found = (current_id == target_id);
            if (found) {
                commit->id = target_id;
            }
            continue;
        }

        if (!found) continue;

        if (strncmp(line, "MSG:", 4) == 0) {
            strncpy(commit->message,
                    line + 4, MAX_MESSAGE - 1);
            commit->message[MAX_MESSAGE - 1] = '\0';
            continue;
        }

        if (strncmp(line, "TIME:", 5) == 0) {
            strncpy(commit->timestamp,
                    line + 5, 63);
            commit->timestamp[63] = '\0';
            continue;
        }

        if (strncmp(line, "BRANCH:", 7) == 0) {
            strncpy(commit->branch,
                    line + 7, MAX_BRANCH_NAME - 1);
            commit->branch[MAX_BRANCH_NAME - 1] = '\0';
            continue;
        }

        if (sscanf(line, "PARENT:%d",
                   &commit->parent_id) == 1) {
            continue;
        }

        if (strncmp(line, "FILES:", 6) == 0) {
            char copy[MAX_LINE];
            strncpy(copy, line + 6, MAX_LINE - 1);
            copy[MAX_LINE - 1] = '\0';

            char* tok = strtok(copy, ",");
            while (tok && commit->file_count < 10) {
                strncpy(
                    commit->filenames[commit->file_count],
                    tok, MAX_FILENAME - 1);
                commit->filenames[commit->file_count]
                    [MAX_FILENAME - 1] = '\0';
                commit->file_count++;
                tok = strtok(NULL, ",");
            }
            continue;
        }

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

        if (strcmp(line, "END") == 0 && found) {
            got_it = 1;
            break;
        }
    }

    fclose(fp);
    return got_it;
}


int restore_file_from_blob(const char* filename,
                           unsigned long hash) {

    char blob_path[MAX_PATH];
    snprintf(blob_path, sizeof(blob_path),
             "%s/%lu.blob", OBJECTS_DIR, hash);

    if (!file_exists(blob_path)) {
        printf(RED
               "    ✗ Blob not found for '%s'\n"
               RESET, filename);
        return 0;
    }

    char content[MAX_CONTENT];
    if (read_file(blob_path, content,
                  MAX_CONTENT) < 0) {
        printf(RED
               "    ✗ Could not read blob for '%s'\n"
               RESET, filename);
        return 0;
    }

    if (write_file(filename, content) != 0) {
        printf(RED
               "    ✗ Could not restore '%s'\n"
               RESET, filename);
        return 0;
    }

    return 1;
}


void show_available_commits(void) {

    FILE* fp = fopen(COMMITS_FILE, "r");

    if (!fp) {
        printf(YELLOW "  No commits found.\n" RESET);
        return;
    }

    printf(CYAN "\n  Available commits:\n" RESET);

    char line[MAX_LINE];
    int current_id = -1;
    char current_msg[MAX_MESSAGE]         = "";
    char current_branch[MAX_BRANCH_NAME]  = "";

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


void print_checkout_summary(Commit* commit,
                             int from_id) {

    printf("\n");
    printf(GREEN "  Checkout successful!\n\n" RESET);

    printf(CYAN
        "  ┌──────────────────────────────────────────┐\n"
        RESET);
    printf(CYAN "  │" RESET
        " Restored to Commit #%-21d"
        CYAN "│\n" RESET, commit->id);
    printf(CYAN "  │" RESET
        " Message : %-30s"
        CYAN "│\n" RESET, commit->message);
    printf(CYAN "  │" RESET
        " Branch  : %-30s"
        CYAN "│\n" RESET, commit->branch);
    printf(CYAN "  │" RESET
        " Time    : %-30s"
        CYAN "│\n" RESET, commit->timestamp);
    printf(CYAN "  │" RESET
        " From    : Commit #%-22d"
        CYAN "│\n" RESET, from_id);
    printf(CYAN
        "  └──────────────────────────────────────────┘\n"
        RESET);

    printf("\n");
    printf(GREEN "  Files restored (%d):\n" RESET,
           commit->file_count);

    for (int i = 0; i < commit->file_count; i++) {
        printf(GREEN "    ✓ " RESET
               "%s\n", commit->filenames[i]);
    }

    printf("\n");
}


int mygit_checkout(const char* target) {

    printf("\n");

    /* Parse target */
    int target_id;

    if (strcmp(target, "latest") == 0) {

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

        target_id = atoi(target);

        if (target_id <= 0) {
            printf(RED
                   "  ✗ Invalid commit ID: '%s'\n"
                   RESET, target);
            printf("  Use a number: "
                   "mygit checkout 2\n");
            show_available_commits();
            return -1;
        }
    }

    /* Check if already on this commit */
    int current_id = get_current_commit_id();

    if (current_id == target_id) {
        printf(YELLOW
               "  Already on commit #%d!\n\n"
               RESET, target_id);
        return 0;
    }

    /* Find the commit */
    Commit target_commit;

    if (!find_commit_by_id(target_id,
                           &target_commit)) {
        printf(RED
               "  ✗ Commit #%d not found!\n"
               RESET, target_id);
        show_available_commits();
        return -1;
    }

    /* Verify all blobs exist first */
    printf(CYAN "  Verifying blobs...\n" RESET);

    int all_blobs_ok = 1;

    for (int i = 0; i < target_commit.file_count; i++) {

        char blob_path[MAX_PATH];
        snprintf(blob_path, sizeof(blob_path),
                 "%s/%lu.blob",
                 OBJECTS_DIR,
                 target_commit.file_hashes[i]);

        if (file_exists(blob_path)) {
            printf(GREEN "    ✓ " RESET "%s\n",
                   target_commit.filenames[i]);
        } else {
            printf(RED "    ✗ " RESET
                   "%s BLOB MISSING!\n",
                   target_commit.filenames[i]);
            all_blobs_ok = 0;
        }
    }

    if (!all_blobs_ok) {
        printf(RED
               "\n  ✗ Checkout aborted!\n\n"
               RESET);
        return -1;
    }

    printf(GREEN "  All blobs verified!\n\n" RESET);

    /* Warn user */
    printf(YELLOW
           "  WARNING: This will overwrite "
           "your current files!\n"
           RESET);
    printf(YELLOW
           "  Uncommitted changes will be LOST!\n\n"
           RESET);
    printf("  Restore to Commit #%d (\"%s\")? "
           "[y/n]: ",
           target_id,
           target_commit.message);

    fflush(stdout);

    char answer = getchar();
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);

    printf("\n");

    if (answer != 'y' && answer != 'Y') {
        printf(YELLOW
               "  Checkout cancelled.\n\n"
               RESET);
        return 0;
    }

    /* Restore files */
    printf(CYAN "  Restoring files...\n" RESET);

    for (int i = 0; i < target_commit.file_count; i++) {

        char* fname = target_commit.filenames[i];
        unsigned long hash = target_commit.file_hashes[i];

        if (restore_file_from_blob(fname, hash)) {
            printf(GREEN "    ✓ Restored: " RESET
                   "%s\n", fname);
        }
    }

    /* Update branch reference */
    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, branch);

    char id_str[20];
    snprintf(id_str, sizeof(id_str),
             "%d", target_id);
    write_file(ref_path, id_str);

    /* Clear staging area */
    FILE* staging = fopen(STAGING_FILE, "w");
    if (staging) {
        fprintf(staging, "# MyGit Staging Area\n");
        fclose(staging);
    }

    /* Show summary */
    print_checkout_summary(&target_commit, current_id);

    return 0;
}