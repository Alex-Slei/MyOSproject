#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput(char ui[]);

// Stores the path to a scratch "root" directory,
// used to make `ls` and similar commands output consistently for tests.
static char root[PATH_MAX];

/**
 * Stores the starting working directory in `root`, so we can return to it later.
 */
void set_root() {
    if (getcwd(root, sizeof(root)) == NULL) {
        perror("getcwd");
    }
}

/**
 * Retrieves the stored root directory path.
 */
const char *get_root(void) {
    return root;
}

// Start of everything
int main(int argc, char *argv[]) {
    printf("Shell version 1.4 created December 2024\n\n");
    fflush(stdout);             //needed for piping to work

    char prompt = '$';
    char userInput[MAX_USER_INPUT];
    int errorCode = 0;          // zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    //init shell memory
    mem_init();
    reset_code_mem(0, -1);

    //added a scratch directory
    set_root();
    mkdir("OS", S_IRWXU);
    //chdir("OS");

    int isBatchMode = !isatty(fileno(stdin));   // returns 1 if we're in batch mode

    while (1) {

        char *token;

        if (!isBatchMode) {
            printf("%c ", prompt);      // only print $ when we're not in batch mode
        }

        if (fgets(userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            quit();             // end of file quit logic
        }
        //tokenize the input for instruction chaining
        token = strtok(userInput, ";");
        while (token != NULL) {
            errorCode = parseInput(token);
            if (errorCode == -1)
                exit(99);       // ignore all other errors
            token = strtok(NULL, ";");  //grabs the next token
        }

        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

int wordEnding(char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ';
}

int parseInput(char inp[]) {
    char tmp[200], *words[100];
    int ix = 0, w = 0;
    int wordlen;
    int errorCode;

    // //printf("DEBUG: Raw input: '");
    // for(int i = 0; inp[i] != '\0'; i++) {
    //     if(inp[i] == '\n') printf("\\n");
    //     else printf("%c", inp[i]);
    // }
    // printf("'\n");

    // Skip initial whitespace
    while (ix < 1000 && (inp[ix] == ' ' || inp[ix] == '\t')) {
        ix++;
    }

    // Process each word
    while (ix < 1000 && inp[ix] != '\0') {
        // Skip any whitespace between words
        while (ix < 1000 && (inp[ix] == ' ' || inp[ix] == '\t')) {
            ix++;
        }

        // Stop if we hit end of line/string
        if (inp[ix] == '\0' || inp[ix] == '\n') {
            break;
        }
        // Extract the word
        wordlen = 0;
        while (ix < 1000 && inp[ix] != ' ' && inp[ix] != '\t'
               && inp[ix] != '\n' && inp[ix] != '\0') {
            tmp[wordlen] = inp[ix];
            wordlen++;
            ix++;
        }

        tmp[wordlen] = '\0';
        //printf("DEBUG: Word %d: '%s'\n", w, tmp);
        words[w] = strdup(tmp);
        w++;
    }
    errorCode = interpreter(words, w);

    //free the words
    for (int i = 0; i < w; i++) {
        free(words[i]);
    }
    return errorCode;
}
