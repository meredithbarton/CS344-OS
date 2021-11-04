#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// set currState as a global variable, since this value is intended to 
// be shared amongst processes 
struct stateValues *currState;

struct input
{
    char *command; // the first word of the full command
    char *args[512];   // all arguments given in full command
    int argCount;   // number of arguments given
    char *formattedCommand[514]; // the command entered into prompt (for use with exec functions)
    /* the formattedCommand exists because execvp() cannot process <,>,& correctly, and needs the 
    full argument intended to be entered, including the inital command. Having this array is a collection
    of only execvp()-approved values
    */
    bool bg_command; // 0 if not background command
};

struct stateValues
{
    int status;
    bool bg_mode;  // background mode, 0 is off, !0 is on
    int pid;
    pid_t bg_children[20];
    int bgChildStatus[20];
    int bg_population;
};

char *expansion(char *inputSegment)
{
    int pid = getpid();
    char *saveptr;
        /* citation https://unix.stackexchange.com/questions/16883/
        Instruction on how to find what os1 has max pid set to be. 
        Using this value, 327680, I know the expanded inputSegment will need no
        more than 6 - 2 + 1 (=number of digits - number of "$$" + 1) more memory space 
        */
    char *expandedSegment = calloc(strlen(inputSegment)+5, sizeof(char));
    // check if $$ stands alone as an argument
    // the else if block below only works when $$ is part of an argument 
    if (strcmp(inputSegment, "$$") == 0){
        sprintf(expandedSegment, "%d", pid);
    }
    else if (strstr(inputSegment, "$$") != NULL){
        char *token = strtok_r(inputSegment, "$$", &saveptr);
        bool expansion_finished = 0;
        while (!expansion_finished){
            sprintf(expandedSegment, "%s%d", token, pid);
            if (strstr(inputSegment, "$$") == NULL){
                expansion_finished = 1;
            }
            else{
                expandedSegment = calloc(strlen(expandedSegment)+5, sizeof(char));
            }
        }
    }
    else{
        strcpy(expandedSegment, inputSegment);
    }
    return expandedSegment;
}

void formatProcessCommand(struct input *userInput){
    int formattedCommandindex = 1;
    for (int x=0; x<userInput->argCount; x++){
        if ((strcmp(userInput->args[x],"<") != 0) && 
            (strcmp(userInput->args[x],">") != 0) &&
            (strcmp(userInput->args[x],"&") != 0)){
            userInput->formattedCommand[formattedCommandindex] = calloc(strlen(userInput->args[x])+1, sizeof(char));
            strcpy(userInput->formattedCommand[formattedCommandindex], userInput->args[x]);

            formattedCommandindex++;
        }
        else{
            // do not add the special character, nor the following arguement 
            /* why: the execvp function will need a list of the full input,
            but is not made to handle special characters. The argument following
            the special character is where the input/output should be. While execvp()
            can process the argument, it will not treat the argument as a src/destination
            and the command will not perform as intended. 
            By incrementing x twice, we skip both the special character and the src/dest filename
            */
            x++;
        }
    }
}

int checkBackground(){
    int ret = 0;
    int bgChildStatus; 
    for (int x=0; x < currState->bg_population; x++){
        if (currState->bg_children[x] > -1){
            if (waitpid(currState->bg_children[x], &bgChildStatus, WNOHANG) > 1){
                // the pid of the child has been returned to the array
                printf("background pid %d is done: ", currState->bg_children[x]);
                printf("terminated by signal %d\n", bgChildStatus);
                fflush(stdout);
                currState->bg_children[x] = -1;
                ret = 1;
            }
        }
    }
    return ret;
}

struct input *getInput(char *rawInput)
{
    struct input *newInput = malloc(sizeof(struct input));

    // remove \n from input
    int inputLen = strlen(rawInput);
    // don't remove end of blank entry,inputLen-1 will break on blank entry
    if (inputLen > 1){
        rawInput[inputLen-1] = '\0';
    }

    // parse input into struct, separating command and arguments
    char *saveptr;
    char *inputSegment = strtok_r(rawInput, " ", &saveptr);
    // allow for expansion with $$
    char *expandedSegment = calloc(2048, sizeof(char));
    expandedSegment = expansion(inputSegment);
    newInput->command = calloc(strlen(expandedSegment)+1, sizeof(char));
    strcpy(newInput->command, expandedSegment);
    newInput->formattedCommand[0] = calloc(strlen(expandedSegment)+1, sizeof(char));
    strcpy(newInput->formattedCommand[0], expandedSegment);

