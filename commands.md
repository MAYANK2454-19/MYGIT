
---

# 📋 All Commands — Single Reference

Save this as `COMMANDS.md`:

```markdown
# MyGit — All Commands Reference

---

## Setup

# Compile the project
gcc -o mygit.exe main.c init.c utils.c add.c commit.c log.c status.c diff.c checkout.c branch.c

# Initialize a repository
mygit.exe init

# Show help
mygit.exe help

---

## Daily Workflow

# Check status of files
mygit.exe status

# Stage a file
mygit.exe add hello.txt

# Stage multiple files
mygit.exe add hello.txt
mygit.exe add notes.txt

# Commit staged files
mygit.exe commit "your message here"

# View commit history
mygit.exe log

# View details of one commit
mygit.exe show 1

---

## Comparing Changes

# See what changed in a file
mygit.exe diff hello.txt

---

## Time Travel

# Go back to an old commit
mygit.exe checkout 1
mygit.exe checkout 2
mygit.exe checkout 3

# Go back to newest commit
mygit.exe checkout latest

---

## Branches

# List all branches
mygit.exe branch

# Create a new branch
mygit.exe branch dev
mygit.exe branch feature
mygit.exe branch hotfix

# Switch to a branch
mygit.exe branch switch dev
mygit.exe branch switch main
mygit.exe branch switch feature

---

## Python Visualizer

# Run the visualizer
python visualize.py

# In the menu:
# 1 = Commit Graph
# 2 = Detailed Tree
# 3 = Branches
# 4 = Staging Area
# 5 = Latest Commit Files
# 6 = Statistics
# 7 = Show Everything
# 0 = Exit

---

## Full Demo Workflow (copy-paste ready)

# Fresh start
rmdir /s /q .mygit
mygit.exe init

# First commit
mygit.exe add hello.txt
mygit.exe add notes.txt
mygit.exe status
mygit.exe commit "Initial commit"
mygit.exe log

# Second commit
# (edit hello.txt first)
mygit.exe status
mygit.exe diff hello.txt
mygit.exe add hello.txt
mygit.exe commit "Updated hello file"
mygit.exe log

# View details
mygit.exe show 1
mygit.exe show 2

# Time travel
mygit.exe checkout 1
type hello.txt
mygit.exe checkout latest
type hello.txt

# Branches
mygit.exe branch dev
mygit.exe branch
mygit.exe branch switch dev
# (edit hello.txt for dev)
mygit.exe add hello.txt
mygit.exe commit "Dev branch commit"
mygit.exe branch
mygit.exe branch switch main
type hello.txt

# Visualizer
python visualize.py

---

## Error Cases (for testing)

# Try to commit with nothing staged
mygit.exe commit "empty"

# Try to add non-existent file
mygit.exe add fakefile.txt

# Try to checkout non-existent commit
mygit.exe checkout 99

# Try to create duplicate branch
mygit.exe branch dev
mygit.exe branch dev

# Try to switch to non-existent branch
mygit.exe branch switch xyz

# Try to diff non-existent file
mygit.exe diff fakefile.txt

---

## Check Internal Files

# See commit history (the linked list on disk!)
type .mygit\commits.dat

# See current branch
type .mygit\HEAD

# See branch pointers
type .mygit\refs\main
type .mygit\refs\dev

# See staging area
type .mygit\staging.dat

# See stored blobs
dir .mygit\objects

# See full .mygit structure
dir /s .mygit