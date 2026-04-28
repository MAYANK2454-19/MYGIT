#include "mygit.h"

int main(int argc, char* argv[]) {

    #ifdef _WIN32
    /*
     * Fix Windows console to show colors and
     * unicode characters correctly
     */
    SetConsoleOutputCP(65001);

    /*
     * ENABLE_VIRTUAL_TERMINAL_PROCESSING might not
     * be defined in older MinGW versions.
     * We define it manually here!
     *
     * This value (0x0004) is from Microsoft docs.
     * It tells Windows console to process
     * ANSI escape codes (our color codes!)
     */
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    #endif

    /* No command given → show help */
    if (argc < 2) {
        print_banner();
        print_help();
        return 0;
    }

    char* command = argv[1];

    /* init does not need .mygit to exist */
    if (strcmp(command, "init") == 0) {
        return mygit_init();
    }

    /* All other commands need .mygit to exist */
    if (!directory_exists(MYGIT_DIR)) {
        printf(RED
               "\n  ✗ Not a MyGit repository!\n"
               RESET);
        printf("  Run "
               YELLOW "mygit init"
               RESET " first.\n\n");
        return 1;
    }

    /* ── ADD ── */
    if (strcmp(command, "add") == 0) {
        if (argc < 3) {
            printf(RED
                   "  ✗ Usage: mygit add <filename>\n"
                   RESET);
            return 1;
        }
        return mygit_add(argv[2]);
    }

    /* ── COMMIT ── */
    else if (strcmp(command, "commit") == 0) {
        if (argc < 3) {
            printf(RED
                   "  ✗ Usage: mygit commit \"message\"\n"
                   RESET);
            return 1;
        }
        return mygit_commit(argv[2]);
    }

    /* ── LOG ── */
    else if (strcmp(command, "log") == 0) {
        return mygit_log();
    }

    /* ── STATUS ── */
    else if (strcmp(command, "status") == 0) {
        return mygit_status();
    }

    /* ── DIFF ── */
    else if (strcmp(command, "diff") == 0) {
        if (argc < 3) {
            printf(RED
                   "  ✗ Usage: mygit diff <filename>\n"
                   RESET);
            return 1;
        }
        return mygit_diff(argv[2]);
    }

    /* ── CHECKOUT ── */
    else if (strcmp(command, "checkout") == 0) {
        if (argc < 3) {
            printf(RED
                   "  ✗ Usage: mygit checkout <id>\n"
                   RESET);
            return 1;
        }
        return mygit_checkout(argv[2]);
    }

   /* ── BRANCH ── */
else if (strcmp(command, "branch") == 0) {

    if (argc < 3) {
        /* mygit branch → list all */
        return mygit_list_branches();
    }

    if (strcmp(argv[2], "switch") == 0) {
        /* mygit branch switch <name> */
        if (argc < 4) {
            printf(RED
                   "  ✗ Usage: mygit branch"
                   " switch <name>\n\n"
                   RESET);
            return 1;
        }
        return switch_branch(argv[3]);
    }

    /* mygit branch <name> → create */
    return mygit_branch_create(argv[2]);
}

    /* ── HELP ── */
    else if (strcmp(command, "help") == 0) {
        print_banner();
        print_help();
        return 0;
    }

    /* ── UNKNOWN ── */
    else {
        printf(RED
               "\n  ✗ Unknown command: '%s'\n"
               RESET, command);
        printf("  Run "
               YELLOW "mygit help"
               RESET " to see commands.\n\n");
        return 1;
    }

    return 0;
}