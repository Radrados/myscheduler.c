#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

//CITS 2002 Project 1 2023
// Student 1:           23423175            RADOS MARKOVIC
// Student 2:           23367345            ADITYA PATIL

//CONSTANTS FOR SYSCONFING AND COMMAND FILES
#define MAX_DEVICES                            4
#define MAX_DEVICE_NAME                       20
#define MAX_COMMANDS                          10
#define MAX_COMMAND_NAME                      20
#define MAX_SYSCALLS_PER_PROCESS              40
#define MAX_RUNNING PROCESSES                 50
#define MAX_LINE_LENGTH                      100 // Assumption: no line in any file will be longer than 100 characters.
#define TIME_CONTEXT_SWITCH                    5
#define TIME_CORE_STATE_TRANSITIONS           10
#define TIME_ACQUIRE_BUS                      20
#define MAX_SYSCALLS_NAME                      6 // Assumption: max length for a syscall name is 6
#define EXEC_SYSCALL_TIME                      1 // time taken for the syscall to actually execute
#define CHAR_COMMENT                          '#'// comment start char in files
#define DEBUG                                  true//
#define MIN(a, b) ((a) < (b) ? (a) : (b))      // simple min function for 2 integers


////////////////////////////// STRUCTURES /////////////////////////////

//define command struct
typedef struct command command;
//define syscall struct
typedef struct syscalls syscalls;

//struct to contain Input/Output devices and their speeds
struct Device {
    //position in device array
    int deviceID;
    char name[MAX_DEVICE_NAME];
    int readSpeed;
    int writeSpeed;
}devices [MAX_DEVICES];

//Syscall fields stores Information on a single system call. Examples:

// Syscall       WHen             Name                StrValue                intValue
// Sleep        exec time        sleep                null                    sleepLength
// exit         exec time        exit                 null                    null
// spawn        exec time        spawn                child command           null
// wait         exec time        wait                 null                    null
// read         exec time        read                 device                  size
// write        exec time        write                device                  size
struct syscalls {
    //time since start of command when the syscall executes               //sleep:
    int when;                                                             // when
    char name[MAX_SYSCALLS_NAME];                                         // name
    //field to store different things based on what syscall it is        //
    char strValue[MAX_DEVICE_NAME];                                       // null
    //field to store different things based on what syscall it is          //
    int intValue;                                                         //
};

//struct containing all information in commands file
struct command {
    //position in commands array
    int commandID;
    //how long command has been executing on CPU
    int runtime;
    char name[MAX_COMMAND_NAME];
    //how many syscalls in command
    int syscallcount;
    //array containing all syscalls called by function
    syscalls syscallsArray[MAX_SYSCALLS_PER_PROCESS]; // Array of syscalls rand uring the process
    //index of currently executing syscall
    int currentsyscall;
};

// create struct of Node with two integers
typedef struct Node {
    int commandID;
    int secondaryValue;
    struct Node* next;
} Node;


typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;


/////////////////////global variables////////////////////

//Queue of commands ready to execute on CPU
Queue *readyQ;
//Queue of blocked commands sleeping
Queue *sleepingQ;
//array of ques containig the commands spawend by the command with the processID of the index of the queue
Queue waitingQ[MAX_COMMANDS];
//array of length MAC COMMANDS with true in the coresponding place if command is completed
bool completed[MAX_COMMANDS];
//max time a command can run on CPU before getting kicked off
int timeQuantum;
//array of commands from command file
command commands[MAX_COMMANDS];
//global clock
int clock;

////////////////////////// FUNCTIONS /////////////////////////////

