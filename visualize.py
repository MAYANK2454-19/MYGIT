#!/usr/bin/env python3
"""
================================================
  MyGit Visualizer
  Reads .mygit folder and shows visual graph
  
  Usage: python visualize.py
  
  Works alongside the C-based mygit system.
  Reads the same .mygit folder.
================================================
"""

import os
import sys


# ════════════════════════════════════════════
# COLOR CODES FOR TERMINAL OUTPUT
# Same concept as our C color codes!
# ════════════════════════════════════════════

class Color:
    RED     = '\033[1;31m'
    GREEN   = '\033[1;32m'
    YELLOW  = '\033[1;33m'
    BLUE    = '\033[1;34m'
    MAGENTA = '\033[1;35m'
    CYAN    = '\033[1;36m'
    WHITE   = '\033[1;37m'
    RESET   = '\033[0m'

    @staticmethod
    def red(text):
        return f"{Color.RED}{text}{Color.RESET}"

    @staticmethod
    def green(text):
        return f"{Color.GREEN}{text}{Color.RESET}"

    @staticmethod
    def yellow(text):
        return f"{Color.YELLOW}{text}{Color.RESET}"

    @staticmethod
    def cyan(text):
        return f"{Color.CYAN}{text}{Color.RESET}"

    @staticmethod
    def magenta(text):
        return f"{Color.MAGENTA}{text}{Color.RESET}"

    @staticmethod
    def white(text):
        return f"{Color.WHITE}{text}{Color.RESET}"


# ════════════════════════════════════════════
# CONSTANTS — Same paths as our C program!
# ════════════════════════════════════════════

MYGIT_DIR    = ".mygit"
OBJECTS_DIR  = ".mygit/objects"
REFS_DIR     = ".mygit/refs"
HEAD_FILE    = ".mygit/HEAD"
STAGING_FILE = ".mygit/staging.dat"
COMMITS_FILE = ".mygit/commits.dat"


# ════════════════════════════════════════════
# CLASS: Commit
# Python version of our C Commit struct!
# ════════════════════════════════════════════

class Commit:
    """
    Represents one commit.
    Just like our C struct Commit!
    
    In C:
      typedef struct Commit {
          int id;
          char message[256];
          ...
      } Commit;
    
    In Python, we use a class instead.
    """
    
    def __init__(self):
        self.id        = 0
        self.message   = ""
        self.timestamp = ""
        self.branch    = ""
        self.parent_id = -1
        self.filenames = []
        self.hashes    = []
    
    def __repr__(self):
        return f"Commit(#{self.id}: {self.message})"


# ════════════════════════════════════════════
# FUNCTION: check_repo_exists
# Make sure .mygit folder exists
# ════════════════════════════════════════════

def check_repo_exists():
    """
    Check if .mygit folder exists.
    If not, tell user to run mygit init first.
    """
    if not os.path.exists(MYGIT_DIR):
        print(Color.red(
            "\n  ✗ No MyGit repository found!"
        ))
        print("    Run " + 
              Color.yellow("mygit init") + 
              " first.\n")
        sys.exit(1)
    
    if not os.path.exists(COMMITS_FILE):
        print(Color.yellow(
            "\n  No commits yet!"
        ))
        print("    Add files and commit first.\n")
        sys.exit(0)


# ════════════════════════════════════════════
# FUNCTION: read_head
# Gets the current branch name from HEAD file
# ════════════════════════════════════════════

def read_head():
    """
    Reads .mygit/HEAD to get current branch.
    
    Just like get_current_branch() in C!
    """
    try:
        with open(HEAD_FILE, 'r') as f:
            return f.read().strip()
    except:
        return "main"


# ════════════════════════════════════════════
# FUNCTION: read_branches
# Reads all branch files from refs/ folder
# Returns dict: {branch_name: commit_id}
# ════════════════════════════════════════════

def read_branches():
    """
    Reads all files in .mygit/refs/
    Each file = one branch
    File content = commit ID
    
    Returns:
        dict: {'main': 3, 'dev': 2}
    
    Just like load_all_branches() in C!
    But Python dicts are easier than linked lists here.
    """
    branches = {}
    
    if not os.path.exists(REFS_DIR):
        return branches
    
    # List all files in refs/ folder
    for filename in os.listdir(REFS_DIR):
        ref_path = os.path.join(REFS_DIR, filename)
        
        # Skip directories
        if os.path.isdir(ref_path):
            continue
        
        try:
            with open(ref_path, 'r') as f:
                content = f.read().strip()
                commit_id = int(content)
                branches[filename] = commit_id
        except:
            pass
    
    return branches


