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
    char c;
    buf[pos] = '\0';

    printPrompt(); 
    fflush(stdout);  

    while (1) {
        c = getchar();  

        if (c == '\n') {  
            buf[pos] = '\n';  
            buf[pos + 1] = '\0';
            break;
        } else if (c == 127) {  
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                printf("\b \b");  
            }
        } else {
            buf[pos++] = c;  
            buf[pos] = '\0';  
            printf("%c", c);  
        }

        
    }
    
}

void runCommand(char *buf){
    // make an array pointing to the start of each argument
    char *args_pointers[ARG_LIMIT];
    int redirect_arg_right = 0;
    int redirect_arg_left = 0;
    // iterate through the array and split the command into args
    int pos = 0;
    int args = 0;
    int prev_start = 0;
    // to allow speech marks
    int speech_open = 0;
    // variables to detect redirection, pipes, and semi-colons
    int redirect_left = 0;
    int redirect_right = 0;
    int pipe_command = 0;
    int sequential_command = 0;
    char *second_command;
    char *redirect_filename;
    // for the loop condition
    int cont = 0; 
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
                sequential_command = 1;
            } else if (buf[pos] == '|'){
                pipe_command = 1;
            } else if (buf[pos] == '<'){
                redirect_left = 1;
                redirect_arg_left = args;
            } else if (buf[pos] == '>'){
                redirect_right = 1;
                redirect_arg_right = args;
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
            if (sequential_command == 1 || pipe_command == 1){
                cont = 1;
                second_command = &buf[pos];

            } else if (redirect_left == 1){
                prev_start = pos;
            } else if (redirect_right == 1){           
                prev_start = pos;
            }else {
                prev_start = pos;
            }     
        } else {
            pos++;
        }
        
    }

    if (args < ARG_LIMIT){
        args_pointers[args] = NULL;
    } else{
        args_pointers[ARG_LIMIT] = NULL;
    }

    
    // execute commands
    if (sequential_command == 1){
        // check if redirection happens too
        sequentialCommand(args_pointers, args, second_command, redirect_arg_right, redirect_arg_left);
    } else if (pipe_command == 1){
        if (redirect_left == 1 || redirect_right == 1){
            //printf("Pipe command with redirection");
        }
        pipeCommand(args_pointers, second_command);
    } else if (redirect_left == 1 || redirect_arg_right){
        redirectionCommand(args_pointers, args, redirect_arg_right, redirect_arg_left);
    }  else {
        executeCommand(args_pointers, args);
    }
    // this removes extra command which is printed for an unknown reasonS
    fflush(stdout);
    printf("\033[2K\r");
    fflush(stdout);
}


int main(){
    clearTerminal();
    printWelcome();
    // firstly make the buf for commands
    char buf[BUF_SIZE];
    while (0 == 0){
        getCommand(buf);
        //printf("¬%s¬", buf);
        runCommand(buf);
        memset(buf, 0, sizeof(buf));

    }
    return 0;
}