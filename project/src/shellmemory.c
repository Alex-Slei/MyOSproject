#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

static const char *const VAR_NOT_FOUND = "Variable does not exist";
static char *const INDEX_OOB = "Index out of bounds";

struct memory_struct {
    char *var;
    char *value;
};

struct code_struct {
    char lines[MEM_SIZE][LINE_SIZE];
    int used;
};

struct memory_struct shellmemory[MEM_SIZE];
struct code_struct code_memory;

// Helper functions

// int match(char *model, char *var) {
//     int i, len = strlen(var), matchCount = 0;
//     for (i = 0; i < len; i++) {
//         if (model[i] == var[i])
//             matchCount++;
//     }
//     if (matchCount == len) {
//         return 1;
//     } else
//         return 0;
// }

// Shell memory functions

/**
* sets every key value pair to NULL
*/
void mem_init() {
    int i;
    code_memory.used = 0;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = NULL;
        shellmemory[i].value = NULL;
    }
}

/**
 * Sets a key value pair in shellmemory,
 * If the inputted key already exists its value will be overwritten.
 *
 * @param var_in key
 * @param value_in value
 */
void mem_set_value(char *var_in, char *value_in) {
    int i;
    //First check if the key already exists, if it does overwrite it
    for (i = 0; i < MEM_SIZE; i++) {
        if (shellmemory[i].var && strcmp(shellmemory[i].var, var_in) == 0) {
            free(shellmemory[i].value); //free the previous value
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (shellmemory[i].var == NULL) {
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    return;
}

/**
* Get a value by its key
* @param var_in the key
* @return the desired value or "Variable does not exist", borrowed pointer do not free
*/
const char *mem_get_value(char *var_in) {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        if (shellmemory[i].var && strcmp(shellmemory[i].var, var_in) == 0) {
            return shellmemory[i].value;
        }
    }
    return VAR_NOT_FOUND;
}

void code_set_line(char *line_in, int n) {
    //line size check
    if (strlen(line_in) > LINE_SIZE) {
        return;
    }

    if (n < 0 || n > MEM_SIZE)
        return;

    strcpy(code_memory.lines[n], line_in);
}

char *code_get_line(int n) {
    if (n >= 0 && n < MEM_SIZE) {
        return code_memory.lines[n];
    } else {
        return INDEX_OOB;
    }

}

// int and start must be allocated before calling
int load_code_mem(const char *path, int *start, int *end) {
    char line[LINE_SIZE];

    //open the file
    FILE *p = fopen(path, "r");
    if (p == NULL) {
        return -1;
    }

    *start = code_memory.used;

    while (fgets(line, LINE_SIZE - 1, p)) {

        if (code_memory.used < MEM_SIZE) {      //check for out of bounds
            strcpy(code_memory.lines[code_memory.used], line);  //copy each line into memory
            code_memory.used++; //update the used counter
        } else {

            reset_code_mem(*start, code_memory.used);
            code_memory.used = *start;
            fclose(p);
            printf("Ran out of code memory");
            return 1;
        }
    }

    *end = code_memory.used;    // assign end from code memory
    fclose(p);                  //close the file
    return 0;
}

/**
 *resets a chunk of code memory to \0
 *@param start the line number of the start of the chunk
 *@param end the line number of the end of the chunk
 *
 *If end is -1 (start can be any int), the code will reset 
 */
void reset_code_mem(int start, int end) {
    if (end == -1) {
        for (int i = 0; i < MEM_SIZE; i++) {
            memset(code_memory.lines[i], 0, sizeof(code_memory.lines[i]));
        }
        code_memory.used = 0;
    } else {
        for (int i = start; i < end; i++) {
            memset(code_memory.lines[i], 0, sizeof(code_memory.lines[i]));
        }
        //code_memory.used = end - start + 1;
    }
}
