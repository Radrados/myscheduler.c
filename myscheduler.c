#include <stdio.h>
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

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

#define DEFAULT_TIME_QUANTUM            100

#define TIME_CONTEXT_SWITCH             5
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_ACQUIRE_BUS                20


//  ----------------------------------------------------------------------

#define CHAR_COMMENT                    '#'
struct Device {
    char name[MAX_DEVICE_NAME];
    unsigned long int readspeed;
    unsigned long int writespeed;
}devices [MAX_DEVICES];



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
