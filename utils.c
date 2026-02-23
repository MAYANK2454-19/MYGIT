/*
 * ============================================
 *          MYGIT - Utility Functions
 *          The toolkit that powers everything
 * ============================================
 */

#include "mygit.h"

/*
 * HASH FUNCTION — djb2 algorithm
 * ───────────────────────────────
 * Takes a string → Returns a unique number
 * 
 * WHY DO WE NEED THIS?
 * → To detect if a file has CHANGED
 * → Same content = Same hash (always)
 * → Different content = Different hash (almost always)
 * 
 * HOW IT WORKS:
 * → Start with a magic number (5381)
 * → For each character: hash = hash * 33 + character
 * → This spreads values evenly (fewer collisions)
 * 
 * TIME COMPLEXITY: O(n) where n = length of content
 * SPACE COMPLEXITY: O(1)
 * 
 * REAL WORLD: Git uses SHA-1 (much more complex)
 *             We use djb2 (simple but effective for our scale)
 */
unsigned long hash_content(const char* content) {
    unsigned long hash = 5381;
    int c;

    while ((c = *content++)) {
        hash = ((hash << 5) + hash) + c;   // hash * 33 + c
    }

    return hash;
}

/*
 * CHECK IF FILE EXISTS
 */
int file_exists(const char* path) {
    FILE* fp = fopen(path, "r");
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

/*
 * CHECK IF DIRECTORY EXISTS
 */
int directory_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

/*
 * CREATE DIRECTORY
 * Works on both Linux/Mac and Windows
 */
int create_directory(const char* path) {
    #ifdef _WIN32
        return mkdir(path);           // Windows
    #else
        return mkdir(path, 0777);     // Linux/Mac
    #endif
}

/*
 * READ ENTIRE FILE INTO BUFFER
 * Returns: number of bytes read, -1 on error
 */
int read_file(const char* path, char* buffer, int max_size) {
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    int bytes_read = fread(buffer, 1, max_size - 1, fp);
    buffer[bytes_read] = '\0';
    fclose(fp);

    return bytes_read;
}

/*
 * WRITE STRING TO FILE
 * Returns: 0 on success, -1 on error
 */
int write_file(const char* path, const char* content) {
    FILE* fp = fopen(path, "w");
    if (!fp) {
        return -1;
    }

    fprintf(fp, "%s", content);
    fclose(fp);

    return 0;
}

/*
 * GET CURRENT TIMESTAMP
 * Format: "2025-01-15 14:30:45"
 */
void get_timestamp(char* buffer, int size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

/*
 * GET NEXT COMMIT ID
 * Reads commits file and returns max_id + 1
 */
int get_next_commit_id(void) {
    FILE* fp = fopen(COMMITS_FILE, "r");
    if (!fp) return 1;  // first commit

    int max_id = 0;
    int id;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "COMMIT:%d", &id) == 1) {
            if (id > max_id) max_id = id;
        }
    }

    fclose(fp);
    return max_id + 1;
}

/*
 * GET CURRENT BRANCH NAME
 * Reads from HEAD file
 */
char* get_current_branch(char* buffer, int size) {
    if (read_file(HEAD_FILE, buffer, size) < 0) {
        strcpy(buffer, "main");
    }

    // Remove newline if present
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return buffer;
}

/*
 * PRINT COOL BANNER
 */
void print_banner(void) {
    printf(CYAN);
    printf("╔══════════════════════════════════════╗\n");
    printf("║        ");
    printf(YELLOW);
    printf("⚡ MyGit v1.0 ⚡");
    printf(CYAN);
    printf("             ║\n");
    printf("║   Mini Version Control System in C   ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf(RESET);
}

/*
 * PRINT HELP / USAGE
 */
void print_help(void) {
    printf("\n");
    printf(YELLOW "USAGE:" RESET "\n");
    printf("  mygit <command> [arguments]\n\n");
    printf(YELLOW "COMMANDS:" RESET "\n");
    printf(GREEN "  init              " RESET "Initialize a new repository\n");
    printf(GREEN "  add <file>        " RESET "Stage a file for commit\n");
    printf(GREEN "  commit \"message\"  " RESET "Save a snapshot\n");
    printf(GREEN "  log               " RESET "Show commit history\n");
    printf(GREEN "  status            " RESET "Show working tree status\n");
    printf(GREEN "  diff <file>       " RESET "Show changes in a file\n");
    printf(GREEN "  checkout <id>     " RESET "Restore a previous commit\n");
    printf(GREEN "  branch <name>     " RESET "Create a new branch\n");
    printf(GREEN "  branch            " RESET "List all branches\n");
    printf(GREEN "  help              " RESET "Show this help message\n");
    printf("\n");
}