#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>



#define BUF_SIZE 300
#define ARG_LIMIT 15
#define LEFT 0
#define RIGHT 1

// stub
void runCommand( char *buf);

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

    // check if the command is a cd 
    if (strcmp(args_pointers[0], "cd") == 0){
        if (args > 1){
            chdir(args_pointers[1]);
        } else {
            perror("No directory specified");
            return EXIT_FAILURE;
        }
        
    } else {

        // make a fork
        pid_t pid = fork();

        // check fork succesful
        if (pid < 0){
            perror("fork");
            exit(EXIT_FAILURE);
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
    }
    return 0;
}

int sequentialCommand(char *args_pointers[ARG_LIMIT], int args, char *second_command){
    executeCommand(args_pointers, args);
    runCommand(second_command);
    return 0;
}

int redirectionCommand(char *args_pointers[ARG_LIMIT], int args, int redirect_right_arg, int redirect_left_arg){
    // ensure that second_command has no \n
    if (redirect_right_arg > 0 && redirect_left_arg > 0){
        // both redirections
    } else if (redirect_right_arg > 0){
        // just redirecting right
        // get the filename
        char *filename = args_pointers[redirect_right_arg];
        // create a fork
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            return EXIT_FAILURE;
        }
        if (pid == 0){
            // in child process
            int fd = syscall(SYS_open, filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1){
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }
            if (close(STDOUT_FILENO) == -1){
                perror("error closing stdout");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) == -1){
                perror("dup2");
                close(fd);
                exit(EXIT_FAILURE);
            }
            // successful so close file and execute command
            if (syscall(SYS_close, fd) == -1){
                perror("Error closing file");
                exit(EXIT_FAILURE);
            }
            // get rid of the filename from the arguments
            args_pointers[redirect_right_arg] = '\0';
            execvp(args_pointers[0], args_pointers);
            perror("exepvp failed");
            return EXIT_FAILURE;
        } else {
            // in parent process
            int status;
            waitpid(pid, &status, 0);
        }

    } else {
        // just redirecting left
        // get the filename
        char *filename = args_pointers[redirect_left_arg];
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            return EXIT_FAILURE;
        }
        if (pid == 0){
            // in child process
            int fd = syscall(SYS_open, filename, O_RDONLY);
            if (fd == -1) {
                perror("open for input redirection");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2 for input redirection");
                close(fd);
                exit(EXIT_FAILURE);
            }
            // successful so close file and execute command
            if (syscall(SYS_close, fd) == -1){
                perror("Error closing file");
                exit(EXIT_FAILURE);
            }

            execvp(args_pointers[0], args_pointers);
            perror("exepvp failed");
            return EXIT_FAILURE;
        } else {
            // in parent process
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}

int pipeCommand(char *args_pointers[ARG_LIMIT], char *second_command){
    // create a pipe
    int pcp[2];
    if (pipe(pcp) == -1){
        perror("Pipe failed");
        return EXIT_FAILURE;
    }
    // fork for the left command
    pid_t pid1 = fork();
    if (pid1 == 0){
        // in child process
        // close stdout
        close(pcp[0]);
        if (dup2(pcp[1], STDOUT_FILENO) == -1){
            perror("dup2 pipe1");
            close(pcp[1]);
            exit(EXIT_FAILURE);
        }
        close(pcp[1]);
        if (execvp(args_pointers[0], args_pointers) == -1){
            perror("execvp");
            close(pcp[1]);
            exit(EXIT_FAILURE);
        }
    } else if (pid1 < 0){
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    

    // now fork for the right hand command which will read from pipe
    pid_t pid2 = fork();
    if (pid2 == 0){
        close(pcp[1]);
        if (dup2(pcp[0], STDIN_FILENO) == -1){
            perror("dup2 pipe 2");
            close(pcp[0]);
            exit(EXIT_FAILURE);
        }
        close(pcp[0]);
        //printf("Entered fork 2");
        // now call run command function for the second command
        runCommand(second_command);
        exit(EXIT_FAILURE);
    }

    close(pcp[0]);
    close(pcp[1]);


    int status1;
    waitpid(pid1, &status1,0);
    int status2;
    waitpid(pid2, &status2, 0);
    return 0;
}

void runCommand(char *buf){
    // make an array pointing to the start of each argument
    //printf("%s", buf);
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
            } else if (buf[pos] == '>'){
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
            if (sequential_command == 1 || pipe_command == 1){
                cont = 1;
                second_command = &buf[pos];
            } else if (redirect_left == 1){
                redirect_arg_left = args;
                prev_start = pos;
            } else if (redirect_right == 1){
                redirect_arg_right = args;
                prev_start = pos;
            }else {
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
    // null terminate the arguments
    //printf("%d\n", args);
    if (args < ARG_LIMIT){
        args_pointers[args] = NULL;
    } else{
        args_pointers[ARG_LIMIT] = NULL;
    }

    

    // execute commands
    if (sequential_command == 1){
        sequentialCommand(args_pointers, args, second_command);
    } else if (pipe_command == 1){
        pipeCommand(args_pointers, second_command);
    } else if (redirect_left == 1 || redirect_arg_right){
        redirectionCommand(args_pointers, args, redirect_arg_right, redirect_arg_left);
    }  else {
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