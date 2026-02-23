/*
 * ============================================
 *          MYGIT - Add Command
 *          "mygit add <filename>"
 * ============================================
 * 
 * PURPOSE:
 *   Stage a file → Ready it for the next commit
 *   Like putting items in your shopping CART
 *   before going to the CHECKOUT counter
 * 
 * DATA STRUCTURES USED:
 *   1. Array (char buffer) → To hold file content
 *   2. Hash function       → To create unique ID for content
 *   3. Linked List concept → Staging area (saved to file)
 * 
 * WHAT HAPPENS:
 *   1. Check if file exists
 *   2. Read file content into memory
 *   3. Hash the content → unique number
 *   4. Save a copy in .mygit/objects/ folder
 *   5. Record in staging.dat: "filename|hash"
 */

#include "mygit.h"

/*
 * FUNCTION: save_blob
 * ───────────────────
 * Saves a COPY of the file content into .mygit/objects/
 * 
 * The copy is called a "blob" (Binary Large OBject)
 * This is the same term real Git uses!
 * 
 * The file is named by its HASH:
 *   Content "Hi there" → Hash 5864370 → File: 5864370.blob
 * 
 * WHY name by hash?
 *   → If two files have SAME content, we store only ONE copy!
 *   → We can find any version by its hash instantly!
 * 
 * PARAMETERS:
 *   content → The text to save
 *   hash    → The hash of the text (used as filename)
 * 
 * RETURNS:
 *   0  → Success
 *   -1 → Error
 */
int save_blob(const char* content, unsigned long hash) {

    /* 
     * Step 1: Build the file path
     * 
     * We want: ".mygit/objects/5864370.blob"
     * 
     * snprintf builds this string safely:
     *   %s  → gets replaced with OBJECTS_DIR (".mygit/objects")
     *   %lu → gets replaced with hash (5864370)
     *         lu = "long unsigned" → matches our hash type
     */
    char blob_path[MAX_PATH];
    snprintf(blob_path, sizeof(blob_path), "%s/%lu.blob", OBJECTS_DIR, hash);

    /* 
     * Step 2: Check if this blob already exists
     * 
     * If same content was added before, same hash → same file
     * No need to save it again! (saves disk space)
     * 
     * This is called DEDUPLICATION
     * Real Git does this too!
     */
    if (file_exists(blob_path)) {
        return 0;  /* Already saved, nothing to do */
    }

    /* 
     * Step 3: Write the content to the blob file
     */
    if (write_file(blob_path, content) != 0) {
        printf(RED "  ✗ Failed to save object\n" RESET);
        return -1;
    }

    return 0;
}

/*
 * FUNCTION: is_already_staged
 * ───────────────────────────
 * Checks if a file is ALREADY in the staging area
 * 
 * WHY?
 *   If user runs "mygit add hello.txt" twice,
 *   we should UPDATE the entry, not create a DUPLICATE
 * 
 * HOW?
 *   Read staging.dat line by line
 *   Each line looks like: "hello.txt|5864370"
 *   Check if filename matches
 * 
 * RETURNS:
 *   1 → Yes, already staged
 *   0 → No, not staged yet
 */