# ════════════════════════════════════════════
# FUNCTION: parse_commits
# Reads commits.dat and returns list of
# Commit objects (newest first)
# ════════════════════════════════════════════

def parse_commits():
    """
    Reads .mygit/commits.dat
    Parses each commit block
    Returns list of Commit objects sorted newest first
    
    This is like load_all_commits() + parse_commit_block()
    from our C code — but in Python!
    
    File format:
        COMMIT:3
        MSG:Fixed bug
        TIME:2026-04-28 14:30:00
        BRANCH:main
        PARENT:2
        FILES:hello.txt,notes.txt
        HASHES:193485797,874291053
        END
    """
    commits = []
    
    try:
        with open(COMMITS_FILE, 'r') as f:
            lines = f.readlines()
    except:
        return commits
    
    # Parse line by line
    current = None
    
    for line in lines:
        line = line.strip()
        
        # Skip empty lines and comments
        if not line or line.startswith('#'):
            continue
        
        # New commit block starts
        if line.startswith('COMMIT:'):
            current = Commit()
            current.id = int(line.split(':')[1])
        
        elif current is None:
            continue
        
        elif line.startswith('MSG:'):
            # Everything after "MSG:" is the message
            current.message = line[4:]
        
        elif line.startswith('TIME:'):
            current.timestamp = line[5:]
        
        elif line.startswith('BRANCH:'):
            current.branch = line[7:]
        
        elif line.startswith('PARENT:'):
            current.parent_id = int(line[7:])
        
        elif line.startswith('FILES:'):
            # Split comma-separated filenames
            files_str = line[6:]
            if files_str:
                current.filenames = files_str.split(',')
        
        elif line.startswith('HASHES:'):
            # Split comma-separated hashes
            hashes_str = line[7:]
            if hashes_str:
                current.hashes = [
                    int(h) for h in hashes_str.split(',')
                ]
        
        elif line == 'END':
            if current:
                commits.append(current)
                current = None
    
    # Sort newest first (highest ID first)
    commits.sort(key=lambda c: c.id, reverse=True)
    
    return commits


# ════════════════════════════════════════════
# FUNCTION: read_staging
# Reads staging.dat
# Returns list of (filename, hash) tuples
# ════════════════════════════════════════════

def read_staging():
    """
    Reads .mygit/staging.dat
    Returns list of staged files with their hashes
    
    File format:
        # MyGit Staging Area
        hello.txt|193485797
        notes.txt|874291053
    """
    staged = []
    
    try:
        with open(STAGING_FILE, 'r') as f:
            for line in f:
                line = line.strip()
                
                # Skip comments and empty lines
                if not line or line.startswith('#'):
                    continue
                
                # Parse "filename|hash"
                parts = line.split('|')
                if len(parts) == 2:
                    filename = parts[0]
                    hash_val = int(parts[1])
                    staged.append((filename, hash_val))
    except:
        pass
    
    return staged


# ════════════════════════════════════════════
# FUNCTION: count_file_lines
# Counts lines in a blob file
# ════════════════════════════════════════════

def count_file_lines(hash_val):
    """
    Reads a blob file and counts its lines.
    
    blob_path = .mygit/objects/<hash>.blob
    """
    blob_path = os.path.join(
        OBJECTS_DIR,
        f"{hash_val}.blob"
    )
    
    try:
        with open(blob_path, 'r') as f:
            lines = f.readlines()
            return len(lines)
    except:
        return 0


# ════════════════════════════════════════════
# FUNCTION: get_branch_for_commit
# Finds which branches point to a commit
# ════════════════════════════════════════════

def get_branches_for_commit(commit_id, branches):
    """
    Finds all branch names that point to this commit.
    
    Example:
        commit_id = 3
        branches = {'main': 3, 'dev': 2}
        returns ['main']
    """
    result = []
    for branch_name, bid in branches.items():
        if bid == commit_id:
            result.append(branch_name)
    return result


