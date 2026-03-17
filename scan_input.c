#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <stdio_ext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "main.h"
extern int pid;
extern jobs_t *head;
int last_status = 0;
extern char prompt[];
//GLOBAL VARIABLE 
// Stores the last command executed so we can save it when Ctrl+Z is pressed
char last_input[1024];  
void scan_input(char *prompt, char *input_string)
{
    //register signal handlers
    signal(SIGINT, my_handler);
    signal(SIGTSTP, my_handler);
    signal(SIGCHLD, my_handler);
    while (1)
    {
        printf("%s ", prompt);
        fflush(stdout);
        if (fgets(input_string, 1024, stdin) == NULL)
        {
            printf("\n");
            exit(0);  // Ctrl+D pressed
        }
        // remove trailing newline
        input_string[strcspn(input_string, "\n")] = 0;
        // store the command in global variable
        strcpy(last_input, input_string);
        // change shell prompt 
       if (strncmp(input_string, "PS1=", 4) == 0)
        { 
            if (input_string[4] == '\0') 
            { 
                printf("invalid prompt\n"); 
                continue; 
            } 
            strcpy(prompt, input_string + 4);
            continue; 
        } 
        else if (strncmp(input_string, "PS1", 3) == 0 && input_string[3] != '=')
        { 
            printf("invalid prompt\n");
            continue; 
        }
        else
        {
            char *cmd = get_command(input_string);
            int type = check_command_type(cmd);
            if (type == BUILTIN)
            {
                printf("it is internal cmd\n");
                int ret = execute_internal_commands(input_string);
                if (strcmp(input_string, "echo $?") != 0 &&
                    strcmp(input_string, "echo $$") != 0 &&
                    strcmp(input_string, "echo $SHELL") != 0 &&
                    strcmp(input_string, "echo $") != 0)
                {
                    last_status = ret;
                }
            }
            else if (type == EXTERNAL)
            {
                pid = fork();
                if (pid < 0)
                {
                    perror("fork failed");
                    
                }
                else if (pid == 0) // child
                {
                    //restore default signal handling for child
                    signal(SIGINT, SIG_DFL);   // Ctrl+C kills child
                    signal(SIGTSTP, SIG_DFL);  // Ctrl+Z stops child
                    execute_external_commands(input_string);
                    exit(1);
                }
                else // parent
                {
                    int status;
                    waitpid(pid, &status, WUNTRACED);
                    if (WIFEXITED(status))
                    last_status = WEXITSTATUS(status);
                    //pid = 0;
                }
            }
            else
            {
                printf("%s: command not found\n", cmd);
                last_status = 127;
            }
        }
        pid=0;
    }
}
void my_handler(int signum)
{
    int status;
    if (signum == SIGINT)
    {
        if(pid==0)
        {
        // Ctrl+C pressed
        printf("\n");
        printf("%s ", prompt);  // print prompt again
        fflush(stdout);
      }
    }
    else if (signum == SIGTSTP)
    {
        if (pid == 0)
        {
            // Ctrl+Z pressed in shell (no foreground process)
            printf("\n");
            printf("%s ", prompt);
            fflush(stdout);
        }
        else
        {
            // Stop the foreground process
            kill(pid, SIGSTOP);
            // Save stopped process in job list
            insert_at_first(&head, pid, last_input);
            printf("\n");
            //pid = 0;  // reset foreground pid
        }
    }
    else if (signum == SIGCHLD)
    {
        // Reap terminated background children
        while (waitpid(-1, &status, WNOHANG) > 0);
    }
}
int insert_at_first(jobs_t **head, pid_t pid, char *cmd)
{
    jobs_t *new;
    new = malloc(sizeof(jobs_t));
    if (new == NULL)
    {
        return -1; // malloc failed
    }
    new->pid = pid;
    // Copy the command name into the struct
    if (cmd != NULL)
    strncpy(new->command, cmd, sizeof(new->command) - 1);
    new->command[sizeof(new->command) - 1] = '\0'; // ensure null termination
    new->link = *head;
    *head = new;
    return 0; // success
}
int delete_first(jobs_t **head)
{
    if (*head == NULL)
    {
        return -1;
    }
    else
    {
        jobs_t *temp = *head;
        *head = temp->link;
        free(temp);
        return 0;
    }
}
