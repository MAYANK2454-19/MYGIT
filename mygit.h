#ifndef MYGIT_H
#define MYGIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #include <io.h>
    #define PATH_SEP "\\"
#else
    #include <dirent.h>
    #include <unistd.h>
    #define PATH_SEP "/"
#endif

/* ─────────── CONSTANTS ─────────── */

#define MAX_MESSAGE     256
#define MAX_FILENAME    256
#define MAX_CONTENT     10000

/* Renamed to avoid conflict with Windows MAX_PATH */
#ifdef MAX_PATH
#undef MAX_PATH
#endif
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

/* ─────────── COLOR CODES ─────────── */

#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define RESET   "\033[0m"

/* ─────────── DATA STRUCTURES ─────────── */

typedef struct StagedFile {
    char filename[MAX_FILENAME];
    unsigned long hash;
    struct StagedFile* next;
} StagedFile;

typedef struct Commit {
    int id;
    char message[MAX_MESSAGE];
    char timestamp[64];
    char branch[MAX_BRANCH_NAME];
    int parent_id;
    int file_count;
    char filenames[10][MAX_FILENAME];
    unsigned long file_hashes[10];
    struct Commit* parent;
    struct Commit* next;
} Commit;

typedef struct Branch {
    char name[MAX_BRANCH_NAME];
    int commit_id;
    struct Branch* next;
} Branch;

/* ─────────── FUNCTION DECLARATIONS ─────────── */

/* init.c */
int mygit_init(void);

/* utils.c */
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

/* add.c */
int mygit_add(const char* filename);

/* commit.c */
int mygit_commit(const char* message);

/* log.c */
int mygit_log(void);

/* diff.c */
int mygit_diff(const char* filename);

/* status.c */
int should_ignore_file(const char* filename);
int mygit_status(void);
int is_in_list(const char* filename,
               char list[][MAX_FILENAME],
               int count);

/* checkout.c */
int mygit_checkout(const char* target);

/* branch.c */
int mygit_branch(const char* branch_name);
int mygit_list_branches(void);

#endif