# ════════════════════════════════════════════
# FUNCTION: print_banner
# Shows the visualizer header
# ════════════════════════════════════════════

def print_banner():
    print()
    print(Color.cyan(
        "  +============================================+"
    ))
    print(Color.cyan("  |") +
          Color.yellow(
              "        MyGit Visualizer                  "
          ) +
          Color.cyan("|"))
    print(Color.cyan("  |") +
          "  Reads .mygit and shows visual graph       " +
          Color.cyan("|"))
    print(Color.cyan(
        "  +============================================+"
    ))
    print()


# ════════════════════════════════════════════
# FUNCTION: print_commit_graph
# THE MAIN VISUALIZATION!
# Shows commits as a visual graph
# ════════════════════════════════════════════

def print_commit_graph(commits, branches, head):
    """
    Draws the commit graph like this:
    
    [3] 2026-04-28  "Fixed bug"        (main) <- HEAD
     |
    [2] 2026-04-28  "Added feature"    (dev)
     |
    [1] 2026-04-28  "Initial commit"   (main)
    
    Parameters:
        commits  → list of Commit objects (newest first)
        branches → dict of branch name → commit id
        head     → current branch name
    """
    
    print(Color.cyan("  COMMIT GRAPH:"))
    print(Color.cyan("  " + "─" * 60))
    print()
    
    if not commits:
        print(Color.yellow("  No commits yet!"))
        print()
        return
    
    for i, commit in enumerate(commits):
        
        # Find which branches point to this commit
        commit_branches = get_branches_for_commit(
            commit.id, branches
        )
        
        # Build the branch label
        branch_label = ""
        if commit_branches:
            labels = []
            for b in commit_branches:
                if b == head:
                    # Current branch gets special color
                    labels.append(
                        Color.green(f"{b}") +
                        Color.yellow(" ← HEAD")
                    )
                else:
                    labels.append(Color.magenta(b))
            branch_label = "  (" + ", ".join(labels) + ")"
        
        # Shorten timestamp (just date part)
        date = commit.timestamp[:10] \
               if len(commit.timestamp) >= 10 \
               else commit.timestamp
        
        # Shorten message if too long
        msg = commit.message
        if len(msg) > 30:
            msg = msg[:27] + "..."
        
        # Print the commit node
        print(
            "  " +
            Color.yellow(f"[{commit.id}]") +
            f" {Color.cyan(date)}" +
            f"  {Color.white(chr(34) + msg + chr(34))}" +
            branch_label
        )
        
        # Print the connecting line (except for last commit)
        if i < len(commits) - 1:
            print(Color.cyan("   |"))
        else:
            # Last commit — show root marker
            print(Color.cyan("   *") +
                  " (root commit)")
    
    print()


# ════════════════════════════════════════════
# FUNCTION: print_branches_table
# Shows all branches in a nice table
# ════════════════════════════════════════════

def print_branches_table(branches, head):
    """
    Shows all branches like:
    
    BRANCHES:
      * main  →  commit #3   (current)
        dev   →  commit #2
    """
    
    print(Color.cyan("  BRANCHES:"))
    print(Color.cyan("  " + "─" * 40))
    
    if not branches:
        print(Color.yellow("  No branches found!"))
        print()
        return
    
    for name, commit_id in sorted(branches.items()):
        is_current = (name == head)
        
        if is_current:
            print(
                Color.green(f"  * {name:<20}") +
                Color.green(f"→  commit #{commit_id}") +
                Color.yellow("  (current)")
            )
        else:
            print(
                f"    {name:<20}" +
                f"→  commit #{commit_id}"
            )
    
    print()


# ════════════════════════════════════════════
# FUNCTION: print_staging_area
# Shows what's in the staging area
# ════════════════════════════════════════════

def print_staging_area(staged):
    """
    Shows staged files like:
    
    STAGING AREA:
      hello.txt   (hash: 193485797)
      notes.txt   (hash: 874291053)
    """
    
    print(Color.cyan("  STAGING AREA:"))
    print(Color.cyan("  " + "─" * 40))
    
    if not staged:
        print(Color.yellow(
            "  Nothing staged. "
            "(use mygit add <file>)"
        ))
        print()
        return
    
    for filename, hash_val in staged:
        print(
            Color.green(f"  + {filename:<25}") +
            f"(hash: {hash_val})"
        )
    
    print()


