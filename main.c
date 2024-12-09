#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LENGTH 1024
#define MAX_ARGS 100
#define MAX_PATH 1024

// Global environment variables storage
char *env_vars[100][2];  // Array of key-value pairs
int env_count = 0;

// Function to get an environment variable value
char* get_env(const char* name) {
    for (int i = 0; i < env_count; ++i) {
        if (strcmp(env_vars[i][0], name) == 0) {
            return env_vars[i][1];
        }
    }
    return NULL;
}

// Function to set an environment variable
void set_env(const char* name, const char* value) {
    for (int i = 0; i < env_count; ++i) {
        if (strcmp(env_vars[i][0], name) == 0) {
            free(env_vars[i][1]);
            env_vars[i][1] = strdup(value);
            return;
        }
    }
    env_vars[env_count][0] = strdup(name);
    env_vars[env_count][1] = strdup(value);
    ++env_count;
}

// Function to unset an environment variable
void unset_env(const char* name) {
    for (int i = 0; i < env_count; ++i) {
        if (strcmp(env_vars[i][0], name) == 0) {
            free(env_vars[i][0]);
            free(env_vars[i][1]);
            for (int j = i; j < env_count - 1; ++j) {
                env_vars[j][0] = env_vars[j + 1][0];
                env_vars[j][1] = env_vars[j + 1][1];
            }
            --env_count;
            return;
        }
    }
}

// Function to parse and replace environment variables in the input
void replace_env_vars(char* command) {
    char result[MAX_CMD_LENGTH] = {0};
    int i = 0, j = 0;

    while (command[i] != '\0') {
        if (command[i] == '$') {
            i++;
            int start = i;
            while (command[i] != ' ' && command[i] != '\0' && command[i] != '$') {
                i++;
            }
            char var_name[MAX_CMD_LENGTH] = {0};
            strncpy(var_name, command + start, i - start);
            char* value = get_env(var_name);
            if (value) {
                strcat(result, value);
            }
        } else {
            result[j++] = command[i++];
        }
    }
    strcpy(command, result);
}

// Function to manually split a string into tokens using spaces (without strtok)
int split_command(char* command, char* args[]) {
    int argc = 0;
    int i = 0, j = 0;
    
    while (command[i] != '\0') {
        // Skip leading spaces
        while (command[i] == ' ' && command[i] != '\0') {
            i++;
        }

        // Find end of the current word
        j = i;
        while (command[j] != ' ' && command[j] != '\0') {
            j++;
        }

        // If there is a word, save it
        if (i != j) {
            command[j] = '\0';  // Null-terminate the word
            args[argc++] = command + i;
            i = j + 1;
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
        wait(NULL);  // Wait for foreground process to finish
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

// Main shell loop
int main() {
    char command[MAX_CMD_LENGTH];
    char *args[MAX_ARGS];

    while (1) {
        printf("xsh# ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        command[strcspn(command, "\n")] = '\0';  // Remove trailing newline

        // Exit condition
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            break;
        }

        // Replace environment variables in the command
        replace_env_vars(command);

        // Tokenize the command manually
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
