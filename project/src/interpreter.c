#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include "shellmemory.h"
#include "shell.h"
#include "scheduler.h"

//int MAX_ARGS_SIZE = 3;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);
int badcommandFileDoesNotExist();
int echo(char *message);
int my_ls();
int my_mkdir(char *dirname);
int my_touch(char *filename);
int my_cd(char *dirname);
int isalnumstring(char *s);
int is_valid_dirnam(const char *s);
int delete_dir(char *path);
int run(char *args[], int args_size);
int my_exec(char *args[], int args_size);

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;
    chdir(get_root());

    if (args_size < 1) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        //print
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        //source
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        //echo
        if (args_size != 2) {
            return badcommand();
        }
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        //my_ls
        if (args_size != 1) {
            return badcommand();
        }
        return my_ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        //my_mkdir
        if (args_size != 2) {
            return badcommand();
        }
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        //my_touch
        if (args_size != 2) {
            return badcommand();
        }
        return my_touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        //my_cd
        if (args_size != 2) {
            return badcommand();
        }
        return my_cd(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        //run
        if (args_size < 2) {
            return badcommand();
        }
        return run(&command_args[0], args_size);

    } else if (strcmp(command_args[0], "exec") == 0) {
        //exec
        if (args_size < 3 || args_size > 5) {
            return badcommand();
        }
        return my_exec(&command_args[0], args_size);
    } else {
        return badcommand();
    }
}

int my_exec(char *args[], int args_size) {
    char *policy = args[args_size - 1];
    int errCode;
    int process_count = 0;
    //chdir("..");
    //check that the policy is correct
    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0
        && strcmp(policy, "RR") != 0 && strcmp(policy, "AGING") != 0 && strcmp(policy, "RR30") != 0) {
        return badcommand();
    }

    for (int i = 1; i < args_size - 1; i++) {   //for each script
        errCode = 0;
        //create load the code into memory
        int start, end;
        errCode = load_code_mem(args[i], &start, &end);
        if (errCode == 1)
            return 1;

        //create PCB
        PCB *proc = createPCB(start, end);
        errCode = processtable_add(proc);
        if (errCode == 1)
            return 1;
        process_count++;
    }

    // Then schedule them all at once
    errCode = schedule(args[args_size - 1], process_count);
    return errCode;
}

int quit() {
    chdir(get_root());
    delete_dir("OS");
    rmdir("OS");
    printf("Bye!\n");
    exit(0);
}

/**
* Deletes all the subfiles of the directory at the givenpath
* Only called by quit() to remove the scratch directory before exiting
*
* @param path Path to the directory
* @returun 0 on success, -1 on failure
*/
int delete_dir(char *path) {
    DIR *d = opendir(path);

    if (!d) {
        perror("opendir");
        return -1;
    }

    struct dirent *currdir;
    char fullpath[200];

    while ((currdir = readdir(d)) != NULL) {

        //exclude . and .. directories
        if (strcmp(currdir->d_name, ".") == 0
            || strcmp(currdir->d_name, "..") == 0) {
            continue;
        }
        // create the string containing the full path
        strcpy(fullpath, path);
        strcat(fullpath, "/");
        strcat(fullpath, currdir->d_name);

        struct stat file_info;  //declare file info struct
        if (stat(fullpath, &file_info) == 0) {  //get information on the current file

            if (S_ISDIR(file_info.st_mode)) {   //if its a directory
                //printf("debug");
                delete_dir(fullpath);   // reccursive call to delete entries in a non emtpy dir
                rmdir(fullpath);        // delete the directory once its empty

            } else {            // if its not a directory then its a file
                unlink(fullpath);       //simply delete the file
            }
        }

    }

    closedir(d);
    return 0;

}

