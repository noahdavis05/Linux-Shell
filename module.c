#include "module.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <termios.h>

void runCommand( char *buf);
int redirectionCommand(char *args_pointers[ARG_LIMIT], int args, int redirect_right_arg, int redirect_left_arg);


void clearTerminal(){
    pid_t pid = fork();
    if (pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0){
        // in child process
        char *args[] = {"clear", NULL};
        execvp("clear", args);
        perror("Execvp");
        exit(EXIT_FAILURE);
    } else {
        // parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

void printWelcome(){
    printf(
"  ##  ##                      ##        ##            \n"
"  ### ##                     ##        ##             \n"
" ######     ####     #####   #####              ##### \n"
" ## ###   ##   ##  ##   ##  ##   ##            ##     \n"
" ##  ##  ##    ## ##    ##  ##   ##             ##    \n"
"##  ##   ##   ##  ##  ###  ##   ##               ##   \n"
"##  ##    ####     ### ##  ##   ##           #####    \n"
"                                                      \n"
"   ####     ##                ###      ###            \n"
"  ##  ##   ##                  ##       ##   \n"
"  ##       #####     ####     ##       ##    \n"
"   ##     ##   ##  ##   ##    ##       ##    \n"
"    ##    ##   ## ########   ##       ##     \n"
"##  ##   ##   ##  ##         ##       ##     \n"
" ####    ##   ##   ####     ####     ####    \n"
"                                             \n"
    );
}

void printPrompt(){
    // print out directory then >>>
    char dir_buffer[1000];
    if (getcwd(dir_buffer, sizeof(dir_buffer)) != NULL){
        printf("%s>>> ", dir_buffer);
    } else {
        perror("getcwd failed");
        exit(EXIT_FAILURE);
    }
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


int sequentialCommand(char *args_pointers[ARG_LIMIT], int args, char *second_command, int redirect_arg_right, int redirect_arg_left){
    if (redirect_arg_left > 0 || redirect_arg_right > 0){
        //printf("Redirection in sequential\n");
        redirectionCommand(args_pointers, args, redirect_arg_right, redirect_arg_left);
    } else{
        executeCommand(args_pointers, args);
    }   
    runCommand(second_command);
    return 0;
}


int redirectionCommand(char *args_pointers[ARG_LIMIT], int args, int redirect_right_arg, int redirect_left_arg){
    //printf("%d\n", redirect_right_arg);
    // ensure that second_command has no \n
    if (redirect_right_arg > 0 && redirect_left_arg > 0){
        char *filename_right = args_pointers[redirect_right_arg];
        char *filename_left = args_pointers[redirect_left_arg];
        // create a fork
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            return EXIT_FAILURE;
        }
        if (pid == 0){
            // in child process
            // open the left file and dup
            int fd_left = open(filename_left, O_RDONLY);
            if (fd_left < 0){
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd_left, STDIN_FILENO) == -1){
                perror("dup2 left");
                exit(EXIT_FAILURE);
            }
            close(fd_left);
            // open the right file and dup
            int fd_right = open(filename_right, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_right < 0){
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd_right, STDOUT_FILENO) == -1){
                perror("dup2 right");
                exit(EXIT_FAILURE);
            }
            close(fd_right);
            args_pointers[redirect_left_arg] = '\0';
            args_pointers[redirect_right_arg] = '\0';
            execvp(args_pointers[0], args_pointers);
            perror("exepvp failed");
            return EXIT_FAILURE;
        } else {
            // in parent process
            int status;
            waitpid(pid, &status, 0);
        }
        
    } else if (redirect_right_arg > 0){
        //printf("%d\n", redirect_right_arg);
        // just redirecting right
        // get the filename
        char *filename = args_pointers[redirect_right_arg];
        //printf("%s\n", filename);
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


void enableRawMode() {
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);          // Get current terminal attributes
    raw.c_lflag &= ~(ICANON | ECHO);        // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // Apply the new attributes
}

void disableRawMode() {
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);          // Get current terminal attributes
    raw.c_lflag |= (ICANON | ECHO);         // Enable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // Apply the new attributes
}

void makePrediction(char *predict_string){

}



void getUserInput(char *buf, int pos){
    while (1) {
        char c = getchar();  

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
        } else if (c == '\t'){
            buf[pos++] = '#';
            buf[pos] = '\0';
            printf("%c",'P'); 
        }else {
            buf[pos++] = c;  
            buf[pos] = '\0';  
            printf("%c", c);  
        }

        // here I can call a function to complete predictions
    }
}

void execute(struct commandTags command, char *args_pointers[ARG_LIMIT], int args){
    if (args < ARG_LIMIT){
        args_pointers[args] = NULL;
    } else{
        args_pointers[ARG_LIMIT] = NULL;
    }

    
    // execute commands
    if (command.sequential_command == 1){
        // check if redirection happens too
        sequentialCommand(args_pointers, args, command.second_command, command.redirect_arg_right, command.redirect_arg_left);
    } else if (command.pipe_command == 1){
        if (command.redirection_left == 1 || command.redirection_right == 1){}
        pipeCommand(args_pointers, command.second_command);
    } else if (command.redirection_left == 1 || command.redirect_arg_right){
        redirectionCommand(args_pointers, args, command.redirect_arg_right, command.redirect_arg_left);
    }  else {
        executeCommand(args_pointers, args);
    }
    // this removes extra command which is printed for an unknown reasonS
    fflush(stdout);
    printf("\033[2K\r");
    fflush(stdout);
}