//debug function
void debugprint(const char *fmt, ...) {
    if(DEBUG){
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

//create new node
Node* newNode(int commandID, int secondaryValue) {
    Node* temp = (Node*)malloc(sizeof(Node));
    if (!temp) {
        printf("Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    temp->commandID = commandID;
    temp-> secondaryValue = secondaryValue;
    temp->next = NULL;
    return temp;
}

// Function to create an empty queue
Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) {
        printf("Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    q->front = q->rear = NULL;
    return q;
}

//initialize array tupe queue to store kids of processes that called wait
void createWaitQueue() {
    for (int i = 0; i < MAX_COMMANDS; i++) {
        waitingQ[i].front = NULL;
        waitingQ[i].rear = NULL;
    }
}

// Function to check if the queue is empty
int isEmpty(Queue* q) {
    return q->front == NULL;
}

// function to add element to sleeping queue
void enqueue(Queue* q, int commandID) {////////////////////////////////////! fix issue
    Node* temp = newNode(commandID,-1);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

// Function to peek at the front element without removing it
int peek(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }
    return q->front->commandID;
}

// Function to remove an element from the queue
int dequeue(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }

    Node* temp = q->front;
    int commandID = temp->commandID;

    //NOT UPDATEING SECONDARY VALUE ITS SO CONFUSING AHHHH
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    return commandID;
}

// Function to remove a node with a specific commandID from a queue
bool removeNodeWithCommandID(Queue* q, int commandID) {
    if (q->front == NULL) return false;  // if the queue is empty, return false

    Node* current = q->front;
    Node* prev = NULL;

    while (current != NULL && current->commandID != commandID) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) return false;  // commandID not found in the queue

    // If the node to be removed is the first node
    if (current == q->front) {
        q->front = q->front->next;
    } else {
        prev->next = current->next;
    }

    // If the node to be removed is the last node
    if (current == q->rear) {
        q->rear = prev;
    }

    free(current);  // release the node
    return true;    // return true indicating the node was removed
}


void read_sysconfig(char argv0[], char filename[])
{
    FILE *file;
    char line[MAX_LINE_LENGTH];
    file = fopen(filename, "r");
    int device_count = 0;

    // check if file is unable to be read.
    if (file == NULL) {
        printf("Error: File %s was unable to be read. Please check your file and/or filename. \n", filename);
        exit(EXIT_FAILURE);
    }

    // proceed to read the entire file
    while (fgets(line, sizeof(line), file) != NULL) {
        // check for lines that don't with '#' as we need to ignore lines that do
        if (line[0] != CHAR_COMMENT) {
            char *found = strstr(line, "device");
            // if first line starts with the word 'device'
            if (found) {
                if (device_count >= MAX_DEVICES) {
                    printf("Error: Max device amount of '%i' has been exceeded.\n", MAX_DEVICES);
                    exit(EXIT_FAILURE);
                }
                // if in the correct 'device' format in file, add it to the devices struct.
                if (sscanf(line, "device %s %iBps %iBps",
                           devices[device_count].name,
                           &devices[device_count].readSpeed,
                           &devices[device_count].writeSpeed) != 3) {
                    printf("Incorrect device format specified.");
                    exit(EXIT_FAILURE);
                }
                device_count++;
            }
                // Check for the 'timequantum' keyword
            else if (strstr(line, "timequantum")) {
                if (sscanf(line, "timequantum %d", &timeQuantum) != 1) {
                    fprintf(stderr, "%s: Error reading timequantum in %s\n", argv0, filename);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    fclose(file);
}

//  ----------------------------------------------------------------------


void read_commands(char argv0[], char filename[]) {
    //int currentCommandID = 0; // Global variable to keep track of the current commandID

    // read the file contents
    FILE *file = fopen(filename, "r");

    // if there is no file, output error.
    if (!file) {
        fprintf(stderr, "Error: The file %s was unable to be read. Please check your file and/or filename. \n",filename);
        exit(EXIT_FAILURE);
    }

    //store each value of the line in 'line'
    char line[MAX_LINE_LENGTH];
    // temporary struct to store the current command that is having syscalls added
    command currentCommand;
    // index to track which command is being updated
    int i = -1;


    while (fgets(line, sizeof(line), file) != NULL) {

        // If line is a comment, skip it
        if (line[0] == CHAR_COMMENT) {
            continue;
        }

        //if a line has a tab, it is a new syscall therefore proceed with the 
        if (line[0]!='\t') {
            i++;

            if(i >= MAX_COMMANDS) {
                fprintf(stderr, "Error: Exceeded maximum allowed commands. Exiting.\n");
                exit(EXIT_FAILURE);
            }
            
            if(i > 0) {
                commands[i-1] = currentCommand;
            }
            strncpy(currentCommand.name, line, MAX_COMMAND_NAME);
            currentCommand.syscallcount = 0;
            currentCommand.commandID = i; // Set commandID
            currentCommand.runtime = 0; // Initialize runtime to 0
        }
        else {
            syscalls currentSyscall;
            if (strstr(line, "sleep")) {
                sscanf(line, "%dusecs sleep %dusecs", &currentSyscall.when, &currentSyscall.intValue);
                strcpy(currentSyscall.name, "sleep");
            }
            else if (strstr(line, "read")) {
                sscanf(line, "%dusecs read %s %dB", &currentSyscall.when, currentSyscall.strValue, &currentSyscall.intValue);
                strcpy(currentSyscall.name, "read");
            } else if (strstr(line, "write")) {
                sscanf(line, "%dusecs write %s %dB", &currentSyscall.when, currentSyscall.strValue, &currentSyscall.intValue);
                strcpy(currentSyscall.name, "write");
            } else if (strstr(line, "exit")) {
                sscanf(line, "%dusecs exit", &currentSyscall.when);
                strcpy(currentSyscall.name, "exit");
            } else if (strstr(line, "spawn")) {
                sscanf(line, "%dusecs spawn %s", &currentSyscall.when, currentSyscall.strValue);
                strcpy(currentSyscall.name, "spawn");
            } else if (strstr(line, "wait")) {
                sscanf(line, "%dusecs wait", &currentSyscall.when);
                strcpy(currentSyscall.name, "wait");
            }
            // Store the current syscall in the currentCommand's syscalls array
            currentCommand.syscallsArray[currentCommand.syscallcount] = currentSyscall;
            currentCommand.syscallcount++;
        }
    }

    // Store the last command after reading the file.
    commands[i] = currentCommand;
    //currentCommandID = i + 1;  // Update the global currentCommandID.
    fclose(file);
}
//  ----------------------------------------------------------------------

int getSoonestWakingCommandIndex() {

    // Initialize to first command's wake time
    int soonestIndex = sleepingQ->front->commandID;  // assuming front() gets the value at the front without dequeueing
    int minWakeTime = commands[soonestIndex].syscallsArray[commands[soonestIndex].currentsyscall].when;

    Node* currentNode = sleepingQ->front; // assuming your queue has a 'front' member

    while(currentNode) {
        int commandIndex = currentNode->commandID;
        int commandWakeTime = commands[commandIndex].syscallsArray[commands[commandIndex].currentsyscall].intValue;

        if(commandWakeTime < minWakeTime) {
            minWakeTime = commandWakeTime;
            soonestIndex = commandIndex;
        }

        currentNode = currentNode->next;  // move to the next node
    }
    if(minWakeTime <= clock){
        return soonestIndex;  // returning the index in the commands[] array of the soonest waking command
    } else{
        return -1;
    }
}



//function to execute a syscall
void executeSysCall (int commandID) {

    //EXECUTE EXIT COMMAND
    if (strcmp(commands[commandID].syscallsArray[commands[commandID].currentsyscall].name, "exit") == 0) {
        commands[commandID].currentsyscall++;
        completed[commandID] = true;
        clock += EXEC_SYSCALL_TIME;
        completed[commandID]= true;
    }

    //  EXECUTE SLEEP SYSCALL
    if (strcmp(commands[commandID].syscallsArray[commands[commandID].currentsyscall].name, "sleep") == 0) {
        clock += EXEC_SYSCALL_TIME;

        //replace sleep length with wake time and place it into the sleeping queue
        commands[commandID].syscallsArray[commands[commandID].currentsyscall].intValue += clock;

        //add curr command to sleeping queue
        enqueue(sleepingQ, commandID);
    } else if (strcmp(commands[commandID].syscallsArray[commands[commandID].currentsyscall].name, "spawn") == 0) {
        clock++;

        //commands[commandID].syscallsArray[commands[commandID].currentsyscall].strValue;
        for (int i = 0; i < MAX_COMMANDS; i++) {
            //if commands[i] matches curent spawn
            if (strcmp(commands[i].name,
                       commands[commandID].syscallsArray[commands[commandID].currentsyscall].strValue) == 0) {
                enqueue(readyQ, commands[i].commandID);
                continue;
            }
        }
        // if it is a wait proccess
    } else if (strcmp(commands[commandID].syscallsArray[commands[commandID].currentsyscall].name, "wait") == 0) {
        for(int i =0; i< commands[commandID].syscallcount-1; i ++){
            if(strcmp(commands[commandID].syscallsArray[i].name, "spawn")==0){
                for(int j = 0; j< MAX_COMMANDS; j++){
                    if (strcmp(commands[commandID].syscallsArray[i].strValue, commands[j].name)==0){
                        enqueue(&waitingQ[commandID],  j);
                    }
                }

            }
        }

    } else if (strcmp(commands[commandID].syscallsArray[commands[commandID].currentsyscall].name, "read") == 0) {

    } else if (strcmp(commands[commandID].syscallsArray[commands[commandID].currentsyscall].name, "write") == 0) {


    }
}

//function to execute a syscall
void runCommand (int commandID){

    //time left till execution of syscall
    int timeLeft;

    //current syscall
    syscalls currSysCall;
    currSysCall = commands[commandID].syscallsArray[commands[commandID].currentsyscall];

    //time till syscall gets executed
    timeLeft = currSysCall.when - commands[commandID].runtime;

    //if process wil finish before timequantum kills it
    if(timeLeft < timeQuantum){

        //increment the clock by the ammount hte command is running
        clock+= timeLeft;
        commands[commandID].runtime+= timeLeft;
        executeSysCall(commandID);

    // the command will time out
    } else{
        clock+= timeQuantum;
        commands[commandID].runtime+= timeQuantum;
        clock+= TIME_CORE_STATE_TRANSITIONS;
        enqueue(readyQ, commandID);


    }


}

void execute_commands(void)
{
    //initialize ready queue
    Queue readyQ = *createQueue();

    //initialize sleeping queue
    Queue sleepingQ = *createQueue();

    //initialize bloccke queue array
    createWaitQueue();

    //initilize array of completed variables
    bool completed[MAX_COMMANDS] = {false}; // Initialize all elements to false

    //create current command variable
    int currentCommand = 0;

    // create queue variable
    //Queue currentQueue;

    //Node* currentNode;

    //initialize clock
    int clock = 0;

    //enque first command into ready queue
    enqueue(&readyQ, currentCommand);

    //while there are commands in the ready queue or sleeping queue
    while((!isEmpty(&readyQ)) || (!isEmpty(&sleepingQ))){

        //IF THERE IS A COMMAND THAT NEEDS TO WAKE UP
        if(getSoonestWakingCommandIndex() != -1){
            currentCommand = getSoonestWakingCommandIndex();
            //add woken process to
            enqueue(&readyQ, currentCommand );
            //increment clock by BLOCKED -> READY TIME (10 usecs)
            clock+= TIME_CORE_STATE_TRANSITIONS;
            //womand woken, start next syscall
            commands[currentCommand].currentsyscall++;
        }

        //unblock any commands waiting for all of their spawned processes to finish
        for(int completedCommand; completedCommand< MAX_COMMANDS; completedCommand++){
            if(completed[completedCommand]){
                for(int j=0; j< MAX_COMMANDS; j++){
                    //if command j was waiting for completedCOmmand and only completedCOmmand
                    if((removeNodeWithCommandID(&waitingQ[j], completedCommand)) &&  (isEmpty(&waitingQ[j]))){
                        enqueue(&readyQ, j);
                    }

                }
            }
        }

        //unblock any completed I/O

        //commence any pending I/O

        //commence/resume the next ready procces if readyQ is not empty
        if (!isEmpty(&readyQ)) {
            currentCommand = dequeue(&readyQ);
            runCommand(currentCommand);

        //if ready queue is empty the just increment clock
        } else{
            clock++;
        }

        //if none of the above, then the CPU is idle



    }


}

//  ----------------------------------------------------------------------

int main(int argc, char *argv[])
{
//  ENSURE THAT WE HAVE THE CORRECT NUMBER OF COMMAND-LINE ARGUMENTS
    if(argc != 3) {
        printf("Usage: %s sysconfig-file command-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

//  READ THE SYSTEM CONFIGURATION FILE
    read_sysconfig(argv[0], argv[1]);

//  READ THE COMMAND FILE
    read_commands(argv[0], argv[2]);

//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    execute_commands();

//  PRINT THE PROGRAM'S RESULTS
    printf("measurements  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}
