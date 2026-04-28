/*
 * ============================================
 *          MYGIT - Status Command
 *          "mygit status"
 * ============================================
 *
 * PURPOSE:
 *   Show the current state of the working directory.
 *   Tell user: what's staged, modified, or untracked.
 *
 * DATA STRUCTURES USED:
 *   1. Arrays     → Store lists of filenames
 *   2. Hash values → Detect if files have changed
 *   3. Structs    → Hold staging area entries
 *
 * THREE FILE STATES:
 *   STAGED    → In staging.dat, ready to commit
 *   MODIFIED  → Staged before but file changed since
 *   UNTRACKED → File exists but never staged
 */

#include "mygit.h"


/*
 * STRUCT: StagingEntry
 * ─────────────────────
 * Holds ONE entry from the staging area.
 * We need this to compare hashes.
 *
 * Think of it as ONE LINE from staging.dat:
 *   "hello.txt|193485797"
 *   filename = "hello.txt"
 *   hash     = 193485797
 */
typedef struct {
    char filename[MAX_FILENAME];
    unsigned long hash;
} StagingEntry;


/*
 * FUNCTION: read_staging_entries
 * ──────────────────────────────
 * Reads ALL entries from staging.dat
 * into an array of StagingEntry structs.
 *
 * PARAMETERS:
 *   entries   → Array to fill with staging entries
 *   max_count → Maximum entries to read
 *
 * RETURNS:
 *   Number of entries read
 */
int read_staging_entries(StagingEntry* entries, int max_count) {

    FILE* fp = fopen(STAGING_FILE, "r");

    if (!fp) {
        return 0;  /* No staging file = nothing staged */
    }

    int count = 0;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp) && count < max_count) {

        /* Remove newline */
        line[strcspn(line, "\n\r")] = '\0';

        /* Skip comments and empty lines */
        if (line[0] == '#' || strlen(line) == 0) {
            continue;
        }

        /*
         * Parse "hello.txt|193485797"
         * Split at '|' to get filename and hash
         */
        char temp[MAX_LINE];
        strcpy(temp, line);

        char* filename = strtok(temp, "|");
        char* hash_str = strtok(NULL, "|");

        if (filename && hash_str) {
            strncpy(entries[count].filename, filename, MAX_FILENAME - 1);
            entries[count].filename[MAX_FILENAME - 1] = '\0';
            entries[count].hash = strtoul(hash_str, NULL, 10);
            count++;
        }
    }

    fclose(fp);
    return count;
}


/*
 * FUNCTION: is_file_staged
 * ────────────────────────
 * Checks if a specific filename is in our
 * staging entries array.
 *
 * PARAMETERS:
 *   filename  → File to look for
 *   entries   → Array of staging entries
 *   count     → How many entries in array
 *
 * RETURNS:
 *   Index of the entry if found (0, 1, 2...)
 *   -1 if NOT found
 *
 * WHY return index instead of 0/1?
 *   So the caller can ACCESS the entry's hash!
 *   Just knowing "it exists" isn't enough.
 *   We need the hash to check if it's MODIFIED.
 */
int is_file_staged(const char* filename,
                   StagingEntry* entries,
                   int count) {

    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].filename, filename) == 0) {
            return i;  /* Found! Return the INDEX */
        }
    }

    return -1;  /* Not found */
}


/*
 * FUNCTION: get_committed_files
 * ─────────────────────────────
 * Gets the list of files in the LAST commit
 * on the current branch.
 *
 * WHY?
 *   We need to know what was committed before.
 *   Files that were committed but NOT in staging
 *   are "not staged for commit" (tracked but clean).
 *
 * HOW:
 *   1. Get current branch
 *   2. Read refs/branch to find latest commit ID
 *   3. Search commits.dat for that commit
 *   4. Extract its file list
 *
 * PARAMETERS:
 *   filenames  → Array to fill with committed filenames
 *   hashes     → Array to fill with their hashes
 *   max_count  → Maximum files to read
 *
 * RETURNS:
 *   Number of files in last commit
 */