int is_already_staged(const char* filename) {

    /* Try to open the staging file */
    FILE* fp = fopen(STAGING_FILE, "r");

    /* 
     * If file doesn't exist, nothing is staged 
     * ! means NOT
     * !fp means "fp is NULL" means "couldn't open"
     */
    if (!fp) {
        return 0;
    }

    char line[MAX_LINE];

    /*
     * fgets reads ONE line at a time
     * Returns NULL when there are no more lines
     * 
     * Staging file looks like:
     *   # MyGit Staging Area
     *   hello.txt|5864370
     *   main.c|9283471
     */
    while (fgets(line, sizeof(line), fp)) {

        /* 
         * Skip comment lines (lines starting with #)
         * 
         * line[0] is the FIRST character of the line
         * If it's '#', this is a comment → skip it
         */
        if (line[0] == '#') {
            continue;   /* "continue" = skip to next loop iteration */
        }

        /*
         * Remove the newline character at the end
         * 
         * fgets includes '\n' at the end of each line
         * "hello.txt|5864370\n"  ← we don't want this \n
         * "hello.txt|5864370"    ← we want this
         * 
         * strcspn finds the POSITION of the first \n or \r
         * Then we put '\0' there to cut the string short
         */
        line[strcspn(line, "\n\r")] = '\0';

        /*
         * Skip empty lines
         * strlen returns the length of a string
         * If length is 0, the line is empty → skip
         */
        if (strlen(line) == 0) {
            continue;
        }

        /*
         * Extract the filename from the line
         * 
         * Line format: "hello.txt|5864370"
         * We want just: "hello.txt"
         * 
         * strtok SPLITS a string at the given character
         * 
         * strtok("hello.txt|5864370", "|")
         *   First call → returns "hello.txt"
         * 
         * Like splitting "hello.txt|5864370" at the '|' sign
         */
        char temp_line[MAX_LINE];
        strcpy(temp_line, line);   /* Make a copy (strtok modifies the original!) */
        
        char* stored_filename = strtok(temp_line, "|");

        /* 
         * Compare the stored filename with the one we're looking for
         * strcmp returns 0 if they're EQUAL
         */
        if (stored_filename && strcmp(stored_filename, filename) == 0) {
            fclose(fp);
            return 1;   /* YES, this file is already staged! */
        }
    }

    fclose(fp);
    return 0;   /* NO, this file is not staged */
}

/*
 * FUNCTION: remove_from_staging
 * ─────────────────────────────
 * Removes a file from staging area so we can re-add it
 * (with updated hash if content changed)
 * 
 * HOW IT WORKS:
 *   1. Read ALL lines from staging.dat
 *   2. Write back ALL lines EXCEPT the one we want to remove
 *   
 *   Like rewriting a to-do list but skipping one item
 * 
 * WHY DO WE NEED THIS?
 *   If user does:
 *     mygit add hello.txt       (hash = 111)
 *     * edits hello.txt *
 *     mygit add hello.txt       (hash = 222)
 *   
 *   We need to REPLACE the old entry, not have duplicates!
 *   So we REMOVE the old one first, then ADD the new one.
 */
void remove_from_staging(const char* filename) {

    FILE* fp = fopen(STAGING_FILE, "r");
    if (!fp) return;   /* Nothing to remove if file doesn't exist */

    /* 
     * We'll store all lines EXCEPT the one to remove
     * 
     * Think of it as two piles:
     *   Pile 1: Read from (old staging file)
     *   Pile 2: Write to (new staging file)
     *   Skip the card we don't want!
     */
    char lines[100][MAX_LINE];   /* Can hold up to 100 lines */
    int count = 0;               /* How many lines we've saved */

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {

        /* 
         * For each line, check if it starts with our filename
         * 
         * strstr(haystack, needle)
         *   Searches for "needle" inside "haystack"
         *   Returns pointer to where it found it
         *   Returns NULL if not found
         * 
         * Example:
         *   strstr("hello.txt|5864370", "hello.txt") → FOUND
         *   strstr("main.c|9283471", "hello.txt")    → NOT FOUND (NULL)
         */
        char temp[MAX_LINE];
        strcpy(temp, line);
        temp[strcspn(temp, "\n\r")] = '\0';

        char* stored_name = strtok(temp, "|");

        /* 
         * If this line is NOT about our file → KEEP it
         * If this line IS about our file → SKIP it (don't copy)
         */
        if (!stored_name || strcmp(stored_name, filename) != 0) {
            /* This line is about a DIFFERENT file → keep it */
            strcpy(lines[count], line);
            count++;
        }
        /* If it matches our filename → we simply don't copy it */
        /* That's how we "remove" it! */
    }

    fclose(fp);

    /* 
     * Now write back all the lines we KEPT
     * This overwrites the old file with the filtered version
     */
    fp = fopen(STAGING_FILE, "w");   /* "w" = overwrite! */
    if (!fp) return;

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s", lines[i]);
    }

    fclose(fp);
}

