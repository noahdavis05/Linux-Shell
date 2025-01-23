#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define BUF_SIZE 300
#define ARG_LIMIT 15

int getCommand(char *buf){
    // print out >>>
    printf(">>> ");
    // read the user input
    fgets(buf, BUF_SIZE, stdin);
    return 0;
}

int executeCommand(char *args_pointers[ARG_LIMIT], int args){
    //for (int i = 0; i < args; i++){
    //    printf("%s\n", args_pointers[i]);
    //}

    // null terminate array of pointers
    if (args < ARG_LIMIT){
        args_pointers[args] = NULL;
    } else{
        args_pointers[ARG_LIMIT] = NULL;
    }

    // make a fork
    pid_t pid = fork();

    // check fork succesful
    if (pid < 0){
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0){
        // in child process
        // replace current process with args_pointers[0]
        execvp(args_pointers[0], args_pointers);

        // if execvp fails, print an error and exit
        perror("execvp");
        return EXIT_FAILURE;
    } else {
        // in parent process
        int status;
        waitpid(pid, &status, 0); // wait for the child to complete
    }

    return 0;
}

void runCommand(char *buf){
    // make an array pointing to the start of each argument
    //printf("%s", buf);
    char *args_pointers[ARG_LIMIT];
    
    // iterate through the array and split the command into args
    int pos = 0;
    int args = 0;
    int prev_start = 0;
    // variables to detect redirection, pipes, and semi-colons
    int redirect_left = 0;
    int redirect_right = 0;
    int pipe_command = 0;
    int sequential_command = 0;
    char *second_command;
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
        if (buf[pos] == ' ' || buf[pos] == ';' || buf[pos] == '|' || buf[pos] == '>' || buf[pos] == '<'){
            // check what the input was
            if (buf[pos] == ';'){
                sequential_command = 1;
            } else if (buf[pos] == '|'){
                pipe_command = 1;
            } else if (buf[pos] == '>'){
                redirect_left = 1;
            } else if (buf[pos] == '<'){
                redirect_right = 1;
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
            if (sequential_command == 1 || pipe_command == 1 || redirect_left == 1 || redirect_right == 1){
                cont = 1;
                second_command = &buf[pos];
            } else {
                prev_start = pos;
            }
            

            
        } else {
            pos++;
        }
        
    }
    //printf("%d", args);
    //for (int i = 0; i < args; i++){
     //   printf("'%s'\n",args_pointers[i]);
    //}
    //printf("second command = '%s'", second_command);

    // execute commands
    if (sequential_command == 1){
        printf("Sequential\n");
    } else if (pipe_command == 1){
        printf("Pipe command\n");
    } else if (redirect_left == 1){
        printf("Redirection left");
    } else if (redirect_right == 1){
        printf("Redirection right\n");
    } else {
        executeCommand(args_pointers, args);
    }
}


int main(){
    printf("my shell\n");
    // firstly make the buf for commands
    char buf[BUF_SIZE];
    while (0 == 0){
        getCommand(buf);
        runCommand(buf);
    }
    return 0;
}