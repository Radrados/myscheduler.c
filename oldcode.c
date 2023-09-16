//testing git
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
#define EXEC_SYSCALL_TIME               1



//  ----------------------------------------------------------------------

#define CHAR_COMMENT                    '#'
#define DEBUG                           true
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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
    int waketime;
};

struct command {
    int runtime;
    char name[MAX_COMMAND_NAME];
    int syscallcount;
    syscalls syscallsarray[MAX_SYSCALLS_PER_PROCESS]; // Array of syscalls rand uring the process
    int currentsyscall;
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
        vprintf(fmt, args);
        va_end(args);
    }
}

// Function to create a new node
Node* newNode(command command1) {
    Node* temp = (Node*)malloc(sizeof(Node));
    if (!temp) {
       debugprint("Memory allocation error\n");
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
       debugprint("Memory allocation error\n");
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

// Function to add an element to the sleep queue in order of wake time
void sleepenqueue(Queue* q, command command1) {
    Node* temp = newNode(command1);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }

    // If the waketime of the new command is smaller than the front's waketime,
    // then insert the new node at the beginning
    if (command1.syscallsarray[command1.currentsyscall].waketime < q->front->command.syscallsarray[q->front->command.currentsyscall].waketime) {
        temp->next = q->front;
        q->front = temp;
    } else {
        // Find a position in the queue such that the command's waketime is in the right position
        Node* current = q->front;
        while (current->next != NULL && current->next->command.syscallsarray[current->next->command.currentsyscall].waketime < command1.syscallsarray[command1.currentsyscall].waketime) {
            current = current->next;
        }

        // Insert the new node after the current node
        temp->next = current->next;
        current->next = temp;

        if (current == q->rear) {
            q->rear = temp;
        }
    }
}

// You can also add dequeue, display functions, etc., as per your requirement.


// Function to peek at the front element without removing it
command peek(Queue* q) {
    if (isEmpty(q)) {
       debugprint("Queue is empty\n");
        exit(EXIT_FAILURE);
    }
    return q->front->command;
}

// Function to remove an element from the queue
command dequeue(Queue* q) {
    if (isEmpty(q)) {
       debugprint("Queue is empty\n");
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
   debugprint("System Configuration:\n");
   debugprint("-------------------------------------------------\n");

    for (int i = 0; i < MAX_DEVICES && devices[i].readspeed != 0; i++) {
        // Checking readspeed != 0 as an indication that the device entry is populated.
       debugprint("Device Name: %s\n", devices[i].name);
       debugprint("Read Speed: %luBps\n", devices[i].readspeed);
       debugprint("Write Speed: %luBps\n", devices[i].writespeed);
       debugprint("-------------------------------------------------\n");
    }
   debugprint("Time Quantum: %i\n", timequantum);
   debugprint("-------------------------------------------------\n");

}


int read_commands(char argv0[], char filename[]) {
    FILE *file = fopen(filename, "r"); // Replace 'filename.txt' with your file's name
    //debugprint("Reading file %s\n", filename);
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
        //debugprint("line: %s\n", line);
        if (line[0] != CHAR_COMMENT) {
            //debugprint("line isnt a commebt\n");
            //if line doesnt start with tan  NEW COMMAND
            if (line[0] != '\t') {

                commandindex++;
                sscanf(line, "%s", commands[commandindex].name);
                commands[commandindex].syscallcount = 0;
                commands[commandindex].runtime = 0;
                commands[commandindex].currentsyscall=0;

                syscallsindex = 0;
                //debugprint("New command: %s syscall count:%i\n", commands[commandindex].name,
                //           commands[commandindex].syscallcount);
            } else {
                //if line starts with a tab
                sscanf(line, "\t%iusecs %s", &temptime, syscallname);
                //if line is an exit syscall
                if (strcmp(syscallname, "exit") == 0) {
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);

                    //debugprint("syscall: %s syscall count:%i\n",
                    //           commands[commandindex].syscallsarray[syscallsindex].name,
                    //           commands[commandindex].syscallcount);


                } else if (strcmp(syscallname, "sleep")==0){//sleep syscall
                    sscanf(line, "\t%iusecs %s %i",&temptime, syscallname, &sleeptime);
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    commands[commandindex].syscallsarray[syscallsindex].sleeptime = sleeptime;


                }else if((strcmp(syscallname, "write")==0) || (strcmp(syscallname, "read")==0)){
                    //debugprint("syscall is write or read\n");
                    sscanf(line,"\t%iusecs %s %s %liB", &temptime, syscallname, device, &bytes);
                    //debugprint("\t%s %liB to/from %s at time: %i\n", syscallname,  bytes, device, temptime);
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].device, device);
                    commands[commandindex].syscallsarray[syscallsindex].bytes = bytes;

                }else if(strcmp(syscallname, "spawn")==0){
                    //debugprint("syscall is spawn\n");
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
void calculateruntime(int clock){
    debugprint("calculating runtime\n");
    //calculate TOTAL runtime
    int totalruntime=0;
    for(int i = 0; i < MAX_COMMANDS; i++) {
        totalruntime += commands[i].syscallsarray[commands[i].syscallcount-1].when;
    }
    //double percentage = (double) totalruntime/clock;
    printf("measurements %i  %i\n", clock, totalruntime);
//    }
//    debugprint("total runtime is %i\n", totalruntime);
//    debugprint("global clock is %li\n", globalclock);
//    long double doubletotalruntime = (long double) totalruntime;
//    long double doubleglobalclock = (long double) globalclock;
//    long double doublerunningtoreadywaittime = (long double) runningtoreadywaittime;
//    long double doublereadytorunningwaittime = (long double) readytorunningwaittime;
////        doubleglobalclock+= doublerunningtoreadywaittime;
////        doubleglobalclock+= doublereadytorunningwaittime;
//    debugprint("double total runtime is %Lf\n", doubletotalruntime);
//    debugprint("time spent waiting from ready to running is %i\n", readytorunningwaittime);
//    debugprint("time spent waiting from running to ready is %i\n", runningtoreadywaittime);
//    //calculate percentage
//    long int percentage =  (doubletotalruntime/doubleglobalclock)*100;
//    int intglobalclock = (int) doubleglobalclock;
//    printf("measurements  %i  %li\n", intglobalclock, percentage);
//    //exit(EXIT_SUCCESS);
//
//    debugprint("segfault\n");
}

void print_command(const command *cmd) {
    if(cmd == NULL) {
       debugprint("Command is NULL.\n");
        return;
    }
   debugprint("+++++++++++++++++++++++++++++++++++++++++\n");
   debugprint("\tCommand Name: %s\n", cmd->name);
   debugprint("\tRuntime: %d\n", cmd->runtime);
   debugprint("\tNumber of Syscalls: %d\n", cmd->syscallcount);
   debugprint("\t Current syscall: %i\n", cmd->currentsyscall);

    for (int j = 0; j < cmd->syscallcount; j++) {
       debugprint("\t\tSyscall at %iusecs: %s\n", cmd->syscallsarray[j].when, cmd->syscallsarray[j].name);

        if (strcmp(cmd->syscallsarray[j].name, "read") == 0) {
           debugprint("\t\t\tRead %luB from %s\n", cmd->syscallsarray[j].bytes, cmd->syscallsarray[j].device);
        } else if (strcmp(cmd->syscallsarray[j].name, "write") == 0) {
           debugprint("\t\t\tWrite %luB to %s\n", cmd->syscallsarray[j].bytes, cmd->syscallsarray[j].device);
        } else if (strcmp(cmd->syscallsarray[j].name, "spawn") == 0) {
           debugprint("\t\t\tSpawn %s\n", cmd->syscallsarray[j].command);
        } else if (strcmp(cmd->syscallsarray[j].name, "sleep") == 0) {
           debugprint("\t\t\tSleep for %iusecs\n", cmd->syscallsarray[j].sleeptime);
        } else if (strcmp(cmd->syscallsarray[j].name, "exit") == 0) {
           debugprint("\t\t\tExited\n");
        }
    }
   debugprint("+++++++++++++++++++++++++++++++++++++++++\n");
}
int findindex(char *name){
    for(int i = 0; i < MAX_COMMANDS; i++) {
        if(strcmp(commands[i].name, name)==0){
            return i;
        }
    }
    return -1;
}

int execute_commands(void){
    readyq = createQueue();
    blockedq = createQueue();
    command currentcommand;
    command completedcommands[MAX_COMMANDS];
    command wokencommand;
    int currentcommandindex;

    int timeleft;//how long a command has till its end
    int readytorunningwaittime = 0;// how long waiting to go from ready to running
    int runningtoreadywaittime = 0;//how long waiting ot ogo from running to ready
    int blockedtoreadywaitime = 0;//how long waiting to go from blocked to ready
    //add first command to ready queue
    enqueue(readyq, commands[0]);
    int waketime;
    //while threr is stuff in the ready queue
    while(!isEmpty(readyq) || !isEmpty(blockedq)){
        debugprint("########################################################\n");



        if(!isEmpty(blockedq)){
            debugprint("blocked queue is not empty\n");
            command blockedcommand = peek(blockedq);
            //print_command(&blockedcommand);
            debugprint("first command in blocked queue has waketime %i\n", peek(blockedq).syscallsarray[peek(blockedq).currentsyscall].waketime);
            debugprint("global clock is %li\n", globalclock);
            //check if the first command in the blocked queue has woken up
            if(peek(blockedq).syscallsarray[peek(blockedq).currentsyscall].waketime <= globalclock){

                debugprint("first command in blocked queue has WOKEN up at time %i\n", globalclock);
                //add the command to the ready queue
                wokencommand = dequeue(blockedq);
                wokencommand.currentsyscall++;
                enqueue(readyq, wokencommand);
                globalclock+= TIME_CORE_STATE_TRANSITIONS;
                blockedtoreadywaitime+= TIME_CORE_STATE_TRANSITIONS;




            }
        }


        if(isEmpty(readyq)){
            debugprint("ready queue is empty\n");
            //if the ready queue is empty, increment the clock by 1
            globalclock++;
            continue;
        }





        //get the first command in the ready queue


        if(isEmpty(readyq)){
            debugprint("\t\t readyq is empty \n");
            }
        //incremetn the clock by the ready -> running time (context switch)
        globalclock+= TIME_CONTEXT_SWITCH;
        readytorunningwaittime+= TIME_CONTEXT_SWITCH;

        currentcommand = dequeue(readyq);
        currentcommandindex= findindex(currentcommand.name);
        debugprint("\ncurrent command is \n");
        print_command(&currentcommand);
        debugprint("current syscall is :  %i: %i %s \n",currentcommand.currentsyscall, currentcommand.syscallsarray[currentcommand.currentsyscall].when, currentcommand.syscallsarray[currentcommand.currentsyscall].name);

        // check if command will finish before time quantum kills it
        timeleft= MIN(currentcommand.syscallsarray[currentcommand.currentsyscall].when, timequantum);
        if(((currentcommand.syscallsarray[currentcommand.currentsyscall].when)- currentcommand.runtime) < timequantum){
            //find the time till the finish of the current syscall in the command
            debugprint("current commands will finish before the time quantum\n");
            //time the command will run before it finishes

            timeleft = (currentcommand.syscallsarray[currentcommand.currentsyscall].when) - (currentcommand.syscallsarray[currentcommand.currentsyscall-1].when);
            commands[currentcommandindex].syscallsarray[currentcommand.currentsyscall].when = timeleft;
            debugprint("current command runtime is %i\n", commands[currentcommand.currentsyscall].runtime );
            //updating the runtime of the command
            debugprint("current command timeleft is %i\n", timeleft);
            globalclock+= timeleft;
            debugprint("global clock after execution is %li\n", globalclock);
            //update runtime
            //currentcommand.runtime = currentcommand.runtime + timeleft;

            // if the suscall finishes,increment the command to the next syscall

            //if the syscall is exit
            if(strcmp(currentcommand.syscallsarray[currentcommand.currentsyscall].name, "exit")==0) {
                debugprint("EXIT SYSCALL\n");
                globalclock += EXEC_SYSCALL_TIME;// 1 time actually needed to execute a syscall
                currentcommand.currentsyscall++;



            }
            //if the sysclall is sleep
            else if(strcmp(currentcommand.syscallsarray[currentcommand.currentsyscall].name, "sleep")==0) {

                debugprint("SLEEP SYSCALL\n");
                debugprint("current syscall is %i\n", currentcommand.currentsyscall);
                debugprint("sleep for : %i\n", currentcommand.syscallsarray[currentcommand.currentsyscall].sleeptime);
                globalclock+= EXEC_SYSCALL_TIME;// 1 time actually needed to execute a syscall

                waketime = globalclock + currentcommand.syscallsarray[currentcommand.currentsyscall].sleeptime;
                currentcommand.syscallsarray[currentcommand.currentsyscall].waketime = waketime;

                enqueue(blockedq, currentcommand);

            }
            

            //if command will not finish before time quantum kills it
        }else{//if command will not finish before time quantum kills it
           debugprint("current command will not finish before the time quantum\n");
            timeleft = timequantum;
            currentcommand.runtime = currentcommand.runtime + timequantum;
           debugprint("current command runtime is %i\n", currentcommand.runtime);
           debugprint("current command timeleft is %i\n", timeleft);
            globalclock+= timequantum;
           debugprint("global clock is %li\n", globalclock);
            enqueue(readyq, currentcommand);
            globalclock+= TIME_CORE_STATE_TRANSITIONS;//10
            runningtoreadywaittime+= TIME_CORE_STATE_TRANSITIONS;

        }
        if(isEmpty(readyq)){
            debugprint("\t\t readyq is empty next cycle should just increment \n");
        }
        if(isEmpty(blockedq)){
            debugprint("\t\t blockedq is empty next cycle should just increment \n");
        }


    }
    //       debugprint("\n the current command 0 is:\n");
//        print_command(&commands[0]);



    debugprint("########################################################\n");
    calculateruntime(globalclock);

}   //    unblock any sleeping processes

//    unblock any processes waiting for all their spawned processes (children) to terminate
//    unblock any completed I/O
//    commence any pending I/O
//    commence/resume the next READY process
//    otherwise the CPU remains idle...

//  ----------------------------------------------------------------------
void print_commands() {
   debugprint("Commands:\n");
   debugprint("-------------------------------------------------\n");

    for (int i = 0; i < MAX_COMMANDS && commands[i].syscallcount > 0; i++) {
        // Checking syscallcount > 0 is a way to ensure that the command is valid
        // (assuming a command with 0 syscalls doesn't exist).

       debugprint("Command Name: %s\n", commands[i].name);
       debugprint("Number of Syscalls: %d\n", commands[i].syscallcount);
        debugprint("current syscall is %i\n");

        for (int j = 0; j < commands[i].syscallcount; j++) {
           debugprint("\tSyscall at %iusecs: %s\n", commands[i].syscallsarray[j].when, commands[i].syscallsarray[j].name);

            if (strcmp(commands[i].syscallsarray[j].name, "read") == 0) {
               debugprint("\t\tRead %luB from %s\n", commands[i].syscallsarray[j].bytes,
                       commands[i].syscallsarray[j].device);
            } else if (strcmp(commands[i].syscallsarray[j].name, "write") == 0) {
               debugprint("\t\tWrite %luB to %s\n", commands[i].syscallsarray[j].bytes,
                       commands[i].syscallsarray[j].device);
            } else if (strcmp(commands[i].syscallsarray[j].name, "spawn") == 0) {
               debugprint("\t\tSpawned %s\n", commands[i].syscallsarray[j].command);
            } else if (strcmp(commands[i].syscallsarray[j].name, "sleep") == 0) {
               debugprint("\t\tSleep for %iusecs\n", commands[i].syscallsarray[j].sleeptime);
            } else if (strcmp(commands[i].syscallsarray[j].name, "exit") == 0) {
               debugprint("\t\tExit\n");
            }
        }




       debugprint("-------------------------------------------------\n");
    }
}

int main(int argc, char *argv[])
{
//  ENSURE THAT WE HAVE THE CORRECT NUMBER OF COMMAND-LINE ARGUMENTS
    if(argc != 3) {
       debugprint("Usage: %s sysconfig-file command-file\n", argv[0]);
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
//   debugprint("Front element: %d\n", peek(q));
//
//   debugprint("Dequeued element: %d\n", dequeue(q));
//
//   debugprint("Front element after dequeue: %d\n", peek(q));



//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    debugprint("Executing commands...\n");
    execute_commands();

//  PRINT THE PROGRAM'S RESULTS

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
