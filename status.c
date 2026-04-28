/*
 * ============================================
 *          MYGIT - Status Command
 *          "mygit status"
 * ============================================
 */

#include "mygit.h"

/* ════════════════════════════════════════════
 * STRUCT: StagingEntry
 * Holds one entry from staging.dat
 * "hello.txt|193485797"
 * ════════════════════════════════════════════
 */
typedef struct {
    char filename[MAX_FILENAME];
    unsigned long hash;
} StagingEntry;


/* ════════════════════════════════════════════
 * FUNCTION: should_ignore_file
 * Returns 1 = ignore this file
 * Returns 0 = show this file
 * ════════════════════════════════════════════
 */
int should_ignore_file(const char* filename) {

    /* List of exact filenames to ignore */
    const char* ignore_names[] = {
        "mygit.exe",
        "mygit",
        "Makefile",
        NULL   /* NULL marks the end of the list */
    };

    /* Check exact name matches */
    int i = 0;
    while (ignore_names[i] != NULL) {
        if (strcmp(filename, ignore_names[i]) == 0) {
            return 1;  /* Ignore it */
        }
        i++;
    }

    /*
     * Check file extension
     * strrchr finds the LAST dot in the filename
     * "add.c"     → ext points to ".c"
     * "hello.txt" → ext points to ".txt"
     * "Makefile"  → ext is NULL (no dot)
     */
    const char* ext = strrchr(filename, '.');

    if (ext != NULL) {

        /* List of extensions to ignore */
        const char* ignore_ext[] = {
            ".c",
            ".h",
            ".exe",
            ".o",
            ".obj",
            ".dat",
            ".blob",
            NULL   /* NULL marks the end */
        };

        int j = 0;
        while (ignore_ext[j] != NULL) {
            if (strcmp(ext, ignore_ext[j]) == 0) {
                return 1;  /* Ignore it */
            }
            j++;
        }
    }

    /* Don't ignore anything else */
    return 0;
}


/* ════════════════════════════════════════════
 * FUNCTION: read_staging_entries
 * Reads all entries from staging.dat
 * into an array of StagingEntry structs
 * ════════════════════════════════════════════
 */
int read_staging_entries(StagingEntry* entries, int max_count) {

    FILE* fp = fopen(STAGING_FILE, "r");
    if (!fp) return 0;

    int count = 0;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)
           && count < max_count) {

        /* Remove newline */
        line[strcspn(line, "\n\r")] = '\0';

        /* Skip comments and empty lines */
        if (line[0] == '#' || strlen(line) == 0) {
            continue;
        }

        /*
         * Parse "hello.txt|193485797"
         * Split at | to get filename and hash
         */
        char temp[MAX_LINE];
        strcpy(temp, line);

        char* fname = strtok(temp, "|");
        char* hstr  = strtok(NULL, "|");

        if (fname && hstr) {
            strncpy(entries[count].filename,
                    fname,
                    MAX_FILENAME - 1);
            entries[count].filename[MAX_FILENAME - 1] = '\0';
            entries[count].hash = strtoul(hstr, NULL, 10);
            count++;
        }
    }

    fclose(fp);
    return count;
}


/* ════════════════════════════════════════════
 * FUNCTION: find_in_staging
 * Searches for filename in staging entries
 * Returns index if found, -1 if not found
 * ════════════════════════════════════════════
 */
int find_in_staging(const char* filename,
                    StagingEntry* entries,
                    int count) {

    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}


/* ════════════════════════════════════════════
 * FUNCTION: get_committed_files
 * Gets file list from the LATEST commit
 * on the current branch
 * ════════════════════════════════════════════
 */
