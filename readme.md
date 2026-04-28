# MyGit — Mini Version Control System
> Built entirely in C | Data Structures Course Project | 2nd Semester

---

## What is MyGit?

MyGit is a simplified version control system built from scratch in C.
It replicates the core functionality of Git — staging files, committing
snapshots, viewing history, comparing changes, and managing branches —
using only fundamental data structures.

This project was built to answer one question:
**"How does Git actually work internally?"**

The answer: Linked Lists, Hash Functions, Arrays, and Files.

---

## Data Structures Used

### 1. Linked List — Commit Chain

Every commit is a NODE in a linked list.
Each node points to its PARENT which is the previous commit.
Commit 3 → Commit 2 → Commit 1 → NULL
↑
HEAD (newest commit)

text


```c
typedef struct Commit {
    int id;
    char message[256];
    char timestamp[64];
    int parent_id;                  /* THE LINK — like a next pointer */
    char filenames[10][256];
    unsigned long file_hashes[10];
    int file_count;
    struct Commit* next;
} Commit;
Why Linked List and not Array?

INSERT is O(1) — new commits always go to the front
TRAVERSE is O(n) — git log reads newest to oldest sequentially
DYNAMIC SIZE — we never know how many commits there will be
No random access needed — linked list is perfect
2. Hash Function — File Fingerprinting
We use the djb2 algorithm to create a unique number for any file content.

C

unsigned long hash_content(const char* content) {
    unsigned long hash = 5381;
    int c;
    while ((c = *content++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}
text

"Hello World"    →  210714636441
"Hello World!"   →  6952330670010  (completely different!)
"Hello World"    →  210714636441   (same content = same hash always)
Why Hashing?

Detect file changes by comparing two numbers — O(1)
Content-addressable storage — same file stored only ONCE
Deduplication — no wasted disk space
This is exactly how real Git works (Git uses SHA-1)
3. Arrays — Diff Algorithm
The diff command splits files into arrays of lines then compares line by line.

C

char old_lines[500][1024];   /* Lines from committed version */
char new_lines[500][1024];   /* Lines from current version */

for (int i = 0; i < max_lines; i++) {
    if (strcmp(old_lines[i], new_lines[i]) == 0)
        /* UNCHANGED — print normally */
    else
        /* CHANGED — print red minus and green plus */
}
Why Array and not Linked List here?

Random access by line number is O(1) with array
Diff needs lines[i] and lines[i+1] — specific positions
Linked list would need O(n) traversal to reach line i
4. Structs — Custom Data Types
C

typedef struct Branch {
    char name[50];
    int  commit_id;
    struct Branch* next;
} Branch;

typedef struct StagedFile {
    char filename[256];
    unsigned long hash;
    struct StagedFile* next;
} StagedFile;
5. File-Based Persistence — Linked List on Disk
Linked lists in RAM disappear when the program exits.
We serialize our data structures to files so they persist forever.

text

.mygit/commits.dat stores the linked list:

    COMMIT:3
    MSG:Fixed bug
    PARENT:2          ← next pointer stored as integer ID
    FILES:hello.txt
    HASHES:193485797
    END

    COMMIT:2
    MSG:Added feature
    PARENT:1
    END

    COMMIT:1
    MSG:Initial commit
    PARENT:-1         ← -1 means NULL, this is the root node
    END
When we run mygit log, we read this file and rebuild
the entire linked list in memory using malloc and pointers.

Commands Reference
Command	Description
mygit init	Initialize a new repository
mygit add <file>	Stage a file for commit
mygit commit "message"	Save a snapshot
mygit log	Show commit history
mygit status	Show staged, modified, untracked files
mygit diff <file>	Show line-by-line changes
mygit checkout <id>	Restore files from a previous commit
mygit checkout latest	Go to the newest commit
mygit show <id>	Show details of one commit
mygit branch	List all branches
mygit branch <name>	Create a new branch
mygit branch switch <name>	Switch to a branch
mygit help	Show help message
Project Structure
text

minigit/
├── mygit.h         All structs, constants, function declarations
├── main.c          Entry point and command parser using argc argv
├── utils.c         Helper functions — hash, file IO, timestamps
├── init.c          mygit init — creates .mygit folder structure
├── add.c           mygit add — staging area and blob storage
├── commit.c        mygit commit — creates linked list nodes on disk
├── log.c           mygit log — traverses linked list, mygit show
├── status.c        mygit status — detects file states using hashing
├── diff.c          mygit diff — naive line by line diff algorithm
├── checkout.c      mygit checkout — restores old files from blobs
├── branch.c        mygit branch — create, list, switch branches
├── visualize.py    Python visualizer bonus feature
└── README.md       This file
text

.mygit/
├── objects/            File snapshots named by hash
│   ├── 193485797.blob
│   └── 874291053.blob
├── refs/               Branch pointers
│   ├── main            contains the number 3
│   └── dev             contains the number 2
├── HEAD                contains the word main
├── commits.dat         Full commit history as linked list
└── staging.dat         Current staging area
How Each Command Works Internally
mygit add hello.txt
text

Step 1: Read hello.txt content from disk
Step 2: Hash the content using djb2 — result is 193485797
Step 3: Save copy to .mygit/objects/193485797.blob
Step 4: Write "hello.txt|193485797" to staging.dat
mygit commit "message"
text

Step 1: Read staging.dat to get list of staged files
Step 2: Create a Commit struct — this is a new linked list node
Step 3: Fill in id, message, timestamp, branch, parent_id
Step 4: parent_id = last commit ID — THIS IS THE LINKED LIST LINK
Step 5: Append commit block to commits.dat
Step 6: Update refs/main to new commit ID
Step 7: Clear staging.dat
mygit log
text

Step 1: Read commits.dat line by line
Step 2: Parse each commit block into Commit struct
Step 3: Allocate memory with malloc for each node
Step 4: Sort by ID and connect with next pointers
Step 5: Traverse: current = head, follow current->next
Step 6: Print each node, stop at NULL
Step 7: Free all memory with free()
mygit diff hello.txt
text

Step 1: Find committed hash of hello.txt from last commit
Step 2: Load OLD version from .mygit/objects/hash.blob
Step 3: Load NEW version from hello.txt on disk
Step 4: Split both into 2D arrays of lines
Step 5: Compare each line position:
        Same line      → print normally (unchanged)
        Different line → print RED minus then GREEN plus
        Old only       → print RED minus (deleted)
        New only       → print GREEN plus (added)
Step 6: Show statistics — additions, deletions, unchanged
mygit checkout 2
text

Step 1: Parse commit ID from user input
Step 2: Find commit 2 in commits.dat
Step 3: Verify ALL blob files exist before touching anything
        This is called atomic operation — all or nothing
Step 4: Ask user to confirm — this is a destructive operation
Step 5: For each file in commit 2:
        Read its blob → Write to working directory
Step 6: Update refs/main to 2
Step 7: Clear staging.dat
mygit branch dev
text

Step 1: Validate branch name — no spaces, not too long
Step 2: Check it does not already exist
Step 3: Get current commit ID from refs/main
Step 4: Create .mygit/refs/dev containing that commit ID
Step 5: Done — a branch is literally just one file!
Time and Space Complexity
Command	Time	Space	Notes
init	O(1)	O(1)	Creates fixed number of files
add	O(n)	O(n)	n is file size for hashing
commit	O(n)	O(n)	n is number of staged files
log	O(c)	O(c)	c is number of commits
status	O(f + c)	O(f)	f is files, c is commits
diff	O(n)	O(n)	n is number of lines
checkout	O(f)	O(f)	f is files in commit
branch create	O(1)	O(1)	Just writes one file
branch switch	O(f)	O(f)	Restores files
hash_content	O(n)	O(1)	n is content length
Build Instructions
Requirements
GCC compiler — MinGW on Windows, built-in on Linux and Mac
Python 3 for the visualizer only
Compile
text

gcc -o mygit.exe main.c init.c utils.c add.c commit.c log.c status.c diff.c checkout.c branch.c
Run
text

mygit.exe help
mygit.exe init
Python Visualizer
text

python visualize.py
Key Design Decisions
Why files instead of a database?
Two reasons. First, for learning — raw file IO teaches how data
persistence actually works at the lowest level. Databases hide
this complexity behind abstractions.

Second, real Git also uses files. The .git folder contains
plain text files for HEAD, branches, and config. Our design
mirrors the real thing.

Why djb2 instead of SHA-1?
SHA-1 is a cryptographic hash requiring hundreds of lines of C.
djb2 achieves the same conceptual goal — unique fingerprint
for unique content — in 6 lines of code.

For a project at this scale, djb2 has acceptable collision
resistance. For production use, SHA-1 or SHA-256 would be used.

Why linked list for commits instead of array?
Three reasons. Insert is O(1) because new commits go to the
front. We never need random access — log always reads
sequentially from newest to oldest. And we never know how
many commits there will be so dynamic size is essential.

Why a branch is just one file?
Because that is all a branch needs to be. A branch is a
name that maps to a commit ID. One file achieves this
perfectly. Creating a branch in a 10 GB repository takes
zero time because we are just writing one number to one file.
This is exactly how real Git implements branches.

What I Learned
Concept	Where Applied
Linked List insert O(1)	mygit commit
Linked List traversal O(n)	mygit log
Hash function djb2	mygit add, status, diff
2D Arrays	mygit diff
File IO — fopen fread fwrite fclose	Every command
Dynamic memory — malloc and free	mygit log, branch
Pointer arithmetic	hash_content function
Structs as custom types	Commit, Branch, StagedFile
Cross-platform code with ifdef	Windows vs Linux
Error handling patterns	Every command
Atomic operations	mygit checkout
Serialization of data structures	commits.dat format
Pass by pointer vs pass by value	read_staged_files
String functions — strcmp strcpy strtok strncpy	Everywhere
Answers to Common Questions
Q: Why linked list for commits and not an array?
Commits are always inserted at the front — O(1) with linked list,
O(n) with array due to shifting. We traverse sequentially from
newest to oldest — linked list is perfect for this. We never need
to access commit number 5 directly without going through 1,2,3,4.
And arrays need a predefined size but we never know how many commits
there will be.

Q: What is the time complexity of mygit log?
O(n) time and O(n) space where n is the number of commits.
We read every node exactly once and store all of them in memory
while displaying.

Q: How do you detect if a file changed?
We hash the file content when it is staged and store that hash.
When mygit status runs, we hash the current file content and
compare the two numbers. Equal means unchanged. Different means
modified. This turns a potentially slow character-by-character
comparison into a fast number comparison after the initial O(n)
hash calculation.

Q: What is a branch internally?
A text file in .mygit/refs/ containing a commit ID number.
refs/main containing "3" means the main branch points to commit 3.
Creating a branch means creating one file. Switching branches
means changing what HEAD contains. Both are O(1) operations
regardless of repository size.

Q: What was the hardest part?
Serialization. In class we always work with linked lists in RAM.
But real software needs persistence — when the program exits,
RAM is erased. Designing a file format that maps directly to
our struct layout, and then reading it back to rebuild the linked
list in memory, was the most challenging and most educational part
of this project.

Author
text

Name      :  Mayank
Course    :  Data Structures
Semester  :  2nd Semester
Languages :  C for core system, Python for visualizer
The best way to understand a tool is to build it from scratch.