# ════════════════════════════════════════════
# FUNCTION: print_latest_commit_files
# Shows files in the most recent commit
# ════════════════════════════════════════════

def print_latest_commit_files(commits):
    """
    Shows the files in the latest commit with
    line counts from their blobs.
    
    FILES IN LATEST COMMIT:
      1. hello.txt   (12 lines)
      2. notes.txt   (8 lines)
    """
    
    print(Color.cyan("  FILES IN LATEST COMMIT:"))
    print(Color.cyan("  " + "─" * 40))
    
    if not commits:
        print(Color.yellow("  No commits yet!"))
        print()
        return
    
    latest = commits[0]  # First = newest (sorted)
    
    print(f"  Commit #{latest.id}: "
          f"{Color.white(latest.message)}\n")
    
    if not latest.filenames:
        print(Color.yellow("  No files in this commit!"))
        print()
        return
    
    for i, filename in enumerate(latest.filenames):
        
        # Get hash for this file
        hash_val = 0
        if i < len(latest.hashes):
            hash_val = latest.hashes[i]
        
        # Count lines in blob
        lines = count_file_lines(hash_val)
        line_str = f"{lines} lines" \
                   if lines > 0 \
                   else "blob not found"
        
        print(
            f"  {i+1}. " +
            Color.green(f"{filename:<25}") +
            f"({line_str})"
        )
    
    print()


# ════════════════════════════════════════════
# FUNCTION: print_statistics
# Shows interesting stats about the repo
# ════════════════════════════════════════════

def print_statistics(commits, branches, staged):
    """
    Shows repo statistics:
    
    REPOSITORY STATISTICS:
      Total commits    : 5
      Total branches   : 2
      Files staged     : 1
      Blobs stored     : 8
    """
    
    print(Color.cyan("  REPOSITORY STATISTICS:"))
    print(Color.cyan("  " + "─" * 40))
    
    # Count blobs
    blob_count = 0
    if os.path.exists(OBJECTS_DIR):
        blob_count = len([
            f for f in os.listdir(OBJECTS_DIR)
            if f.endswith('.blob')
        ])
    
    # Total files committed (across all commits)
    total_files = sum(
        len(c.filenames) for c in commits
    )
    
    # Count unique files
    unique_files = set()
    for c in commits:
        for f in c.filenames:
            unique_files.add(f)
    
    print(f"  Total commits    : " +
          Color.yellow(str(len(commits))))
    
    print(f"  Total branches   : " +
          Color.yellow(str(len(branches))))
    
    print(f"  Files staged     : " +
          Color.yellow(str(len(staged))))
    
    print(f"  Blobs stored     : " +
          Color.yellow(str(blob_count)))
    
    print(f"  Unique files     : " +
          Color.yellow(str(len(unique_files))))
    
    print(f"  Total snapshots  : " +
          Color.yellow(str(total_files)))
    
    print()


# ════════════════════════════════════════════
# FUNCTION: print_ascii_tree
# Shows a more detailed ASCII tree view
# ════════════════════════════════════════════

def print_ascii_tree(commits, branches, head):
    """
    Shows a detailed tree with file info:
    
    DETAILED TREE:
    
    Commit #3 ── main (HEAD)
    │  Message : Fixed bug
    │  Time    : 2026-04-28
    │  Files   : hello.txt
    │
    Commit #2 ── dev
    │  Message : Added feature
    │  Files   : hello.txt, notes.txt
    │
    Commit #1 ── main
       Message : Initial commit
       Files   : hello.txt
    """
    
    print(Color.cyan("  DETAILED TREE VIEW:"))
    print(Color.cyan("  " + "─" * 50))
    print()
    
    if not commits:
        print(Color.yellow("  No commits!"))
        print()
        return
    
    for i, commit in enumerate(commits):
        
        is_last = (i == len(commits) - 1)
        
        # Find branches at this commit
        commit_branches = get_branches_for_commit(
            commit.id, branches
        )
        
        # Build branch string
        branch_str = ""
        if commit_branches:
            parts = []
            for b in commit_branches:
                if b == head:
                    parts.append(
                        Color.green(b) +
                        Color.yellow("*")
                    )
                else:
                    parts.append(Color.magenta(b))
            branch_str = " ── " + ", ".join(parts)
        
        # Print commit header
        connector = "└──" if is_last else "├──"
        print(
            f"  {connector} " +
            Color.yellow(f"Commit #{commit.id}") +
            branch_str
        )
        
        # Print details
        pipe = " " if is_last else "│"
        
        print(
            f"  {pipe}   " +
            Color.cyan("Message : ") +
            Color.white(commit.message)
        )
        
        print(
            f"  {pipe}   " +
            Color.cyan("Time    : ") +
            commit.timestamp
        )
        
        if commit.parent_id == -1:
            parent_str = "(root)"
        else:
            parent_str = f"Commit #{commit.parent_id}"
        
        print(
            f"  {pipe}   " +
            Color.cyan("Parent  : ") +
            parent_str
        )
        
        if commit.filenames:
            files_str = ", ".join(commit.filenames)
            print(
                f"  {pipe}   " +
                Color.cyan("Files   : ") +
                Color.green(files_str)
            )
        
        if not is_last:
            print(f"  │")
    
    print()


