/*
 * ============================================
 *          MYGIT - Mini Version Control
 *          Header File - Core Definitions
 * ============================================
 */

#ifndef MYGIT_H
#define MYGIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* 
 * WINDOWS vs LINUX compatibility
 * Windows doesn't have dirent.h or unistd.h
 * We handle both cases here
 */
#ifdef _WIN32
    #include <direct.h>     // Windows: for _mkdir
    #include <windows.h>    // Windows: for directory operations
    #include <io.h>         // Windows: for _access
    #define PATH_SEP "\\"
#else
    #include <dirent.h>     // Linux/Mac: for directory operations
    #include <unistd.h>     // Linux/Mac: for access()
    #define PATH_SEP "/"
#endif

/* ─── rest of the file stays EXACTLY the same ─── */

/* ─────────── CONSTANTS ─────────── */

#define MAX_MESSAGE     256
#define MAX_FILENAME    256
#define MAX_CONTENT     10000
#define MAX_PATH        512
#define MAX_LINE        1024
#define MAX_LINES       500
#define MAX_BRANCH_NAME 50
#define HASH_LENGTH     20

#define MYGIT_DIR       ".mygit"
#define OBJECTS_DIR     ".mygit/objects"
#define REFS_DIR        ".mygit/refs"
#define HEAD_FILE       ".mygit/HEAD"
#define STAGING_FILE    ".mygit/staging.dat"
#define COMMITS_FILE    ".mygit/commits.dat"

/* ─────────── COLOR CODES (for pretty output) ─────────── */

#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define RESET   "\033[0m"

/* ─────────── DATA STRUCTURES ─────────── */

/*
 * STAGED FILE NODE (Linked List)
 * ──────────────────────────────
 * When you do "mygit add file.txt", the file goes here.
 * It's a waiting area before commit.
 * 
 * WHY LINKED LIST? 
 * → We don't know how many files user will stage
 * → Easy to add/remove dynamically
 * → No wasted memory like arrays
 */
typedef struct StagedFile {
    char filename[MAX_FILENAME];
    unsigned long hash;              // hash of file content
    struct StagedFile* next;
} StagedFile;

/*
 * COMMIT NODE (Linked List)
 * ─────────────────────────
 * Each commit is a snapshot in time.
 * Points to its parent (previous commit).
 * 
 * WHY LINKED LIST?
 * → Commits are SEQUENTIAL (one after another)
 * → We only add to the front (latest commit)
 * → We traverse from newest → oldest (like git log)
 * → Insert: O(1), Traverse: O(n)
 */
typedef struct Commit {
    int id;
    char message[MAX_MESSAGE];
    char timestamp[64];
    char branch[MAX_BRANCH_NAME];
    int parent_id;                   // -1 if first commit
    int file_count;
    char filenames[10][MAX_FILENAME]; // files in this commit
    unsigned long file_hashes[10];    // their hashes
    struct Commit* parent;           // pointer to previous commit
    struct Commit* next;             // for loading list from file
} Commit;

/*
 * BRANCH NODE (Linked List)
 * ─────────────────────────
 * A branch is just a NAME pointing to a COMMIT ID.
 * 
 * Example:
 *   main → commit #5
 *   dev  → commit #3
 */
typedef struct Branch {
    char name[MAX_BRANCH_NAME];
    int commit_id;
    struct Branch* next;
} Branch;

/* ─────────── FUNCTION DECLARATIONS ─────────── */

// init.c
int mygit_init(void);

// utils.c
unsigned long hash_content(const char* content);
int file_exists(const char* path);
int directory_exists(const char* path);
int create_directory(const char* path);
int read_file(const char* path, char* buffer, int max_size);
int write_file(const char* path, const char* content);
void get_timestamp(char* buffer, int size);
int get_next_commit_id(void);
char* get_current_branch(char* buffer, int size);
void print_banner(void);
void print_help(void);

// add.c
int mygit_add(const char* filename);

// commit.c
int mygit_commit(const char* message);

// log.c
int mygit_log(void);

// diff.c
int mygit_diff(const char* filename);

// status.c
int mygit_status(void);

// checkout.c
int mygit_checkout(const char* target);

// branch.c
int mygit_branch(const char* branch_name);
int mygit_list_branches(void);

#endif