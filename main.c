#include<stdio.h>
#include<stdlib.h>   // for system()
#include "main.h"
char prompt[] = "minishell$";
extern char *external_commands[155];
int main()
{
    //load external commands list
    extract_external_commands(external_commands);
    system("clear");
    char input_string[1024];
    scan_input(prompt, input_string);
    return 0;
}
