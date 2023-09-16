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
#define MAX_LINE_LENGTH                      100 // Assumption: no line in any file will be longer than 100 characters
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
    //how long command has been executing on CPU
    int runtime;
    char name[MAX_COMMAND_NAME];
    //how many syscalls in command
    int syscallcount;
    //array containing all syscalls called by function
    syscalls syscallsarray[MAX_SYSCALLS_PER_PROCESS]; // Array of syscalls rand uring the process
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
//max time a command can run on CPU before getting kicked off
int timeQuantum;
//array of commands from command file
command commands[MAX_COMMANDS];

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
                if (sscanf(line, "device %s %luBps %luBps",
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


void read_commands(char argv0[], char filename[])
{
}

//  ----------------------------------------------------------------------

void execute_commands(void)
{

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
