#ifndef MAIN_H
#define MAIN_H
#include <sys/types.h>
// command types 
#define BUILTIN  1
#define EXTERNAL 2
#define NO_COMMAND 3
//job structure 
typedef struct jobs
{
    pid_t pid;
    char command[100];
    struct jobs *link;
} jobs_t;
// globals
extern jobs_t *head;
extern pid_t pid;
extern int last_status;
extern char prompt[];
extern char *external_commands[155];
// scan
void scan_input(char *prompt, char *input_string);
//signal 
void my_handler(int signum);
//job helpers
int insert_at_first(jobs_t **head, pid_t pid, char *cmd);
int delete_first(jobs_t **head);
//command helpers
char *get_command(char *input_string);
void extract_external_commands(char **external_commands);
int check_command_type(char *command);
void execute_external_commands(char *input_string);
int execute_internal_commands(char *input_string);
#endif