int set(char *var, char *value) {
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

int source(char *script) {
    //chdir("..");
    char absolute_path[PATH_MAX];
    int errCode = 0;

    if (realpath(script, absolute_path) == NULL) {
        //chdir("OS");
        return badcommandFileDoesNotExist();
    }

    int start, end;
    errCode = load_code_mem(absolute_path, &start, &end);
    //error checks

    //ini the PCB
    PCB *proc = createPCB(start, end);
    //add to processtable
    processtable_add(proc);

    errCode = schedule("FCFS", 1); //policy: FCFS(arbirtrary), proc_count = 1;

    //chdir("OS");
    return errCode;
}

int echo(char *message) {
    if (message[0] != '$') {
        printf("%s\n", message);
        if (!is_valid_dirnam(message))
            return 1;
        return 0;
    } else {
        if (!is_valid_dirnam(&message[1]))
            return 1;
        printf("%s\n", mem_get_value(&message[1]));
        return 0;
    }
}

int my_ls() {
    chdir("OS");
    struct dirent **names;
    int n = scandir(".", &names, NULL, alphasort);      //scandir returns the number of files

    // or returns -1 if theres an error
    if (n == -1) {
        return badcommand();
    } else {
        //iterate through the output of scandir()
        for (int i = 0; i < n; i++) {
            printf("%s\n", names[i]->d_name);
            free(names[i]);     // free current dirent
        }

        free(names);            //free the array
        return 0;
    }
}

int my_mkdir(char *dirname) {
    chdir("OS");
    const char *name = dirname; //declare string

    if (dirname[0] == '$') {
        name = mem_get_value(&dirname[1]);      //fetches the name from memory

        // variable does not exist check
        if (strcmp(name, "Variable does not exist") == 0) {
            printf("Bad command: my_mkdir\n");
            return 1;
        }
    }
    //non alphanumerical token check
    if (!is_valid_dirnam(name)) {
        printf("Bad command: my_mkdir\n");
        return 1;
    }

    return mkdir(name, S_IRWXU);
}

int my_touch(char *filename) {
    chdir("OS");
    //non alphanumerical token check
    if (!is_valid_dirnam(filename)) {
        return badcommand();
    }

    FILE *f = fopen(filename, "a");
    if (f == NULL) {
        return badcommand();
    }

    fclose(f);                  //close file before returning
    return 0;
}

/**
 * Takes a null-terminated string as an input
 * returns 1 if a string does NOT contain any alphanumerical characters
 * returns 0 if it does
 */
int isalnumstring(char *s) {
    //loop over the string
    for (int i = 0; s[i] != '\0'; i++) {
        //check that the current charcter is not alphanumerical
        if (!isalnum((unsigned char) s[i])) {
            return 0;
        }
    }
    return 1;
}

/**
 * Takes a null-terminated string as an input
 * returns 1 if the string is a valid directory name
 * returns 0 if the string is NOT a valid directory name
 */
int is_valid_dirnam(const char *s) {
    //loop over the string
    for (int i = 0; s[i] != '\0'; i++) {

        //check that the current charcter is allowed
        if ((isalnum((unsigned char) s[i])) || s[i] == '_' || s[i] == '-'
            || s[i] == '.') {
            continue;
        } else {
            return 0;           //returns if it is not
        }

    }
    return 1;
}


int my_cd(char *dirname) {
    chdir("OS");
    int found = 0;
    struct dirent **names;
    int n = scandir(".", &names, NULL, alphasort);      //scandir returns the number of files

    // or returns -1 if theres an error
    if (n == -1) {
        return badcommand();
    } else {
        //iterate through the output of scandir()
        for (int i = 0; i < n; i++) {
            if (!found) {
                if (strcmp(names[i]->d_name, dirname) == 0) {   //if we have a match
                    if (chdir(dirname) == 0) {  //try to cd
                        found = 1;      // success
                    } else {
                        found = 0;      // matched but couldn’t cd
                    }
                }
            }
            free(names[i]);     // free current dirent
        }

        free(names);            //free the array

        if (!found) {
            printf("bad command: my_cd\n");
            return 1;
        } else {
            return 0;
        }
    }
}

int run(char *args[], int args_size) {
    //chdir(get_root());
    pid_t pid = fork();
    args[args_size] = NULL;

    if (pid == 0) {
        execvp(args[1], &args[1]);
        perror("error when calling execvp");    //program shouldn't reach here unless exec fails
        exit(1);                // we want to terminate the child rather than returning -1

    } else if (pid > 0) {
        //parent (wait)
        wait(NULL);
        //chdir("OS");
        return 0;

    } else {
        //fork fails
        perror("error when calling fork()");
        return -1;
    }
}

int help() {
    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}
