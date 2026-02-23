/*
 * ============================================
 *          MYGIT - Main Entry Point
 *          Parses commands like real Git
 * ============================================
 * 
 * HOW IT WORKS:
 * User types: ./mygit commit "first commit"
 *                      ^^^^^^ ^^^^^^^^^^^^^^
 *                      argv[1]   argv[2]
 * 
 * We check argv[1] and call the right function.
 * This is how ALL command-line tools work!
 */

#include "mygit.h"

int main(int argc, char* argv[]) {

    // No command given → show help
    if (argc < 2) {
        print_banner();
        print_help();
        return 0;
    }

    // Parse the command (argv[1])
    char* command = argv[1];

    /* ─── INIT ─── */
    if (strcmp(command, "init") == 0) {
        return mygit_init();
    }

    /*
     * For all other commands, repository must exist
     * (just like git gives error if you haven't done git init)
     */
    if (!directory_exists(MYGIT_DIR)) {
        printf(RED "✗ Not a MyGit repository!\n" RESET);
        printf("  Run " YELLOW "mygit init" RESET " first.\n");
        return 1;
    }

    /* ─── ADD ─── */
    if (strcmp(command, "add") == 0) {
        if (argc < 3) {
            printf(RED "✗ Please specify a file: mygit add <filename>\n" RESET);
            return 1;
        }
        return mygit_add(argv[2]);
    }

    /* ─── COMMIT ─── */
    else if (strcmp(command, "commit") == 0) {
        if (argc < 3) {
            printf(RED "✗ Please provide a message: mygit commit \"your message\"\n" RESET);
            return 1;
        }
        return mygit_commit(argv[2]);
    }

    /* ─── LOG ─── */
    else if (strcmp(command, "log") == 0) {
        return mygit_log();
    }

    /* ─── STATUS ─── */
    else if (strcmp(command, "status") == 0) {
        return mygit_status();
    }

    /* ─── DIFF ─── */
    else if (strcmp(command, "diff") == 0) {
        if (argc < 3) {
            printf(RED "✗ Please specify a file: mygit diff <filename>\n" RESET);
            return 1;
        }
        return mygit_diff(argv[2]);
    }

    /* ─── CHECKOUT ─── */
    else if (strcmp(command, "checkout") == 0) {
        if (argc < 3) {
            printf(RED "✗ Please specify commit ID or branch: mygit checkout <target>\n" RESET);
            return 1;
        }
        return mygit_checkout(argv[2]);
    }

    /* ─── BRANCH ─── */
    else if (strcmp(command, "branch") == 0) {
        if (argc < 3) {
            return mygit_list_branches();  // no arg → list branches
        }
        return mygit_branch(argv[2]);      // with arg → create branch
    }

    /* ─── HELP ─── */
    else if (strcmp(command, "help") == 0) {
        print_banner();
        print_help();
        return 0;
    }

    /* ─── UNKNOWN COMMAND ─── */
    else {
        printf(RED "✗ Unknown command: '%s'\n" RESET, command);
        printf("  Run " YELLOW "mygit help" RESET " to see available commands.\n");
        return 1;
    }

    return 0;
}