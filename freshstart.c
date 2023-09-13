//
// Created by RadRados on 12/9/2023.
//
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
//  you may need other standard header files

//  add your name when you open the file
//  CITS2002 Project 1 2023
//  Student1:   23423175        RADOS MARKOVIC

//  Student2:   23367345        ADITYA PATIL


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
#define MAX_LINE_LENGTH                 255
//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

#define DEFAULT_TIME_QUANTUM            100

#define TIME_CONTEXT_SWITCH             5
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_ACQUIRE_BUS                20
#define MAX_SYSCALLS_NAME               10 /// assuming max name for a syscall is 10



//  ----------------------------------------------------------------------

#define CHAR_COMMENT                    '#'
#define DEBUG                           true

//STRUCTURES



struct Device {
    char name[MAX_DEVICE_NAME];
    unsigned long int readspeed;
    unsigned long int writespeed;
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
};

struct command {
    int runtime;
    char name[MAX_COMMAND_NAME];
    int syscallcount;
    syscalls syscallsarray[MAX_SYSCALLS_PER_PROCESS]; // Array of syscalls rand uring the process
};
typedef struct Node {
    command command;
    struct Node* next;
} Node;

typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;

//global variables
Queue *readyq;

Queue *blockedq;

int timequantum;

command commands[MAX_COMMANDS];

long int globalclock = 0;




//debug function
void debugprint(const char *fmt, ...) {
    if(DEBUG) {
        va_list args;
        va_start(args, fmt);
        printf("\t");
        vprintf(fmt, args);
        va_end(args);
    }
}

// Function to create a new node
Node* newNode(command command1) {
    Node* temp = (Node*)malloc(sizeof(Node));
    if (!temp) {
        printf("Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    temp->command = command1;
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
void enqueue(Queue* q, command command1) {
    Node* temp = newNode(command1);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

// Function to peek at the front element without removing it
command peek(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }
    return q->front->command;
}

// Function to remove an element from the queue
command dequeue(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }

    Node* temp = q->front;
    command command1 = temp->command;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    return command1;
}

void read_sysconfig(char argv0[], char filename[])
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "%s: unable to open %s for reading \n", argv0, filename);
        exit(EXIT_FAILURE);
    }

    char line[256];
    int device_count = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != CHAR_COMMENT) {
            if (strstr(line, "device") != NULL) {
                sscanf(line, "device %s %luBps %luBps",
                       devices[device_count].name,
                       &devices[device_count].readspeed,
                       &devices[device_count].writespeed);
                device_count++;
            }
            else if (strstr(line, "timequantum") != NULL) {
                sscanf(line, "timequantum %d", &timequantum);
            }
        }
    }
    fclose(file);
}

void printSysConfig() {
    printf("System Configuration:\n");
    printf("-------------------------------------------------\n");

    for (int i = 0; i < MAX_DEVICES && devices[i].readspeed != 0; i++) {
        // Checking readspeed != 0 as an indication that the device entry is populated.
        printf("Device Name: %s\n", devices[i].name);
        printf("Read Speed: %luBps\n", devices[i].readspeed);
        printf("Write Speed: %luBps\n", devices[i].writespeed);
        printf("-------------------------------------------------\n");
    }
    printf("Time Quantum: %i\n", timequantum);
    printf("-------------------------------------------------\n");

}