int get_committed_files(char filenames[][MAX_FILENAME],
                        unsigned long* hashes,
                        int max_count) {

    /*
     * Get current branch name
     */
    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    /*
     * Get latest commit ID on this branch
     */
    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path), "%s/%s", REFS_DIR, branch);

    char id_str[20];
    if (read_file(ref_path, id_str, sizeof(id_str)) < 0) {
        return 0;  /* No commits yet */
    }

    int target_id = atoi(id_str);
    if (target_id <= 0) {
        return 0;  /* No commits yet */
    }

    /*
     * Open commits.dat and find the commit with this ID
     */
    FILE* fp = fopen(COMMITS_FILE, "r");
    if (!fp) {
        return 0;
    }

    int count = 0;
    char line[MAX_LINE];
    int found_commit = 0;  /* Are we inside the right commit block? */
    int current_id = -1;   /* ID of the commit block we're reading */

    while (fgets(line, sizeof(line), fp)) {

        line[strcspn(line, "\n\r")] = '\0';

        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        /*
         * Check if this is a new commit block
         * "COMMIT:3" → current_id = 3
         */
        int read_id;
        if (sscanf(line, "COMMIT:%d", &read_id) == 1) {
            current_id = read_id;
            /* Are we in the commit we're looking for? */
            found_commit = (current_id == target_id);
            continue;
        }

        /* Skip this block if not our target commit */
        if (!found_commit) {
            continue;
        }

        /* We're in the right commit! Parse file list */
        if (strncmp(line, "FILES:", 6) == 0 && count == 0) {

            char files_copy[MAX_LINE];
            strncpy(files_copy, line + 6, MAX_LINE - 1);
            files_copy[MAX_LINE - 1] = '\0';

            char* token = strtok(files_copy, ",");
            while (token != NULL && count < max_count) {
                strncpy(filenames[count], token, MAX_FILENAME - 1);
                filenames[count][MAX_FILENAME - 1] = '\0';
                count++;
                token = strtok(NULL, ",");
            }
        }

        /* Parse hash list */
        if (strncmp(line, "HASHES:", 7) == 0) {

            char hashes_copy[MAX_LINE];
            strncpy(hashes_copy, line + 7, MAX_LINE - 1);
            hashes_copy[MAX_LINE - 1] = '\0';

            char* token = strtok(hashes_copy, ",");
            int idx = 0;
            while (token != NULL && idx < max_count) {
                hashes[idx] = strtoul(token, NULL, 10);
                idx++;
                token = strtok(NULL, ",");
            }
        }

        /* End of this commit block */
        if (strcmp(line, "END") == 0 && found_commit) {
            break;  /* Found what we needed, stop reading */
        }
    }

    fclose(fp);
    return count;
}


/*
 * FUNCTION: list_directory_files
 * ──────────────────────────────
 * Lists ALL regular files in the current directory.
 * Skips hidden files (starting with '.') and folders.
 *
 * This is the MOST PLATFORM-DEPENDENT function.
 * Windows and Linux have different APIs for this!
 *
 * PARAMETERS:
 *   files     → Array to fill with filenames
 *   max_count → Maximum files to return
 *
 * RETURNS:
 *   Number of files found
 */
int list_directory_files(char files[][MAX_FILENAME], int max_count) {

    int count = 0;

    #ifdef _WIN32
    /*
     * WINDOWS VERSION
     * ───────────────
     * Windows uses FindFirstFile / FindNextFile API
     *
     * WIN32_FIND_DATA → Struct holding file information
     * HANDLE          → Like a FILE* but for directory searching
     *
     * Process:
     *   1. FindFirstFile("*") → Start searching, get first file
     *   2. FindNextFile()     → Get next file (repeat)
     *   3. FindClose()        → Stop searching (like fclose)
     */
    WIN32_FIND_DATA findData;

    /*
     * "*" means "find ALL files"
     * FindFirstFile returns info about the first match
     * and a HANDLE we use for subsequent calls
     */
    HANDLE hFind = FindFirstFile("*", &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;  /* Directory is empty or error */
    }

    do {
        /*
         * findData.cFileName = name of current file
         * findData.dwFileAttributes = file properties (is it a folder? hidden?)
         *
         * FILE_ATTRIBUTE_DIRECTORY → It's a folder, skip it!
         * We only want REGULAR FILES.
         *
         * & is bitwise AND → checks if the DIRECTORY bit is set
         */
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;  /* Skip folders */
        }

        /* Skip hidden files (starting with '.') */
        if (findData.cFileName[0] == '.') {
            continue;
        }

        /* Skip our own executable */
        if (strcmp(findData.cFileName, "mygit.exe") == 0) {
            continue;
        }

        /* Skip .c source files (they're our code, not user data) */
        /* Actually let's include them — user might track .c files! */

        if (count < max_count) {
            strncpy(files[count], findData.cFileName, MAX_FILENAME - 1);
            files[count][MAX_FILENAME - 1] = '\0';
            count++;
        }

    } while (FindNextFile(hFind, &findData) && count < max_count);
    /*
     * FindNextFile returns:
     *   Non-zero → found another file, continue
     *   0        → no more files, stop the loop
     *
     * The do-while loop:
     *   Runs ONCE first (the "do" part)
     *   Then checks condition (FindNextFile)
     *   If condition true → loop again
     *   If condition false → stop
     */

    FindClose(hFind);  /* Like fclose but for directory handles */

    #else
    /*
     * LINUX/MAC VERSION
     * ─────────────────
     * Uses opendir/readdir/closedir from <dirent.h>
     *
     * DIR*    → Like FILE* but for directories
     * dirent* → Struct holding one file's info
     */
    DIR* dir = opendir(".");  /* "." means current directory */

    if (!dir) {
        return 0;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL && count < max_count) {

        /* Skip hidden files and folders (. and ..) */
        if (entry->d_name[0] == '.') {
            continue;
        }

        /* Skip our executable */
        if (strcmp(entry->d_name, "mygit") == 0) {
            continue;
        }

        strncpy(files[count], entry->d_name, MAX_FILENAME - 1);
        files[count][MAX_FILENAME - 1] = '\0';
        count++;
    }

    closedir(dir);

    #endif

    return count;
}


