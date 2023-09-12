#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define constants for maximum sizes
#define MAX_DEVICES                     4
#define MAX_DEVICE_NAME                 20
#define MAX_COMMANDS                    10
#define MAX_COMMAND_NAME                20
#define MAX_SYSCALLS_PER_PROCESS        40
#define MAX_RUNNING_PROCESSES           50

// Define other constants for time and syscall details
#define DEFAULT_TIME_QUANTUM            100
#define TIME_CONTEXT_SWITCH             5
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_ACQUIRE_BUS                20
#define MAX_SYSCALLS_NAME               10

#define CHAR_COMMENT                    '#'  // Character indicating a comment line

struct Device {
    char name[MAX_DEVICE_NAME];
    unsigned long int readspeed;
    unsigned long int writespeed;
} devices[MAX_DEVICES];  // Array to store device information

// Define structures for syscalls and commands
struct syscalls {
    int when;
    char name[MAX_SYSCALLS_NAME];
    struct Device device;  // Nested struct for the device
    struct command *command;  // Pointer to a command
};

struct command {
    char name[MAX_COMMAND_NAME];
    struct syscalls syscalls_array[MAX_SYSCALLS_PER_PROCESS];
    int syscall_count;  // Keep track of how many syscalls are in the array
};

// Function to read commands from a file
int readcommands(const char* filename, struct command commands[]) {
    // Open the file for reading
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening command file");
        return -1;
    }

    char buffer[255];
    int command_index = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Skip comment lines
        if (buffer[0] == CHAR_COMMENT) {
            continue;
        }

        // Reset syscall index when a new command starts
        if (buffer[0] != '\t') {
            command_index++;
            sscanf(buffer, "%s", commands[command_index - 1].name);  // Parse the command name
            commands[command_index - 1].syscall_count = 0;  // Initialize the syscall count for the new command
        } else {
            struct syscalls *newSyscall = &commands[command_index - 1].syscalls_array[commands[command_index - 1].syscall_count++];

            if (sscanf(buffer, "\t%dusecs %s", &newSyscall->when, newSyscall->name) != 2) {
                perror("Error parsing command file");
                fclose(fp);
                return -1;
            }

            // Additional parsing logic based on syscall type
            if (strcmp(newSyscall->name, "read") == 0 || strcmp(newSyscall->name, "write") == 0) {
                char devName[MAX_DEVICE_NAME];
                unsigned long int bytes;
                sscanf(buffer, "\t%dusecs %s %s %luB", &newSyscall->when, newSyscall->name, devName, &bytes);

                // Search and link the device in the devices list
                for (int i = 0; i < MAX_DEVICES; i++) {
                    if (strcmp(devices[i].name, devName) == 0) {
                        newSyscall->device = devices[i];
                        break;
                    }
                }
            } else if (strcmp(newSyscall->name, "spawn") == 0) {
                char cmdName[MAX_COMMAND_NAME];
                sscanf(buffer, "\t%dusecs %s %s", &newSyscall->when, newSyscall->name, cmdName);

                // Link the command in the commands list (this is a simplistic way and might need changes for actual integration)
                newSyscall->command = &commands[command_index];
            }
        }
    }

    fclose(fp);
    return command_index;
}

// Function to print the commands and syscalls
void print_commands(struct command commands[], int total_commands) {
    for (int i = 0; i < total_commands; i++) {
        printf("Command Name: %s    Number of Syscalls: %d\n\n", commands[i].name, commands[i].syscall_count);

        for (int j = 0; j < commands[i].syscall_count; j++) {
            printf("\t %i\t%s\t%s\t%i\t%s\n", commands->syscalls_array[j].when, commands->syscalls_array[j].name,
                   commands->syscalls_array[j].device.name, commands->syscalls_array[j].device.readspeed,
                   commands->syscalls_array[j].command->name);
        }

        printf("-------------------------------\n");
    }
}

int main(int argc, char *argv[]) {
    // Ensure that we have the correct number of command-line arguments
    if (argc != 3) {
        printf("Usage: %s sysconfig-file command-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct command commands[MAX_COMMANDS] = {0};
    int numofcommands;
    numofcommands = readcommands(argv[2], commands);
    print_commands(commands, numofcommands);

    // Print the program's results
    printf("measurements  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}
