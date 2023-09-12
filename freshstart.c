//
// Created by RadRados on 12/9/2023.
//
#include <stdio.h>
#include <stdarg.h>

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
#define DEBUG 0
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
    char device[MAX_DEVICE_NAME];  // nested struct
    char command[MAX_COMMAND_NAME]; // Pointer to command as it's not yet fully defined
    unsigned long int bytes;
    int sleeptime;
};
struct command {
    char name[MAX_COMMAND_NAME];
    int syscallcount;
    syscalls syscallsarray[MAX_SYSCALLS_PER_PROCESS]; // Array of syscalls rand uring the process
};
//teteeteter
int timequantum;
command commands[MAX_COMMANDS];
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
                    commands[commandindex].syscallcount++;
                    syscallsindex++;
                    debugprint("syscall: %s syscall count:%i\n",
                               commands[commandindex].syscallsarray[syscallsindex].name,
                               commands[commandindex].syscallcount);


                } else if (strcmp(syscallname, "sleep")==0){
                    sscanf(line, "\t%iusecs %s %i",&temptime, syscallname, &sleeptime);
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    commands[commandindex].syscallsarray[syscallsindex].sleeptime = sleeptime;
                    commands[commandindex].syscallcount++;
                    syscallsindex++;

                }else if((strcmp(syscallname, "write")==0) || (strcmp(syscallname, "read")==0)){
                    debugprint("syscall is write or read\n");
                    sscanf(line,"\t%iusecs %s %s %liB", &temptime, syscallname, device, &bytes);
                    debugprint("\t%s %liB to/from %s at time: %i\n", syscallname,  bytes, device, temptime);
                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].device, device);
                    commands[commandindex].syscallsarray[syscallsindex].bytes = bytes;
                    commands[commandindex].syscallcount++;
                    syscallsindex++;
                }else if(strcmp(syscallname, "spawn")==0){
                    debugprint("syscall is spawn\n");
                    sscanf(line,"\t%iusecs %s %s", &temptime, syscallname, childcommandname);

                    commands[commandindex].syscallsarray[syscallsindex].when = temptime;
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].name, syscallname);
                    strcpy(commands[commandindex].syscallsarray[syscallsindex].command, childcommandname);
                    commands[commandindex].syscallcount++;
                    syscallsindex++;
                }

                }




        }


    }
    fclose(file);
    return 0;

//  ----------------------------------------------------------------------
}

void execute_commands(void)
{
}

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
//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    execute_commands();

//  PRINT THE PROGRAM'S RESULTS
    printf("measurements  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