/*
 * ═══════════════════════════════════════════
 * MAIN FUNCTION: mygit_add
 * ═══════════════════════════════════════════
 * 
 * This is what gets called when user types:
 *   mygit add hello.txt
 * 
 * THE BIG PICTURE:
 *   1. Does the file exist?
 *   2. Read its content
 *   3. Hash the content
 *   4. Save a copy (blob) in objects folder
 *   5. Add to staging area
 */
int mygit_add(const char* filename) {

    /* 
     * ──────────────────────────
     * STEP 1: Does the file exist?
     * ──────────────────────────
     * 
     * We can't add a file that doesn't exist!
     * Just like you can't photocopy a document you don't have.
     */
    if (!file_exists(filename)) {
        printf(RED "✗ File not found: '%s'\n" RESET, filename);
        printf("  Make sure the file exists in the current directory.\n");
        return -1;
    }

    /* 
     * ──────────────────────────
     * STEP 2: Read the file content
     * ──────────────────────────
     * 
     * We create a BIG empty box (buffer) and fill it 
     * with the file's content.
     * 
     * Think of it like:
     *   Empty notebook (buffer) → Copy the document into it
     */
    char content[MAX_CONTENT];

    /*
     * read_file returns:
     *   Positive number = how many bytes it read (success!)
     *   -1 = something went wrong (error!)
     */
    int bytes = read_file(filename, content, MAX_CONTENT);

    if (bytes < 0) {
        printf(RED "✗ Could not read file: '%s'\n" RESET, filename);
        return -1;
    }

    /* 
     * ──────────────────────────
     * STEP 3: Hash the content
     * ──────────────────────────
     * 
     * Create a FINGERPRINT of the file's content.
     * 
     * "Hi there" → 193485797
     * 
     * If the file changes even by ONE character,
     * the hash will be COMPLETELY different!
     */
    unsigned long hash = hash_content(content);

    /* 
     * ──────────────────────────
     * STEP 4: Save a copy (blob)
     * ──────────────────────────
     * 
     * Store the content in .mygit/objects/193485797.blob
     * This is our "photocopy" — safe backup!
     */
    if (save_blob(content, hash) != 0) {
        printf(RED "✗ Failed to save file object\n" RESET);
        return -1;
    }

    /* 
     * ──────────────────────────
     * STEP 5: Add to staging area
     * ──────────────────────────
     * 
     * Write "hello.txt|193485797" into staging.dat
     * 
     * But first, check if this file was already staged.
     * If yes, remove the old entry first (it might have old hash)
     */
    if (is_already_staged(filename)) {
        remove_from_staging(filename);
    }

    /* 
     * Now APPEND the new entry to staging.dat
     * 
     * "a" mode = APPEND (add to end, don't overwrite!)
     * 
     * Remember:
     *   "w" = overwrite (DELETES everything, writes fresh)
     *   "a" = append (KEEPS everything, adds to the end)
     */
    FILE* fp = fopen(STAGING_FILE, "a");

    if (!fp) {
        printf(RED "✗ Could not open staging area\n" RESET);
        return -1;
    }

    /* 
     * Write: "hello.txt|193485797\n"
     * 
     * %s  → filename (hello.txt)
     * %lu → hash (193485797) — lu means "long unsigned"
     * \n  → newline (so next entry goes on next line)
     */
    fprintf(fp, "%s|%lu\n", filename, hash);
    fclose(fp);

    /* 
     * ──────────────────────────
     * STEP 6: Tell the user!
     * ──────────────────────────
     */
    printf(GREEN "✓ Staged: " RESET "'%s'\n", filename);
    printf("  Hash: %lu\n", hash);
    printf("  Blob: %s/%lu.blob\n", OBJECTS_DIR, hash);
    printf(CYAN "  → Ready for commit!\n" RESET);

    return 0;   /* Success! */
}