int get_committed_files(char filenames[][MAX_FILENAME],
                        unsigned long* hashes,
                        int max_count) {

    /* Get current branch */
    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    /* Build path to branch ref file */
    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path),
             "%s/%s", REFS_DIR, branch);

    /* Read latest commit ID */
    char id_str[20];
    if (read_file(ref_path, id_str, sizeof(id_str)) < 0) {
        return 0;
    }

    int target_id = atoi(id_str);
    if (target_id <= 0) return 0;

    /* Open commits file */
    FILE* fp = fopen(COMMITS_FILE, "r");
    if (!fp) return 0;

    int count      = 0;
    int found      = 0;
    int current_id = -1;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {

        line[strcspn(line, "\n\r")] = '\0';

        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        /* Check for new commit block */
        int rid;
        if (sscanf(line, "COMMIT:%d", &rid) == 1) {
            current_id = rid;
            found = (current_id == target_id);
            continue;
        }

        /* Skip if not our target commit */
        if (!found) continue;

        /* Parse FILES line */
        if (strncmp(line, "FILES:", 6) == 0) {
            char copy[MAX_LINE];
            strncpy(copy, line + 6, MAX_LINE - 1);
            copy[MAX_LINE - 1] = '\0';

            char* tok = strtok(copy, ",");
            while (tok && count < max_count) {
                strncpy(filenames[count],
                        tok,
                        MAX_FILENAME - 1);
                filenames[count][MAX_FILENAME - 1] = '\0';
                count++;
                tok = strtok(NULL, ",");
            }
        }

        /* Parse HASHES line */
        if (strncmp(line, "HASHES:", 7) == 0) {
            char copy[MAX_LINE];
            strncpy(copy, line + 7, MAX_LINE - 1);
            copy[MAX_LINE - 1] = '\0';

            char* tok = strtok(copy, ",");
            int idx = 0;
            while (tok && idx < max_count) {
                hashes[idx] = strtoul(tok, NULL, 10);
                idx++;
                tok = strtok(NULL, ",");
            }
        }

        /* End of target commit block */
        if (strcmp(line, "END") == 0 && found) {
            break;
        }
    }

    fclose(fp);
    return count;
}


/* ════════════════════════════════════════════
 * FUNCTION: is_in_list
 * Linear search in a 2D array of filenames
 * Returns 1 if found, 0 if not found
 * ════════════════════════════════════════════
 */
int is_in_list(const char* filename,
               char list[][MAX_FILENAME],
               int count) {

    for (int i = 0; i < count; i++) {
        if (strcmp(list[i], filename) == 0) {
            return 1;
        }
    }
    return 0;
}


/* ════════════════════════════════════════════
 * FUNCTION: list_directory_files
 * Lists all USER files in current directory.
 * Automatically ignores .c .h .exe etc.
 * ════════════════════════════════════════════
 */
int list_directory_files(char files[][MAX_FILENAME],
                         int max_count) {
    int count = 0;

#ifdef _WIN32

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile("*", &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }

    do {
        /*
         * FILTER 1: Skip directories
         * We only want regular files
         */
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        /*
         * FILTER 2: Skip hidden files
         * Files starting with '.' are hidden
         * Example: .mygit, .gitignore
         */
        if (fd.cFileName[0] == '.') {
            continue;
        }

        /*
         * FILTER 3: Skip project/system files
         * Uses our should_ignore_file function
         */
        if (should_ignore_file(fd.cFileName)) {
            continue;
        }

        /*
         * This file passed all filters!
         * Add it to our list.
         */
        if (count < max_count) {
            strncpy(files[count],
                    fd.cFileName,
                    MAX_FILENAME - 1);
            files[count][MAX_FILENAME - 1] = '\0';
            count++;
        }

    } while (FindNextFile(hFind, &fd)
             && count < max_count);

    FindClose(hFind);

#else

    DIR* dir = opendir(".");
    if (!dir) return 0;

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL
           && count < max_count) {

        if (entry->d_name[0] == '.') continue;

        if (should_ignore_file(entry->d_name)) continue;

        strncpy(files[count],
                entry->d_name,
                MAX_FILENAME - 1);
        files[count][MAX_FILENAME - 1] = '\0';
        count++;
    }

    closedir(dir);

#endif

    return count;
}


/* ════════════════════════════════════════════
 * MAIN FUNCTION: mygit_status
 * ════════════════════════════════════════════
 */
