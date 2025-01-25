#ifndef MODULE_H
#define MODULE_H

#define BUF_SIZE 300
#define ARG_LIMIT 15
#define LEFT 0
#define RIGHT 1
// function prototypes
void printWelcome();
void printPrompt();
int executeCommand(char *args_pointers[ARG_LIMIT], int args);
int sequentialCommand(char *args_pointers[ARG_LIMIT], int args, char *second_command, int redirect_arg_right, int redirect_arg_left);
int redirectionCommand(char *args_pointers[ARG_LIMIT], int args, int redirect_right_arg, int redirect_left_arg);
int pipeCommand(char *args_pointers[ARG_LIMIT], char *second_command);

#endif