/*
 * FUNCTION: is_in_list
 * ─────────────────────
 * Checks if a filename exists in a 2D array of filenames.
 *
 * PARAMETERS:
 *   filename → What to look for
 *   list     → 2D array of filenames
 *   count    → How many filenames in list
 *
 * RETURNS:
 *   1 → Found
 *   0 → Not found
 *
 * This is a LINEAR SEARCH — O(n) time complexity
 * We check each element one by one until we find it.
 */
int is_in_list(const char* filename,
               char list[][MAX_FILENAME],
               int count) {

    for (int i = 0; i < count; i++) {
        if (strcmp(list[i], filename) == 0) {
            return 1;  /* Found! */
        }
    }
    return 0;  /* Not found */
}


/*
 * ═══════════════════════════════════════════════
 * MAIN FUNCTION: mygit_status
 * ═══════════════════════════════════════════════
 *
 * This is what runs when user types: mygit status
 *
 * ALGORITHM:
 *   1. Get current branch
 *   2. Read staging area
 *   3. For each staged file:
 *      → Hash the CURRENT version
 *      → Compare with staged hash
 *      → If different → MODIFIED
 *      → If same → cleanly staged
 *   4. List all files in directory
 *   5. For each directory file:
 *      → If NOT staged → UNTRACKED
 */