# ════════════════════════════════════════════
# FUNCTION: interactive_menu
# Let user choose what to see
# ════════════════════════════════════════════

def interactive_menu(commits, branches,
                     staged, head):
    """
    Shows an interactive menu so user can
    choose what to display.
    """
    
    while True:
        print(Color.cyan("  " + "═" * 45))
        print(Color.yellow("  What would you like to see?"))
        print(Color.cyan("  " + "═" * 45))
        print(f"  {Color.green('1')}. Commit Graph")
        print(f"  {Color.green('2')}. Detailed Tree")
        print(f"  {Color.green('3')}. Branches")
        print(f"  {Color.green('4')}. Staging Area")
        print(f"  {Color.green('5')}. Latest Commit Files")
        print(f"  {Color.green('6')}. Statistics")
        print(f"  {Color.green('7')}. Show Everything")
        print(f"  {Color.red('0')}. Exit")
        print()
        
        try:
            choice = input("  Enter choice: ").strip()
        except (KeyboardInterrupt, EOFError):
            print("\n")
            break
        
        print()
        
        if choice == '1':
            print_commit_graph(
                commits, branches, head
            )
        
        elif choice == '2':
            print_ascii_tree(
                commits, branches, head
            )
        
        elif choice == '3':
            print_branches_table(branches, head)
        
        elif choice == '4':
            print_staging_area(staged)
        
        elif choice == '5':
            print_latest_commit_files(commits)
        
        elif choice == '6':
            print_statistics(
                commits, branches, staged
            )
        
        elif choice == '7':
            print_commit_graph(
                commits, branches, head
            )
            print_ascii_tree(
                commits, branches, head
            )
            print_branches_table(branches, head)
            print_staging_area(staged)
            print_latest_commit_files(commits)
            print_statistics(
                commits, branches, staged
            )
        
        elif choice == '0':
            print(Color.yellow(
                "  Goodbye!\n"
            ))
            break
        
        else:
            print(Color.red(
                "  Invalid choice! Try again.\n"
            ))


# ════════════════════════════════════════════
# MAIN — Entry point
# ════════════════════════════════════════════

def main():
    """
    Main function.
    
    Steps:
      1. Check repo exists
      2. Read all data from .mygit
      3. Show interactive menu
    """
    
    # Enable Windows color support
    if sys.platform == 'win32':
        os.system('color')
    
    # Print header
    print_banner()
    
    # Step 1: Check repo exists
    check_repo_exists()
    
    # Step 2: Read all data
    head     = read_head()
    branches = read_branches()
    commits  = parse_commits()
    staged   = read_staging()
    
    # Show quick summary
    print(
        f"  Repository: " +
        Color.green(os.path.abspath(".")) +
        "\n"
    )
    print(
        f"  Current branch : " +
        Color.yellow(head)
    )
    print(
        f"  Total commits  : " +
        Color.yellow(str(len(commits)))
    )
    print(
        f"  Total branches : " +
        Color.yellow(str(len(branches)))
    )
    print(
        f"  Files staged   : " +
        Color.yellow(str(len(staged)))
    )
    print()
    
    # Step 3: Show interactive menu
    interactive_menu(commits, branches,
                     staged, head)


# Run the script
if __name__ == "__main__":
    main()