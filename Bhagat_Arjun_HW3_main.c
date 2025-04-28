/**************************************************************
 * Class::  CSC-415-01 Fall 2024
 * Name::  Arjun Bhagat
 * Student ID::  917129686
 * GitHub-Name::  smeerj
 * Project:: Assignment 3 – Simple Shell with Pipes
 *
 * File:: Bhagat_Arjun_HW3_main.c
 *
 * Description::
 * A simple shell that supports piping.
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAX_CMD_LENGTH 159
#define MAX_ARGS (MAX_CMD_LENGTH - 1) / 2
#define MAX_PIPES MAX_CMD_LENGTH / 2

void executeCommands(char *input)
{
    // Initialize array to store command strings, and a counter for the number
    // of arguments
    char *commands[MAX_ARGS];
    int numCommands = 0;

    // Tokenize the input with the pipe character as the delimiter
    commands[numCommands] = strtok(input, "|");
    while (commands[numCommands] != NULL)
    {
        numCommands++;
        commands[numCommands] = strtok(NULL, "|");
    }

    // Array of file descriptors for the pipes
    int pipeFDS[2 * (numCommands - 1)];

    // For the number of commands found, create the pipes
    for (int i = 0; i < numCommands - 1; i++)
    {
        if (pipe(pipeFDS + i * 2) < 0)
        {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numCommands; i++)
    {
        pid_t PID = fork();

        if (PID < 0)
        {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (PID == 0)
        {
            // If not at the first command, redirect input from the previous
            // command
            int readEnd = pipeFDS[(i - 1) * 2];
            if (i > 0)
            {
                // Read the end of the previous pipe
                dup2(readEnd, STDIN_FILENO);
            }

            // If not at the last command, redirect output to the pipe of
            // the next command
            int writeEnd = pipeFDS[i * 2 + 1];
            if (i < numCommands - 1)
            {
                // Write to the end of the current pipe
                dup2(writeEnd, STDOUT_FILENO);
            }

            // Close all pipe file descriptors for the child process
            for (int j = 0; j < 2 * (numCommands - 1); j++)
            {
                close(pipeFDS[j]);
            }

            // Tokenize the command into arguments with space as the delimiter
            char *args[MAX_ARGS];
            int arg_count = 0;
            char *token = strtok(commands[i], " ");
            while (token != NULL && arg_count < MAX_ARGS - 1)
            {
                args[arg_count++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_count] = NULL;

            // Execute the command
            execvp(args[0], args);
            perror("exec failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process: Close all pipe file descriptors
    for (int i = 0; i < 2 * (numCommands - 1); i++)
    {
        close(pipeFDS[i]);
    }

    // Wait for all child processes to complete and print their PIDs
    // and return statuses
    for (int i = 0; i < numCommands; i++)
    {
        int status;
        pid_t childPID = wait(&status);
        printf("Child PID: %d, ", childPID);
        printf("Return status: %d\n", WEXITSTATUS(status));
    }
}

int main(int argc, char *argv[])
{
    char command[MAX_CMD_LENGTH];

    // Set default command prompt to "<", and change it to the given
    // prompt if one was provided
    char *cmdPrompt = "> ";
    if (argc > 1)
    {
        cmdPrompt = argv[1];
    }

    while (true)
    {
        printf("%s", cmdPrompt);

        // Read a line of input from the user
        if (fgets(command, MAX_CMD_LENGTH, stdin) == NULL)
        {
            // Handle EOF or error
            if (feof(stdin))
            {
                break;
            }
            else
            {
                perror("Error reading input");
                exit(EXIT_FAILURE);
            }
        }

        // Removes new line if it exists in the last command argument
        int commandLen = strlen(command);
        if (command[commandLen - 1] == '\n')
        {
            command[commandLen - 1] = '\0';
        }

        // If user types 'exit', then exit out
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        executeCommands(command);
    }

    return 0;
}
