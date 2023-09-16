#include <stdio.h>
#include <stdlib.h>
//  you may need other standard header files


//  CITS2002 Project 1 2023
//  Student1:   STUDENT-NUMBER1   NAME-1
//  Student2:   STUDENT-NUMBER2   NAME-2


//  myscheduler (v1.0)
//  Compile with:  cc -std=c11 -Wall -Werror -o myscheduler myscheduler.c


//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF sysconfig AND command DETAILS
//  THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE //  CONSTANTS
//  WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES                     4
#define MAX_DEVICE_NAME                 20
#define MAX_COMMANDS                    10
#define MAX_COMMAND_NAME                20
#define MAX_SYSCALLS_PER_PROCESS        40
#define MAX_RUNNING_PROCESSES           50

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

#define DEFAULT_TIME_QUANTUM            100

#define TIME_CONTEXT_SWITCH             5
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_ACQUIRE_BUS                20
#define MAX_SYSCALLS_NAME               6 // assuming max name for a syscall is 10


//  ----------------------------------------------------------------------

#define CHAR_COMMENT                    '#'
# define MAX_LINE_CHAR_LENGTH           256
#define DEBUG                           true
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct Device {
    char name[MAX_DEVICE_NAME];
    unsigned long int readspeed;
    unsigned long int writespeed;
    int device_id; // maybe remove, rad asked me to add it
}devices [MAX_DEVICES];

typedef struct command command;

typedef struct syscalls syscalls;

struct syscalls {
    int when;
    char name[MAX_SYSCALLS_NAME];//
    char device[MAX_DEVICE_NAME];  // nested struct
    char command[MAX_COMMAND_NAME]; // Pointer to command as it's not yet fully defined
    unsigned long int bytes;
    int sleeptime;
    int waketime;
};

struct command {
    int runtime;
    char name[MAX_COMMAND_NAME];
    int syscallcount;
    syscalls syscallsarray[MAX_SYSCALLS_PER_PROCESS]; // Array of syscalls rand uring the process
    int currentsyscall;
};

////////////////////////////////START QUEUE///////////////////////////////////////

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

//global variables

// queue of all the commands to be run
Queue *readyQ;

// queue of all the commands that have a "wait" time
Queue *blockedQ;

// max amount of time a process is allowed to run for.
int timeQuantum;

// create new node
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

// Function to add an element to the queue
void enqueue(Queue* q, int commandID, int secondaryValue) {
    Node* temp = newNode(commandID, secondaryValue);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}
//SKIPPED THIS FUNCTION COME BACK AND FIX IT PLSSS
// Function to add an element to the sleep queue in order of wake time
// void sleepenqueue(Queue* q, int commandID, int wakeTime) {
//     Node* temp = newNode(commandID, wakeTime);
//     if (q->rear == NULL) {
//         q->front = q->rear = temp;
//         return;
//     }
//     // command current_command = getCommandByID(command_id);
//     // // If the waketime of the new command is smaller than the front's waketime,
//     // // then insert the new node at the beginning
//     // if (command1.syscallsarray[command1.currentsyscall].waketime < q->front->command.syscallsarray[q->front->command.currentsyscall].waketime) {
//     //     temp->next = q->front;
//     //     q->front = temp;
//     // } else {
//     //     // Find a position in the queue such that the command's waketime is in the right position
//     //     Node* current = q->front;
//     //     while (current->next != NULL && current->next->command.syscallsarray[current->next->command.currentsyscall].waketime < command1.syscallsarray[command1.currentsyscall].waketime) {
//     //         current = current->next;
//     //     }

//         // Insert the new node after the current node
//         temp->next = current->next;
//         current->next = temp;

//         if (current == q->rear) {
//             q->rear = temp;
//         }
//     }
// }

// You can also add dequeue, display functions, etc., as per your requirement.


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
////////////////////////////////END QUEUE///////////////////////////////////////
// re-write queues for command id and value for queue fields
void read_sysconfig(char argv0[], char filename[])
{
    FILE *file;
    char line[MAX_LINE_CHAR_LENGTH];
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
                    &devices[device_count].readspeed,
                    &devices[device_count].writespeed) != 3) {
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

//  vim: ts=8 sw=4
