#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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
#define LINESIZE                        255

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

#define CHAR_COMMENT  '#'
#define DEBUG 1
// Debug print function
void debugprint(const char *fmt, ...) {
    if(DEBUG) {
        va_list args;
        va_start(args, fmt);
        printf("\t");
        vprintf(fmt, args);
        va_end(args);
    }
}



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
    struct Device device;  // nested struct
    unsigned long bytes;//bytes to read or write
    command *command; // Pointer to command as it's not yet fully defined
};

struct command {
    char name[MAX_COMMAND_NAME];
    syscalls syscalls_array[MAX_SYSCALLS_PER_PROCESS]; //array of syscalls called by fuction
    int syscallcount; // how many syscalls per command
};
//teteeteter


void read_sysconfig(char argv0[], char filename[])
{
//change this entire file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "%s: unable to open %s for reading \n", argv0, filename);
        exit(EXIT_FAILURE);
    }

    char line[256];
    int device_count = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != CHAR_COMMENT && strstr(line, "device") != NULL) {
            sscanf(line, "device %s %luBps %luBps",
                   devices[device_count].name,
                   &devices[device_count].readspeed,
                   &devices[device_count].writespeed);
            device_count++;
        }
    }
    printf("here are the number of devices: %i \n", device_count);
    //for (int i = 0; i < device_count; i++) {
    //printf("Device Name: %s \n", devices[i].name);
    //printf("Device Name: %lu \n", devices[i].readspeed);
    //printf("Device Name: %lu \n", devices[i].writespeed);
    //printf("---------------------------------- \n");
    //}
    fclose(file);
}

int readcommands(char argv0[], char filename[], command commands[]) {
    FILE *file = fopen(filename, "r"); // open file in read mode
    if (!file) {
        perror("eroor opening commands file");
        return -1;
    }
    char line[LINESIZE];
    int commandindex=-1;
    int currentsyscallcount = 0;

    while (fgets(line, sizeof(line), file) != NULL) {//read each line untill end of file and store i tin line string
        debugprint("line: %s\n", line);
        if (line[0] == CHAR_COMMENT) {// if line starts with commentchar skip it
            continue;
        }

        if (line[0] != '\t') {// if line doesnt start with tab then it is a new command
            commandindex++;
            debugprint("command index %i\n", commandindex);
            debugprint("CURRENT SYSCALL COUNT: %i\n", commands[commandindex].syscallcount);
            sscanf(line, "%s", commands[commandindex - 1].name); // save command name into command struct
            debugprint("\tcommand name: %s\n", commands[commandindex ].name);
            commands[commandindex - 1].syscallcount = 0; // set syscall count to 0 as it is a new command
            debugprint("CURRENT SYSCALL COUNT: %i\n", commands[commandindex].syscallcount);
        }

        else {//if line starts with tab then it is a syscall
            debugprint("\tline starts with tab\n");
            debugprint("\t\t\tcurrent syscall count: %i\n", commands[commandindex ].syscallcount);

            struct syscalls *newSyscall;//pointer to struct syscall
            struct command *currentcommand = &commands[commandindex - 1];//pointer to current command
            struct syscalls *syscallarray = currentcommand->syscalls_array;//pointer to syscall array within current command

            //int currentsyscallcount = currentcommand->syscallcount;//current syscall count for current array
            newSyscall = &syscallarray[currentsyscallcount];            //set new syscall to the current syscall count// MAGIC, IDK WHAT THIS IS DOING
            debugprint("\tcurrent syscall count: %i\n", currentsyscallcount);
            currentsyscallcount++;
            debugprint("\tupdated syscall count %i\n", currentsyscallcount);
            sscanf(line, "\t%dusecs %s",
                   &newSyscall->when,
                   newSyscall->name);//split the line and save the when and name into new syscall
            debugprint("\tsyscall name: %s\n", newSyscall->name);
            if(strcmp(newSyscall->name, "exit") == 0){
                debugprint("\t\texit syscall found\n");
                commands[commandindex].syscalls_array[currentsyscallcount]= *newSyscall;
                commands[commandindex].syscallcount = currentsyscallcount;
                debugprint("\t\tcurrent suscallcount: %i\n", commands[commandindex].syscallcount);

            }

            else if(strcmp(newSyscall->name, "sleep") == 0) {//if syscall is sleep
                debugprint("\t\tsleep syscall found\n");
                sscanf(line, "\t%dusecs %s %luB",
                       &newSyscall->when,
                       newSyscall->name,
                       &newSyscall->bytes);//split the line and save the when and name into new syscall
                debugprint("\t\tsscanf complete for sleep syscall\n");
                commands[commandindex].syscalls_array[currentsyscallcount]= *newSyscall;
                commands[commandindex].syscallcount = currentsyscallcount;
                debugprint("\t\tcurrent syscall %s\n", commands[commandindex].syscalls_array[currentsyscallcount].name);

            }


            debugprint("\tpre updated syscall count %i\n", commands[commandindex - 1].syscallcount);

            commands[commandindex - 1].syscallcount++;
            debugprint("\tupdated syscall count %i\n", commands[commandindex - 1].syscallcount);
            debugprint("command index %i\n--------------------------\n", commandindex);

        }

    }
    fclose(file);
    return commandindex+1;

}


//  ----------------------------------------------------------------------

void printcommands(struct command commands[], int totalcommands){
    for(int i = 0; i < totalcommands; i++){
        printf("Command Name: %s    Number of Syscalls: %d\n\n", commands[i].name, commands[i].syscallcount);
        for(int j =0 ; j< commands[i].syscallcount; j++){
            printf("\t %i\t%s\t%s\t%lu\t%s\n",
                   commands->syscalls_array[j].when,
                   commands->syscalls_array[j].name,
                   commands->syscalls_array[j].device.name,
                   commands->syscalls_array[j].bytes,
                   commands->syscalls_array[j].command->name);
        }
        printf("-------------------------------\n");
    }
}
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
    struct command commands[MAX_COMMANDS] = {0};
    int numofcommands;
    numofcommands = readcommands(argv[0], argv[2], commands);
    debugprint("====================================\n");
    debugprint("printing commands:\n"
            );
    printcommands(commands, numofcommands);
    debugprint("num of commands is %i\n", numofcommands);
//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    execute_commands();

//  PRINT THE PROGRAM'S RESULTS
    printf("measurements  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4