int read_commands(char argv0[], char filename[]) {
    FILE *file = fopen(filename, "r"); // Replace 'filename.txt' with your file's name
    debugprint("Reading file %s\n", filename);
    char line[MAX_LINE_LENGTH]; // Assuming max line length is 255 chars plus a null terminator
    int commandindex = -1;
    int temptime;
    char syscallname[MAX_SYSCALLS_NAME];
    int syscallsindex = 0;
    char device[MAX_DEVICE_NAME];
    char childcommandname[MAX_COMMAND_NAME];
    unsigned long int bytes;
    int sleeptime;
    while (fgets(line, sizeof(line), file)) {
        // If the first char isn't a comment
        debugprint("line: %s\n", line);
        if (line[0] != CHAR_COMMENT) {
            debugprint("line isnt a commebt\n");
            //if line doesnt start with tan  NEW COMMAND
            if (line[0] != '\t') {

                commandindex++;
                sscanf(line, "%s", commands[commandindex].name);
                commands[commandindex].syscallcount = 0;
                commands[commandindex].runtime = 0;
                syscallsindex = 0;
                debugprint("New command: %s syscall count:%i\n", commands[commandindex].name,
                           commands[commandindex].syscallcount);
            } else {
                //if line starts with a tab
                sscanf(line, "\t%iusecs %s", &temptime, syscallname);
                //if line is an exit syscall
                if (strcmp(syscallname, "exit") == 0) {
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);

                    debugprint("syscall: %s syscall count:%i\n",
                               commands[commandindex].syscallsarray[syscallsindex].name,
                               commands[commandindex].syscallcount);


                } else if (strcmp(syscallname, "sleep")==0){//sleep syscall
                    sscanf(line, "\t%iusecs %s %i",&temptime, syscallname, &sleeptime);
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    commands[commandindex].syscallsarray[syscallsindex].sleeptime = sleeptime;


                }else if((strcmp(syscallname, "write")==0) || (strcmp(syscallname, "read")==0)){
                    debugprint("syscall is write or read\n");
                    sscanf(line,"\t%iusecs %s %s %liB", &temptime, syscallname, device, &bytes);
                    debugprint("\t%s %liB to/from %s at time: %i\n", syscallname,  bytes, device, temptime);
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].device, device);
                    commands[commandindex].syscallsarray[syscallsindex].bytes = bytes;

                }else if(strcmp(syscallname, "spawn")==0){
                    debugprint("syscall is spawn\n");
                    sscanf(line,"\t%iusecs %s %s", &temptime, syscallname, childcommandname);

                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].command, childcommandname);

                   }
                    commands[commandindex].syscallcount++;
                    syscallsindex++;

                }




        }


    }
    fclose(file);
    return 0;

//  ----------------------------------------------------------------------
}

void print_command(const command *cmd) {
    if(cmd == NULL) {
        printf("Command is NULL.\n");
        return;
    }
    printf("..................................................................\n");
    printf("\tCommand Name: %s\n", cmd->name);
    printf("\tRuntime: %d\n", cmd->runtime);
    printf("\tNumber of Syscalls: %d\n", cmd->syscallcount);

    for (int j = 0; j < cmd->syscallcount; j++) {
        printf("\t\tSyscall at %iusecs: %s\n", cmd->syscallsarray[j].when, cmd->syscallsarray[j].name);

        if (strcmp(cmd->syscallsarray[j].name, "read") == 0) {
            printf("\t\t\tRead %luB from %s\n", cmd->syscallsarray[j].bytes, cmd->syscallsarray[j].device);
        } else if (strcmp(cmd->syscallsarray[j].name, "write") == 0) {
            printf("\t\t\tWrote %luB to %s\n", cmd->syscallsarray[j].bytes, cmd->syscallsarray[j].device);
        } else if (strcmp(cmd->syscallsarray[j].name, "spawn") == 0) {
            printf("\t\t\tSpawned %s\n", cmd->syscallsarray[j].command);
        } else if (strcmp(cmd->syscallsarray[j].name, "sleep") == 0) {
            printf("\t\t\tSlept for %iusecs\n", cmd->syscallsarray[j].sleeptime);
        } else if (strcmp(cmd->syscallsarray[j].name, "exit") == 0) {
            printf("\t\t\tExited\n");
        }
    }
    printf("..................................................................\n");
}

