#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include "module.h"


void getCommand(char *buf){
    int pos = 0;  
    buf[pos] = '\0';

    enableRawMode(); // needed to disable common input handling e.g tab automatically tabbing
    printPrompt(); 
    fflush(stdout);  
    getUserInput(buf, pos);
    disableRawMode();
    printf("\n");
}

void runCommand(char *buf){
    // make an array pointing to the start of each argument
    char *args_pointers[ARG_LIMIT];    
    int args = 0;
    
    // variables to detect redirection, pipes, and semi-colons
    struct commandTags command;
    command.pipe_command = 0;
    command.redirection_left = 0;
    command.redirection_right = 0;
    command.sequential_command = 0;
    command.redirect_arg_left = 0;
    command.redirect_arg_right = 0;
    

    // for the loop condition
    int cont = 0; 
    int pos = 0;
    int prev_start = 0;
    // to allow speech marks
    int speech_open = 0;
    // split the buf up into arguments
    while (cont == 0 && pos < BUF_SIZE){
        if (buf[pos] == '\n'){
            buf[pos] = '\0';
            cont = 1;
            // terminate the last arg
            args_pointers[args] = &buf[prev_start];
            args++;          
        }
        if (buf[pos] == '"'){
            if (speech_open == 0){
                speech_open = 1;
            } else{
                speech_open = 0;
            }
        }
        if ((buf[pos] == ' ' || buf[pos] == ';' || buf[pos] == '|' || buf[pos] == '>' || buf[pos] == '<') && speech_open == 0){
            // check what the input was
            if (buf[pos] == ';'){
                command.sequential_command = 1;
            } else if (buf[pos] == '|'){
                command.pipe_command = 1;
            } else if (buf[pos] == '<'){
                command.redirection_left = 1;
                command.redirect_arg_left = args;
            } else if (buf[pos] == '>'){
                command.redirection_right = 1;
                command.redirect_arg_right = args;
            }
            // this is the end of an argument
            if ( prev_start != pos){
                args_pointers[args] = &buf[prev_start];
                args++;
            }
            // so we can null terminate the argument
            buf[pos] = '\0';
            pos++;
            // increment and change other variables
            
            // iterate until buf[pos] is not a space
            while (buf[pos] == ' '){
                pos++;
            }
            // check we haven't reached the end
            if (buf[pos] == '\n'){
                buf[pos] = '\0';
                cont = 1;   
            }
            if (command.sequential_command == 1 || command.pipe_command == 1){
                cont = 1;
                command.second_command = &buf[pos];

            } else if (command.redirection_left == 1){
                prev_start = pos;
            } else if (command.redirection_right == 1){           
                prev_start = pos;
            }else {
                prev_start = pos;
            }     
        } else {
            pos++;
        }
        
    }

    execute(command, args_pointers, args);
}


int main(){
    clearTerminal();
    printWelcome();
    // firstly make the buf for commands
    char buf[BUF_SIZE];
    while (0 == 0){
        getCommand(buf);
        runCommand(buf);
        memset(buf, 0, sizeof(buf));

    }
    return 0;
}