/*
 * ============================================
 *          MYGIT - Init Command
 *          "mygit init"
 * ============================================
 * 
 * WHAT IT DOES:
 * Creates the .mygit folder structure
 * Just like "git init" creates .git
 * 
 * FOLDER STRUCTURE CREATED:
 * .mygit/
 * ├── objects/       ← Will store file snapshots
 * ├── refs/          ← Will store branch pointers
 * ├── HEAD           ← Points to current branch
 * ├── commits.dat    ← Empty, ready for commits
 * └── staging.dat    ← Empty, ready for staged files
 */

#include "mygit.h"

int mygit_init(void) {
    
    // Check if already initialized
    if (directory_exists(MYGIT_DIR)) {
        printf(YELLOW "⚠  Repository already initialized!\n" RESET);
        return 0;
    }

    printf(CYAN "Initializing MyGit repository...\n" RESET);

    // Create directory structure
    // Each folder has a purpose (explain this to professor!)

    if (create_directory(MYGIT_DIR) != 0) {
        printf(RED "✗ Failed to create %s\n" RESET, MYGIT_DIR);
        return -1;
    }
    printf(GREEN "  ✓ Created %s/\n" RESET, MYGIT_DIR);

    if (create_directory(OBJECTS_DIR) != 0) {
        printf(RED "✗ Failed to create %s\n" RESET, OBJECTS_DIR);
        return -1;
    }
    printf(GREEN "  ✓ Created %s/\n" RESET, OBJECTS_DIR);

    if (create_directory(REFS_DIR) != 0) {
        printf(RED "✗ Failed to create %s\n" RESET, REFS_DIR);
        return -1;
    }
    printf(GREEN "  ✓ Created %s/\n" RESET, REFS_DIR);

    // Create HEAD file → points to "main" branch
    if (write_file(HEAD_FILE, "main") != 0) {
        printf(RED "✗ Failed to create HEAD\n" RESET);
        return -1;
    }
    printf(GREEN "  ✓ Created HEAD → main\n" RESET);

    // Create empty commits file
    FILE* fp = fopen(COMMITS_FILE, "w");
    if (fp) {
        fprintf(fp, "# MyGit Commit History\n");
        fclose(fp);
        printf(GREEN "  ✓ Created commits database\n" RESET);
    }

    // Create empty staging file
    fp = fopen(STAGING_FILE, "w");
    if (fp) {
        fprintf(fp, "# MyGit Staging Area\n");
        fclose(fp);
        printf(GREEN "  ✓ Created staging area\n" RESET);
    }

    // Create initial branch reference
    char ref_path[MAX_PATH];
    snprintf(ref_path, sizeof(ref_path), "%s/main", REFS_DIR);
    write_file(ref_path, "0");  // No commits yet

    printf("\n");
    printf(GREEN "✅ Initialized empty MyGit repository in .mygit/\n" RESET);
    printf(CYAN  "   Start tracking files with: mygit add <filename>\n" RESET);

    return 0;
}