    // load arguments into the args array
    int index = 0;

    inputSegment = strtok_r(NULL, " ", &saveptr);
    while (inputSegment != NULL){
        // allow for expansion with $$
        char *expandedSegment = calloc(2048, sizeof(char));
        expandedSegment = expansion(inputSegment);
        // catalogue all arguments 
        newInput->args[index] = calloc(strlen(expandedSegment)+1, sizeof(char));
        strcpy(newInput->args[index], expandedSegment);
        inputSegment = strtok_r(NULL, " ", &saveptr);
        index++;
    } 

    // check whether it's a background process
    if (strcmp(newInput->args[index-1], "&") == 0){
        // set bg_command to true
        newInput->bg_command = 1;
    }
    else{
        newInput->bg_command = 0;
    }

    newInput->argCount = index;

    formatProcessCommand(newInput);
    
    return newInput;
}

int CheckBuiltIns(struct input *userInput)
{
     // check for status command
    if (strcmp(userInput->command, "status") == 0){
        printf("Exit value %d\n", currState->status);
        return 0;
        }

    // check for cd command
    else if (strcmp(userInput->command, "cd") == 0) {
        if (userInput->args[0] == NULL){
            /* citation: https://stackoverflow.com/questions/9493234/chdir-to-home-directory
            Instruction on how to navigate to the home directory */
            char *homeDir = getenv("HOME");
            if (homeDir != NULL){
                // getenv returns NULL if env not found
                chdir(homeDir);
            }
            else{
                printf("Error navigating to the home directory\n");
            }
        }
        else{
            // navigate to directory in first argumnent    
            int chdirCheck = chdir(userInput->args[0]);
            if (chdirCheck == 0){
                printf("Now in %s\n", userInput->args[0]);
            }
            else{
                printf("Error changing to %s directory\n", userInput->args[0]);
            }              
        }
        return 0;
    }
    return -1;
}

