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
//extern char prompt[];
jobs_t *head = NULL;
pid_t pid = 0;
extern int last_status;
extern char prompt[];
char *external_commands[155]; // array to store external commands
char command[64];             // buffer to store command name
//builtin commands
char *builtins[] = {
    "echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs",
    "let", "eval", "set", "unset", "export", "declare", "typeset",
    "readonly", "getopts", "source", "exit", "exec", "shopt",
    "caller", "true", "type", "hash", "bind", "help",
    "fg", "bg", "jobs",
    NULL
};
// Extract first word (command) from input string
char *get_command(char *input_string)
{
    int i = 0, j = 0;
    static char command_local[50];
    while(input_string[i]==' '||input_string[i]=='\t') //skip leading spaces
    i++;
    while(input_string[i]!=' ' && input_string[i]!='\n' && input_string[i]!='\t') //fetch first word
    command_local[j++] = input_string[i++];
    command_local[j] = '\0';
    return command_local;
}
//read cmd from file and store it in array
void extract_external_commands(char **external_commands)
{
    int fp = open("external.txt", O_RDONLY);
    if (fp == -1) 
    {
        perror("error opening file");
        return;
    }
    char buffer[4096];
    int n = read(fp, buffer, 4095);
    if (n < 0)
    {
        write(2, "read failed\n", 12);
        close(fp);
        exit(1);
    }
    buffer[n] = '\0';
    close(fp);
    int i = 0, j = 0, k = 0;
    char temp[100];
    while (i < n)
    {
        char ch = buffer[i];
        if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r')
        {
            if (k > 0)
            {
                temp[k] = '\0';
                external_commands[j] = malloc(k + 1);
                strcpy(external_commands[j], temp);

                j++;
                k = 0;
            }
        }
        else
            temp[k++] = ch;

        i++;
    }
    if (k > 0)
    {
        temp[k] = '\0';
        external_commands[j] = malloc(k + 1);
        strcpy(external_commands[j], temp);
        j++;
    }
    external_commands[j] = NULL;
}
int check_command_type(char *command)
{
    for(int i=0; builtins[i]!=NULL; i++)
    {
        if(strcmp(command,builtins[i])==0)
            return BUILTIN;
    }
    for(int i=0; external_commands[i]!=NULL; i++)
    {
        if(strcmp(command,external_commands[i])==0)
            return EXTERNAL;
    }
    return NO_COMMAND;
}
int is_pipe_present(char *cmd)
{
    return strchr(cmd, '|') != NULL;
}
void execute_external_commands(char *input_string)
{
    char *commands[20];
    int pipe_count = 0;
    char buffer[1024];
    strncpy(buffer, input_string, 1024);
    buffer[1024 - 1] = '\0';
    char *tok = strtok(buffer, "|");
    while (tok != NULL && pipe_count < 20)
    {
        while (*tok == ' ')
        tok++;
        commands[pipe_count++] = tok;
        tok = strtok(NULL, "|");
    }
    int pipes[pipe_count - 1][2];
    for (int i = 0; i < pipe_count - 1; i++)
    {
        if (pipe(pipes[i]) < 0)
        {
            perror("pipe");
            exit(1);
        }
    }
    for (int i = 0; i < pipe_count; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            if (i > 0)
            dup2(pipes[i - 1][0], 0);
            if (i < pipe_count - 1)
            dup2(pipes[i][1], 1);
            for (int j = 0; j < pipe_count - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            char *argv[64];
            int argc = 0;
            char *arg = strtok(commands[i], " ");
            while (arg != NULL && argc < 63)
            {
                argv[argc++] = arg;
                arg = strtok(NULL, " ");
            }
            argv[argc] = NULL;
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(1);
        }
    }
    for (int i = 0; i < pipe_count - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for (int i = 0; i < pipe_count; i++)
        wait(NULL);
}
//INTERNAL COMMANDS
int execute_internal_commands(char *input_string)
{
    char buffer[100];
    // 1. pwd
    if (strcmp("pwd", input_string) == 0)
    {
        getcwd(buffer, sizeof(buffer));
        printf("%s\n", buffer);
        return 0;
    }
    // 2. exit
    else if (strcmp("exit", input_string) == 0)
    {
        exit(0);
    }
    // 3. cd
    else if (strcmp("cd", input_string) == 0)
    {
        getcwd(buffer, sizeof(buffer));
        printf("%s\n", buffer);
        return 0;
    }
    // 4. echo $?
    else if (strcmp("echo $?", input_string) == 0)
    {
        if (WIFEXITED(last_status))
        printf("%d\n", WEXITSTATUS(last_status));
        else
        printf("Abnormal termination  OR invalid command %d\n", last_status);
        return 0;
    }
    // 5. echo $$
    else if (strcmp("echo $$", input_string) == 0)
    {
        printf("current pid is :%d\n", getpid());
        return 0;
    }
    // 6. echo $SHELL
    else if (strcmp("echo $SHELL", input_string) == 0)
    {
        char *shell = getenv("SHELL");
        if (shell)
        printf("%s\n", shell);
        else
        printf("SHELL not set\n");
        return 0;
    }
    // 7. jobs
    else if (strcmp("jobs", input_string) == 0)
    {
        jobs_t *temp = head;
        int job_id = 1;
        while (temp)
        {
            // Print actual command stored in struct
            printf("[%d] Stopped %d %s\n",
            job_id++, temp->pid, temp->command);
            temp = temp->link;
        }
        return 0;
    }
    // 8.fg
    else if (strcmp("fg", input_string) == 0)
    {
        if (head == NULL)
        {
            printf("fg: no jobs\n");
            return 1;
        }
        // Remove job from the list
        jobs_t *job = head;
        head = head->link;
        pid = job->pid;               // set as foreground pid
        printf("%s\n", job->command); // show which job is resumed
        // Continue the stopped process
        kill(pid, SIGCONT);
        int status;
        waitpid(pid, &status, WUNTRACED); // wait for stop or exit
        delete_first(&head);
        // If stopped again, re-insert into job list
        //if (WIFSTOPPED(status))
        //insert_at_first(&head, pid, job->command);
        //pid = 0; // reset foreground pid
        free(job);
        // Print prompt again
        //printf("%s ", prompt);
        fflush(stdout);
        return 0;
    }
    // 9. bg
    else if (strcmp("bg", input_string) == 0)
    {
        if (head == NULL)
        {
            printf("bg: no jobs\n");
            return 1;
        }
        jobs_t *job = head;   // next stopped job (older after fg)
        head = head->link;
        kill(-job->pid, SIGCONT);
        delete_first(&head);
        printf("[%d] %s &\n", job->pid, job->command);
        free(job);
        return 0;
    }

}
