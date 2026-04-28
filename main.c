
    /* rest of your main() code stays the same */
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

/* ADD THESE LINES AT THE TOP OF main() */
int main(int argc, char* argv[]){


    #ifdef _WIN32
    /*
     * SetConsoleOutputCP sets the Windows console's
     * character encoding to UTF-8 (code page 65001)
     *
     * This tells Windows CMD:
     * "Hey, we're sending UTF-8 text, display it correctly!"
     *
     * Without this: ─ shows as ΓöÇ
     * With this:    ─ shows as ─
     *
     * 65001 = UTF-8 code page number
     */
    SetConsoleOutputCP(65001);

    /*
     * Also enable virtual terminal processing
     * This makes ANSI color codes work properly on Windows!
     *
     * HANDLE → A reference to a Windows resource
     * GetStdHandle(STD_OUTPUT_HANDLE) → Get handle to console output
     * SetConsoleMode → Change how the console behaves
     * ENABLE_VIRTUAL_TERMINAL_PROCESSING → Allow ANSI escape codes
     */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    #endif 


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