int mygit_status(void) {

    /* ── Step 1: Show branch ── */
    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    printf("\n");
    printf(CYAN "  On branch: " YELLOW "%s\n" RESET, branch);
    printf("\n");

    /* ── Step 2: Read staging area ── */
    StagingEntry entries[50];
    int staged_count = read_staging_entries(entries, 50);

    /* ── Step 3: Read last commit files ── */
    char committed_files[50][MAX_FILENAME];
    unsigned long committed_hashes[50];
    int committed_count = get_committed_files(
                              committed_files,
                              committed_hashes,
                              50);

    /* ── Step 4: List directory files ── */
    char dir_files[100][MAX_FILENAME];
    int dir_count = list_directory_files(dir_files, 100);

    /* ── Step 5: Categorize files ── */

    /* Files that are cleanly staged */
    char staged_clean[50][MAX_FILENAME];
    int  staged_clean_count = 0;

    /* Files that changed AFTER staging */
    char modified[50][MAX_FILENAME];
    int  modified_count = 0;

    /* Files never tracked */
    char untracked[100][MAX_FILENAME];
    int  untracked_count = 0;

    /*
     * Check each staged entry:
     * Compare current hash vs staged hash
     */
    for (int i = 0; i < staged_count; i++) {

        char* fname = entries[i].filename;
        unsigned long staged_hash = entries[i].hash;

        /* File deleted after staging */
        if (!file_exists(fname)) {
            strncpy(modified[modified_count],
                    fname, MAX_FILENAME - 1);
            modified[modified_count][MAX_FILENAME-1] = '\0';
            modified_count++;
            continue;
        }

        /* Read current content and hash it */
        char content[MAX_CONTENT];
        if (read_file(fname, content, MAX_CONTENT) < 0) {
            continue;
        }

        unsigned long cur_hash = hash_content(content);

        if (cur_hash == staged_hash) {
            /* Hash matches → cleanly staged */
            strncpy(staged_clean[staged_clean_count],
                    fname, MAX_FILENAME - 1);
            staged_clean[staged_clean_count][MAX_FILENAME-1] = '\0';
            staged_clean_count++;
        } else {
            /* Hash different → modified after staging */
            strncpy(modified[modified_count],
                    fname, MAX_FILENAME - 1);
            modified[modified_count][MAX_FILENAME-1] = '\0';
            modified_count++;
        }
    }

    /*
     * Check directory files for untracked:
     * Not in staging AND not in last commit
     */
    for (int i = 0; i < dir_count; i++) {

        char* fname = dir_files[i];

        int in_staging = (find_in_staging(
                            fname,
                            entries,
                            staged_count) != -1);

        int in_commit = is_in_list(
                            fname,
                            committed_files,
                            committed_count);

        if (!in_staging && !in_commit) {
            strncpy(untracked[untracked_count],
                    fname, MAX_FILENAME - 1);
            untracked[untracked_count][MAX_FILENAME-1] = '\0';
            untracked_count++;
        }
    }

    /* ── Step 6: Display results ── */

    /* STAGED */
    if (staged_clean_count > 0) {
        printf(GREEN
               "  Changes to be committed:\n"
               RESET);
        printf(GREEN
               "  (use \"mygit commit\" to save)\n\n"
               RESET);

        for (int i = 0; i < staged_clean_count; i++) {
            printf(GREEN "        staged:    " RESET
                   "%s\n", staged_clean[i]);
        }
        printf("\n");
    }

    /* MODIFIED */
    if (modified_count > 0) {
        printf(RED
               "  Changes not staged for commit:\n"
               RESET);
        printf(RED
               "  (use \"mygit add <file>\" to update)\n\n"
               RESET);

        for (int i = 0; i < modified_count; i++) {
            printf(RED "        modified:  " RESET
                   "%s\n", modified[i]);
        }
        printf("\n");
    }

    /* UNTRACKED */
    if (untracked_count > 0) {
        printf(YELLOW
               "  Untracked files:\n"
               RESET);
        printf(YELLOW
               "  (use \"mygit add <file>\" to track)\n\n"
               RESET);

        for (int i = 0; i < untracked_count; i++) {
            printf(YELLOW "        untracked: " RESET
                   "%s\n", untracked[i]);
        }
        printf("\n");
    }

    /* CLEAN */
    if (staged_clean_count == 0
        && modified_count  == 0
        && untracked_count == 0) {

        printf(GREEN
               "  Nothing to commit, "
               "working tree clean!\n\n"
               RESET);
    }

    /* SUMMARY LINE */
    if (staged_clean_count > 0
        || modified_count  > 0
        || untracked_count > 0) {

        printf(CYAN "  -------------------------------------\n"
               RESET);

        if (staged_clean_count > 0) {
            printf(GREEN "  %d file(s) staged  " RESET,
                   staged_clean_count);
        }
        if (modified_count > 0) {
            printf(RED "%d file(s) modified  " RESET,
                   modified_count);
        }
        if (untracked_count > 0) {
            printf(YELLOW "%d file(s) untracked" RESET,
                   untracked_count);
        }

        printf("\n\n");
    }

    return 0;
}