void execute_commands(void)
{
    readyq= createQueue();
    blockedq = createQueue();
    command complete[MAX_COMMANDS];//array of completed processes
    enqueue(readyq, commands[0]);
    command currentcommand;

    int timetonextsyscall = 0;
    int totalruntime;
    while(!isEmpty(readyq)){

        command currentcommand = dequeue(readyq);
        if (isEmpty(readyq)){
            debugprint("<<<<<<<<<<<<<<<readyq is empty\n");
        }
        print_command(&currentcommand);
        debugprint("executing command== %s\n", currentcommand.name);
        globalclock+=5;//add 5 usecs to clock as that is the ready -> running time requirement
        debugprint("update globalclock: %i          ready -> running\n", globalclock);

        //run command till timeout or syscall
        int i = 0;
        timetonextsyscall= ((currentcommand.syscallsarray[0].when)-(currentcommand.runtime));
        if(timetonextsyscall<timequantum){//if command exits before next time quantum kills it
            debugprint("command %s will complete systemcall : %s before time quantum\n", currentcommand.name, currentcommand.syscallsarray[0].name);
            globalclock+=timetonextsyscall;
            currentcommand.runtime+=timetonextsyscall;
            debugprint("command %s has completed systemcall : %s after %i runtime\n", currentcommand.name, currentcommand.syscallsarray[0].name, currentcommand.runtime);


        } else{//if it will time out before next syscall
            globalclock+=timequantum;//time command ran for
            globalclock+=10; ///time for timeout
            currentcommand.runtime+=timequantum;
            enqueue(readyq, currentcommand);

        }
    }

    commands[0] = currentcommand;
    printf("after simulation the updated command is %s:\n", currentcommand.name);
    debugprint("commadn 0 is now %s\n", commands[0].name);
    //calculate cpuusage
    for(int i = 0; i< MAX_COMMANDS; i++){//loop through every command
        //following j loop completely unnecessary for now
        for(int j = 0; j < commands[i].syscallcount; j++){//loop through every syscall of every command
            totalruntime += commands[i].runtime;
        }
    }
    int percentage;
    percentage = (totalruntime/globalclock)*100;
    printf("measurements  %i  %i\n", totalruntime, 0);

}   //    unblock any sleeping processes

//    unblock any processes waiting for all their spawned processes (children) to terminate
//    unblock any completed I/O
//    commence any pending I/O
//    commence/resume the next READY process
//    otherwise the CPU remains idle...


//  ----------------------------------------------------------------------
void print_commands() {
    printf("Commands:\n");
    printf("-------------------------------------------------\n");

    for (int i = 0; i < MAX_COMMANDS && commands[i].syscallcount > 0; i++) {
        // Checking syscallcount > 0 is a way to ensure that the command is valid
        // (assuming a command with 0 syscalls doesn't exist).

        printf("Command Name: %s\n", commands[i].name);
        printf("Number of Syscalls: %d\n", commands[i].syscallcount);

        for (int j = 0; j < commands[i].syscallcount; j++) {
            printf("\tSyscall at %iusecs: %s\n", commands[i].syscallsarray[j].when, commands[i].syscallsarray[j].name);

            if (strcmp(commands[i].syscallsarray[j].name, "read") == 0) {
                printf("\t\tRead %luB from %s\n", commands[i].syscallsarray[j].bytes,
                       commands[i].syscallsarray[j].device);
            } else if (strcmp(commands[i].syscallsarray[j].name, "write") == 0) {
                printf("\t\tWrote %luB to %s\n", commands[i].syscallsarray[j].bytes,
                       commands[i].syscallsarray[j].device);
            } else if (strcmp(commands[i].syscallsarray[j].name, "spawn") == 0) {
                printf("\t\tSpawned %s\n", commands[i].syscallsarray[j].command);
            } else if (strcmp(commands[i].syscallsarray[j].name, "sleep") == 0) {
                printf("\t\tSlept for %iusecs\n", commands[i].syscallsarray[j].sleeptime);
            } else if (strcmp(commands[i].syscallsarray[j].name, "exit") == 0) {
                printf("\t\tExited\n");
            }
        }




        printf("-------------------------------------------------\n");
    }
}

int main(int argc, char *argv[])
{
//  ENSURE THAT WE HAVE THE CORRECT NUMBER OF COMMAND-LINE ARGUMENTS
    if(argc != 3) {
        printf("Usage: %s sysconfig-file command-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

//  READ THE SYSTEM CONFIGURATION FILE
    read_sysconfig(argv[0], argv[1]);
    printSysConfig();
    //  READ THE COMMAND FILE
    read_commands(argv[0], argv[2]);
    print_commands();


//
//    enqueue(q, 10);
//    enqueue(q, 20);
//    enqueue(q, 30);
//
//    printf("Front element: %d\n", peek(q));
//
//    printf("Dequeued element: %d\n", dequeue(q));
//
//    printf("Front element after dequeue: %d\n", peek(q));



//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    debugprint("Executing commands...\n");
    execute_commands();

//  PRINT THE PROGRAM'S RESULTS

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
