#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LENGTH 1000
#define MAX_ARGS 100
#define MAX_PATH 1000

// Function to parse and replace environment variables in the input
void replace_env_vars(char* command) {
    char result[MAX_CMD_LENGTH] = {0};
    int ix = 0, iy = 0;

    while (command[ix] != '\0') {
        if (command[ix] == '$') {
            ix++;
            int start = ix;
            while (command[ix] != ' ' && command[ix] != '\0' && command[ix] != '$') {
                ix++;
            }
            char var_name[MAX_CMD_LENGTH] = {0};
            strncpy(var_name, command + start, ix - start);
            char* value = getenv(var_name);  // Use the system environment
            if (value) {
                strcat(result, value);
            }
        } else {
            result[iy++] = command[ix++];
        }
    }
    strcpy(command, result);
}

// Function to manually split a string into tokens using spaces (without strtok)
int split_command(char* command, char* args[]) {
    int argc = 0;
    int ix = 0, iy = 0;

    while (command[ix] != '\0') {
        // Skip leading spaces
        while (command[ix] == ' ' && command[ix] != '\0') {
            ix++;
        }

        // Find end of the current word
        iy = ix;
        while (command[iy] != ' ' && command[iy] != '\0') {
            iy++;
        }

        // If there is a word, save it
        if (ix != iy) {
            command[iy] = '\0'; 
            args[argc++] = command + ix;
            ix = iy + 1;
        }
    }

    args[argc] = NULL;  // Null-terminate the argument list
    return argc;
}

// Function to execute built-in commands like cd, pwd
int execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1]) {
            if (chdir(args[1]) == -1) {
                perror("cd failed");
            }
        } else {
            printf("cd: missing argument\n");
        }
        return 1;
    }
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAX_PATH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd failed");
        }
        return 1;
    }
    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
        exit(0);  // Exit the shell
    }
    return 0;
}

// Function to execute external commands
void execute_external(char **args, int background) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("Command not found");
        exit(1);
    } else if (!background) {
        wait(NULL); 
    }
}

// Function to handle background execution
int handle_background(char* args[], int argc) {
    int background = 0;
    if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
        background = 1;
        args[argc - 1] = NULL;  // Remove '&' from arguments
    }
    return background;
}

int main() {
    char command[MAX_CMD_LENGTH];
    char *args[MAX_ARGS];

    while (1) {
        printf("xsh# ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        command[strcspn(command, "\n")] = '\0';

        // Exit condition
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            break;
        }

        // Replace environment variables in the command
        replace_env_vars(command);

        int argc = split_command(command, args);

        // Handle built-in commands first
        if (execute_builtin(args)) {
            continue;
        }

        // Handle background execution
        int background = handle_background(args, argc);

        // Execute external commands
        execute_external(args, background);
    }

    return 0;
}