int mygit_status(void) {

    /*
     * ──────────────────────────────────────────
     * STEP 1: Show current branch
     * ──────────────────────────────────────────
     */
    char branch[MAX_BRANCH_NAME];
    get_current_branch(branch, sizeof(branch));

    printf("\n");
    printf(CYAN "  On branch: " YELLOW "%s\n" RESET, branch);
    printf("\n");

    /*
     * ──────────────────────────────────────────
     * STEP 2: Read staging area into array
     * ──────────────────────────────────────────
     *
     * StagingEntry entries[50] → Array of max 50 staged files
     * staged_count             → How many files are actually staged
     */
    StagingEntry entries[50];
    int staged_count = read_staging_entries(entries, 50);

    /*
     * ──────────────────────────────────────────
     * STEP 3: Get files from last commit
     * ──────────────────────────────────────────
     *
     * We need this to identify files that are
     * "tracked" (committed before) vs "untracked"
     */
    char committed_files[50][MAX_FILENAME];
    unsigned long committed_hashes[50];
    int committed_count = get_committed_files(committed_files,
                                               committed_hashes,
                                               50);

    /*
     * ──────────────────────────────────────────
     * STEP 4: List all files in current directory
     * ──────────────────────────────────────────
     */
    char dir_files[100][MAX_FILENAME];
    int dir_count = list_directory_files(dir_files, 100);

    /*
     * ──────────────────────────────────────────
     * STEP 5: Find STAGED and MODIFIED files
     * ──────────────────────────────────────────
     *
     * For each staged entry:
     *   → Check if the actual file changed
     *   → If yes: MODIFIED (needs re-staging)
     *   → If no:  cleanly STAGED
     */

    /* Arrays to collect staged and modified files */
    char staged_files[50][MAX_FILENAME];
    int staged_file_count = 0;

    char modified_files[50][MAX_FILENAME];
    int modified_count = 0;

    for (int i = 0; i < staged_count; i++) {

        char* filename = entries[i].filename;
        unsigned long staged_hash = entries[i].hash;

        /*
         * Does the file still exist on disk?
         */
        if (!file_exists(filename)) {
            /*
             * File was staged but then DELETED!
             * This is a special case.
             * Real git shows "deleted" for this.
             * We'll show it as modified for simplicity.
             */
            strncpy(modified_files[modified_count],
                    filename,
                    MAX_FILENAME - 1);
            modified_files[modified_count][MAX_FILENAME - 1] = '\0';
            modified_count++;
            continue;
        }

        /*
         * Read the CURRENT file content and hash it
         *
         * This is how we detect changes:
         *   staged_hash  = hash when user ran "mygit add"
         *   current_hash = hash of file RIGHT NOW
         *
         *   staged_hash == current_hash → no changes → STAGED ✅
         *   staged_hash != current_hash → file changed → MODIFIED ✏️
         */
        char content[MAX_CONTENT];
        if (read_file(filename, content, MAX_CONTENT) < 0) {
            continue;
        }

        unsigned long current_hash = hash_content(content);

        if (current_hash == staged_hash) {
            /*
             * Hash matches → file hasn't changed since staging
             * It's cleanly STAGED ✅
             */
            strncpy(staged_files[staged_file_count],
                    filename,
                    MAX_FILENAME - 1);
            staged_files[staged_file_count][MAX_FILENAME - 1] = '\0';
            staged_file_count++;

        } else {
            /*
             * Hash is DIFFERENT → file was modified after staging
             * It needs to be re-added! ✏️
             */
            strncpy(modified_files[modified_count],
                    filename,
                    MAX_FILENAME - 1);
            modified_files[modified_count][MAX_FILENAME - 1] = '\0';
            modified_count++;
        }
    }

    /*
     * ──────────────────────────────────────────
     * STEP 6: Find UNTRACKED files
     * ──────────────────────────────────────────
     *
     * An untracked file is one that:
     *   → Exists in the directory
     *   → Is NOT in staging area
     *   → Is NOT in the last commit
     *
     * We check ALL files in directory
     * and filter out the ones we know about.
     */
    char untracked_files[100][MAX_FILENAME];
    int untracked_count = 0;

    for (int i = 0; i < dir_count; i++) {

        char* filename = dir_files[i];

        /* Is it in staging area? */
        int in_staging = (is_file_staged(filename, entries,
                                          staged_count) != -1);

        /* Is it in the last commit? */
        int in_commit = is_in_list(filename,
                                    committed_files,
                                    committed_count);

        /*
         * If it's in NEITHER staging NOR last commit
         * → It's UNTRACKED
         *
         * || means OR: "if not in staging OR not in commit"
         * We use ! (NOT) and && (AND):
         *   !in_staging && !in_commit
         *   "NOT staged AND NOT committed"
         *   Both must be true for it to be untracked!
         */
        if (!in_staging && !in_commit) {
            strncpy(untracked_files[untracked_count],
                    filename,
                    MAX_FILENAME - 1);
            untracked_files[untracked_count][MAX_FILENAME - 1] = '\0';
            untracked_count++;
        }
    }

    /*
     * ──────────────────────────────────────────
     * STEP 7: Display the results!
     * ──────────────────────────────────────────
     *
     * Show a clean, color-coded summary.
     */

    /* ── STAGED FILES ── */
    if (staged_file_count > 0) {
        printf(GREEN "  Changes to be committed:\n" RESET);
        printf(GREEN "  (use \"mygit commit\" to save these)\n" RESET);
        printf("\n");

        for (int i = 0; i < staged_file_count; i++) {
            printf(GREEN "        staged:   %s\n" RESET,
                   staged_files[i]);
        }
        printf("\n");
    }

    /* ── MODIFIED FILES ── */
    if (modified_count > 0) {
        printf(RED "  Changes not staged for commit:\n" RESET);
        printf(RED "  (use \"mygit add <file>\" to update)\n" RESET);
        printf("\n");

        for (int i = 0; i < modified_count; i++) {
            printf(RED "        modified: %s\n" RESET,
                   modified_files[i]);
        }
        printf("\n");
    }

    /* ── UNTRACKED FILES ── */
    if (untracked_count > 0) {
        printf(YELLOW "  Untracked files:\n" RESET);
        printf(YELLOW "  (use \"mygit add <file>\" to track)\n" RESET);
        printf("\n");

        for (int i = 0; i < untracked_count; i++) {
            printf(YELLOW "        untracked: %s\n" RESET,
                   untracked_files[i]);
        }
        printf("\n");
    }

    /* ── CLEAN STATE ── */
    if (staged_file_count == 0 &&
        modified_count == 0 &&
        untracked_count == 0) {

        printf(GREEN "  ✅ Nothing to commit, working tree clean!\n" RESET);
        printf("\n");
    }

    /*
     * ──────────────────────────────────────────
     * STEP 8: Show helpful summary line
     * ──────────────────────────────────────────
     *
     * Like real git's summary at the bottom
     */
    if (staged_file_count > 0 || modified_count > 0) {

       printf(CYAN "  -------------------------------------\n" RESET);

        if (staged_file_count > 0) {
            printf(GREEN "  %d file(s) staged" RESET, staged_file_count);
            if (modified_count > 0 || untracked_count > 0) {
                printf(", ");
            }
        }

        if (modified_count > 0) {
            printf(RED "%d file(s) modified" RESET, modified_count);
            if (untracked_count > 0) {
                printf(", ");
            }
        }

        if (untracked_count > 0) {
            printf(YELLOW "%d file(s) untracked" RESET, untracked_count);
        }

        printf("\n\n");
    }

    return 0;
}