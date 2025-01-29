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

void resetHistory(){
    FILE *file = fopen(HISTORY_FILE, "w");
    fclose(file);
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
            //printf("!%s!\n",args_pointers[1]);
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

    tcgetattr(STDIN_FILENO, &raw);         
    raw.c_lflag &= ~(ICANON | ECHO);       
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}

void disableRawMode() {
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);      
    raw.c_lflag |= (ICANON | ECHO);      
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}



void predictWord(char *word) {
    // Get all the files and directories in the current directory
    char bestMatch[BUF_SIZE] = "";
    DIR *folder;
    struct dirent *entry;
    int min_length = BUF_SIZE;

    folder = opendir(".");
    if (folder == NULL) {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }

    // Find the best match for the given word
    while ((entry = readdir(folder))) {
        if (strncmp(entry->d_name, word, strlen(word)) == 0) {
            if (strlen(entry->d_name) < min_length){
                strncpy(bestMatch, entry->d_name, BUF_SIZE - 1);
                bestMatch[BUF_SIZE - 1] = '\0'; 
                min_length = strlen(entry->d_name);
            }

            
        }
    }

    closedir(folder);

    // Print the best match in light gray
    if (min_length < BUF_SIZE) {
        printf("\033[s");              
        printf("\033[K");              
        printf("\033[90m%s\033[0m", &bestMatch[strlen(word)]); 
        printf("\033[u");              
        fflush(stdout);
    } else {
        printf("\033[s");              
        printf("\033[K");
    }
}

void makePrediction(char *buf) {
    // Extract the current word being typed
    char *word = &buf[0];
    int i = 0;
    int prev_start = 0;

    while (buf[i] != '\0') {
        
        if (buf[i] == ' ' || buf[i] == '>' || buf[i] == '<' || buf[i] == ';' || buf[i] == '|') {
            prev_start = i + 1;
        } else if(buf[i] == '/' && buf[i-1] == '.'){
            prev_start = i + 1;
        }
        i++;
    }

    word = &buf[prev_start];
    predictWord(word); // Make a prediction based on the extracted word
}

int chooseAndWriteWord(char *buf, int pos, char *word){
    // Get all the files and directories in the current directory
    char bestMatch[BUF_SIZE] = "";
    DIR *folder;
    struct dirent *entry;
    int min_length = BUF_SIZE;

    folder = opendir(".");
    if (folder == NULL) {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }

    // Find the best match for the given word
    while ((entry = readdir(folder))) {
        if (strcmp(entry->d_name, ".") == 0){continue;}
        if (strncmp(entry->d_name, word, strlen(word)) == 0) {
            if (strlen(entry->d_name) < min_length){
                strncpy(bestMatch, entry->d_name, BUF_SIZE - 1);
                bestMatch[strlen(entry->d_name)] = '\0'; 
                min_length = strlen(entry->d_name);
            }

            
        }
    }

    closedir(folder);
    // now write word to buf and command line in white
    pos -= strlen(word);
    // from here we can write the predicted word to command line and buf
    int i;
    for (i = 0; i < strlen(bestMatch); i++){
        buf[pos + i] = bestMatch[i];
        //printf("%d\n", i);
    }
    pos += i;
    pos;
    //buf[pos] = '\0';
    // write to command line
    printf("\033[2K\r");//clear
    printPrompt();
    printf("%s", buf);
    fflush(stdout);
    return pos;
}

int autoFill(char *buf, int pos){
    // Extract the current word being typed
    char *word = &buf[0];
    int i = 0;
    int prev_start = 0;

    while (buf[i] != '\0') {
        
        if (buf[i] == ' ' || buf[i] == '>' || buf[i] == '<' || buf[i] == ';' || buf[i] == '|') {
            prev_start = i + 1;
        } else if(buf[i] == '/' && buf[i-1] == '.'){
            prev_start = i + 1;
        }
        i++;
    }

    word = &buf[prev_start];
    // now need to make the prediction
    return chooseAndWriteWord(buf, pos, word);
}


void writeHistory(char *buf){
    // check file hasn't got more than 500 entries
    FILE *file = fopen(HISTORY_FILE, "a");
    fprintf(file,"%s", buf);
    fclose(file);
}

int linesInHistory(){
    FILE *file = fopen(HISTORY_FILE, "r");
    int lines = 0;
    char c;
    for (c = getc(file); c!=EOF; c = getc(file)){
        if (c == '\n'){
            lines++;
        }
    }
    fclose(file);
    return lines;
}

char *getHistoryCommand(int line){
    char *command = malloc(BUF_SIZE);
    char buffer[BUF_SIZE];
    FILE *file = fopen(HISTORY_FILE, "r");
    int lines = 0;

    while (fgets(buffer, BUF_SIZE, file)){
        if (lines == line){
            fclose(file);
            snprintf(command, BUF_SIZE, "%s", buffer);
            // iterate through command and change \n to \0
            int pos = 0;
            while (command[pos] != '\n' && pos < BUF_SIZE){
                pos++;
            }
            command[pos] = '\0';
            return command;
        }
        lines++;
    }

    fclose(file);
    free(command);
    return NULL;
}

int getBufLen(char *buf){
    int pos = 0;
    while (buf[pos] != '\0' && pos < BUF_SIZE){
        pos++;
    }
    return pos;
}


void getUserInput(char *buf, int pos) {
    int currentHistory = linesInHistory() - 1;
    int maxHistory = currentHistory;
    while (1) {
        char c = getchar();

        if (c == '\n') { // Handle Enter key
            //printf("1%s1\n", buf);
            buf[pos] = '\n';
            buf[pos + 1] = '\0';
            //printf("2%s2\n", buf);
            writeHistory(buf);
            currentHistory++;
            break;
        } else if (c == 127) { // Handle Backspace
            if (pos > 0) {
                pos--;                
                buf[pos] = '\0';      

                // Clear the current line
                printf("\033[2K\r");  

                // Rewrite the prompt and the updated buffer
                printPrompt();
                printf("%s", buf);
                fflush(stdout);       
            }
        } else if (c == '\t') { // Handle Tab key
            pos = autoFill(buf, pos);        
            //buf[pos] = '\0';
            //printf("&%s&", buf);
        } else if (c == '\033'){
            getchar();
            char arrow = getchar();
            if (arrow == 'A'){
                // up arrow
                //printf("Up");
                currentHistory--;
                if (currentHistory < 0){
                    currentHistory = 0;
                }
                
            } else if (arrow == 'B'){
                // down arrow
                //printf("Down");
                currentHistory++;
                if (currentHistory > maxHistory){
                    currentHistory = maxHistory;
                }
            }
            // get the command from history
            char *command = getHistoryCommand(currentHistory);
            //printf("%s\n", command);
            snprintf(buf, BUF_SIZE, "%s",command);
            free(command);
            // now clear line print command 
            printf("\033[2K\r");  
            //printf("%d",currentHistory);
            // Rewrite the prompt and the updated buffer
            printPrompt();
            printf("%s", buf);
            pos = getBufLen(buf);
            //fflush(stdout);   
        }else { // Handle Normal Input
            buf[pos++] = c;
            buf[pos] = '\0';
            printf("%c", c);          
        }

        // Call the function to make predictions based on the current buffer
        if (pos > 0 && c != '\t'){
            makePrediction(buf);
        }
        
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

}