int ioHandling(struct input *userInput){
    int result = 1;
    bool outputRedirected = 0;
    bool inputRedirected = 0;
    bool inputFound = 0;
    bool outputFound = 0;
    int ret = 0;
    for (int x=0; x < userInput->argCount; x++){

        if (strcmp(userInput->args[x], "<") == 0){
            inputFound = 1;
            int sourceFD = open(userInput->args[x+1], O_RDONLY);
            if (sourceFD == -1){
                printf("Error opening %s\n", userInput->args[x+1]);
                currState->status = 1;
            }
            else{
                result = dup2(sourceFD, 0);
                if (result == -1){
                    printf("Error in assigning input source\n");
                    currState->status = 1;
                }
                else{
                    inputRedirected = 1;
                }
            }
        }

        if (strcmp(userInput->args[x], ">") == 0){
            outputFound = 1;
            int destFD = open(userInput->args[x+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (destFD == -1){
                printf("Error opening %s\n", userInput->args[x+1]);
                currState->status = 1;
            }
            else{
                result = dup2(destFD, 1);
                if (result == -1){
                    printf("Error in assigning output destination\n");
                    currState->status = 1;
                }
                else{
                    outputRedirected = 1;
                }
            } 
        }
    }

    // if process is run in background and input/output not specified, designate to /dev/null
    if ((!outputRedirected) && (outputFound) && (userInput->bg_command != 0)){
        int bgDest = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (bgDest == -1){
            printf("Error opening /dev/null");
            currState->status = 1;
        }
        else{
            result = dup2(bgDest, 1);
            if (result == -1){
                printf("Error in assigning output destination\n");
                currState->status = 1;
            }
            else{
                outputRedirected = 1;
            }
        }
    }

    if((!inputRedirected) &&(inputFound) && (userInput->bg_command != 0)){
        int bgSrc = open("/dev/null", O_RDONLY);            
        if (bgSrc == -1){
            printf("Error opening /dev/null");
            currState->status = 1;
        }
        else{
            result = dup2(bgSrc, 0);
            if (result == -1){
                printf("Error in assigning input source\n");
                currState->status = 1;
            }
            else{
                inputRedirected = 1;
            }
        }
    }

    if ((inputFound && !inputRedirected) || (outputFound && !outputRedirected)){
        ret = -1;
    }
    return ret;
}

void executeMiscCommand(struct input *userInput, struct sigaction my_sigint){
    /* citation: The basis of this function is a union of Exploration: Process API and 
    the video lectures Dr. Brewster has available on YouTube. 
    */
    int bgChildStatus;
    pid_t spawnpid = -5;
    int childStatus;
    //int bgChildStatus;
    int childPid;
    int ioHandled;
    spawnpid = fork();
    switch(spawnpid){
        case -1:
            perror("fork() failed");
            exit(1);
            break;
        case 0:
            // code used by child process
            if (!userInput->bg_command){
                // SIGINT should not be ignored here
                my_sigint.sa_handler = SIG_DFL;
                // set my_signint as SIGINT signal
                sigaction(SIGINT, &my_sigint, NULL);
            }
             
            // check for and direct input and output. Returns -1 if failure occurs 
            ioHandled = ioHandling(userInput);
            if (ioHandled == -1){
                currState->status = 1;
                exit(1);
                break;
            }

            execvp(userInput->formattedCommand[0], userInput->formattedCommand);
            // error if execvp() isn't the end of the child
            perror("CHILD: exec failure");
            fflush(stdout);
            currState->status = 1;
            exit(1);
        default:
            // execute in background if signified
            if (userInput->bg_command && currState->bg_mode){
                // use WNOHANG to tell parent to not wait for this child
                // append to bg_children to be checked on later
                currState->bg_children[currState->bg_population] = spawnpid;
                currState->bg_population++;
                waitpid(spawnpid, &bgChildStatus, WNOHANG);
                printf("background pid is %d\n", spawnpid);
                fflush(stdout);
            }
            // otherwise, run in foreground
            else {
                childPid = waitpid(spawnpid, &childStatus, 0);
                if (WIFEXITED(childStatus)){
                  currState->status = WEXITSTATUS(childStatus);
                } else{
                    printf("Child %d exited abnormally due to signal %d\n", spawnpid, WTERMSIG(childStatus));
                }
            }
    }
}

void handleTSTP(){
    if (currState->bg_mode){
        printf("Foreground-only mode is now on (& is ignored)\n");
        currState->bg_mode = 0;
    } else{
        printf("Exiting foreground-only mode\n");
        currState->bg_mode = 1;
    }
}

int main()
{
    // currState initialized to status = 0 and background processes allowed
    currState = (struct stateValues *)malloc(sizeof(struct stateValues));
    currState->status = 0;
    currState->bg_mode = 1;
    currState->pid = getpid();
    currState->bg_population = 0;

    struct sigaction my_sigTSTP = {0};
    // handle the signal through custom handler
    my_sigTSTP.sa_handler = handleTSTP;
    // block other signals while my_sigint is running
    sigfillset(&my_sigTSTP.sa_mask);
    my_sigTSTP.sa_flags = SA_RESTART;
    // set my_signint as SIGINT signal
    sigaction(SIGTSTP, &my_sigTSTP, NULL);

    char *rawInput = malloc(2049*sizeof(char));
    struct input *newInput;
    bool exited = 0;  // 1 if shell should exit, 0 if cd or status, else -1
    
    while (!exited){
        // set sigint signal -- should be ignored everywhere except foreground children
        struct sigaction my_sigint = {0};
        // handle the signal by ignoring it
        my_sigint.sa_handler = SIG_IGN;
        // block other signals while my_sigint is running
        sigfillset(&my_sigint.sa_mask);
        my_sigint.sa_flags = 0;
        // set my_signint as SIGINT signal
        sigaction(SIGINT, &my_sigint, NULL);
        printf(": ");
        fflush(stdout);
        bool bg_ret = 0; // 0 for no background finished
        bg_ret = checkBackground();
        if (!bg_ret){
            // get input
            fgets(rawInput, 2048, stdin);
            newInput = getInput(rawInput);
            if ((strcmp(newInput->command, "") == 0) ||
            (strncmp(newInput->command, "#", 1) == 0)){
                continue;
            }
            // check for exit command
            if (strcmp(newInput->command, "exit") == 0){
                exited = 1;
                return 0;
            }
            CheckBuiltIns(newInput);
            // CheckBuiltIns will return -1 if the command is not exit or cd
            // This allows the program to determine whether to start child process
            if ((strcmp(newInput->command, "cd") != 0) && 
                (strcmp(newInput->command, "status") != 0) &&
                (!exited)){
                executeMiscCommand(newInput, my_sigint);
            }
        }
    }
    return 0;
}
