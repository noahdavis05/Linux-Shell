#ifndef MODULE_H
#define MODULE_H

#define BUF_SIZE 300
#define ARG_LIMIT 15
#define LEFT 0
#define RIGHT 1
#define MAX_ENTRIES 1024
#define HISTORY_FILE "history.txt"

// my Struct
struct commandTags {
    int pipe_command;
    int sequential_command;
    int redirection_left;
    int redirection_right;
    char *second_command;
    char *filename;
    int redirect_arg_right;
    int redirect_arg_left;
};

// function prototypes
void clearTerminal();
void printWelcome();
void printPrompt();
int executeCommand(char *args_pointers[ARG_LIMIT], int args);
int sequentialCommand(char *args_pointers[ARG_LIMIT], int args, char *second_command, int redirect_arg_right, int redirect_arg_left);
int redirectionCommand(char *args_pointers[ARG_LIMIT], int args, int redirect_right_arg, int redirect_left_arg);
int pipeCommand(char *args_pointers[ARG_LIMIT], char *second_command);
void enableRawMode();
void disableRawMode();
void getUserInput(char *buf, int pos);
void execute(struct commandTags command, char *args_pointers[ARG_LIMIT], int args);


#endif