//
// Created by RadRados on 12/9/2023.
//

//
//            //if syscall is read or write
//            if (strcmp(newSyscall->name, "read") == 0 || strcmp(newSyscall->name, "write") == 0) {
//                char devicename[MAX_DEVICE_NAME];
//                unsigned long int bytes;
//                // assign data from line into neqsyscall
//                sscanf(line, "\t%dusecs %s %s %luB",
//                       &newSyscall->when,
//                       newSyscall->name,
//                       devicename,
//                       &newSyscall->bytes);//maybe remove &from bytes
//
//                //loop through and find device name
//                for (int i = 0; i < MAX_DEVICES; i++) {
//                    if (strcmp(devicename, devices[i].name) == 0) {
//                        newSyscall->device = devices[i];
//                        break;
//                    }
//                }
//
//            } else {
//
//                //if syscall is spawn
//                if (strcmp(newSyscall->name, "spawn") == 0) {
//                    struct command *newcommand;//creates new command struct to store name of command that will be spawned
//                    //cannot store other command details as that command might not have been read off of file yet
//
//                    //assign data from line into newsyscall
//                    sscanf(line, "\t%dusecs %s %s",
//                           &newSyscall->when,
//                           newSyscall->name,
//                           &newSyscall->command->name);
//
//                    newSyscall->command = newcommand;//assign new command to new syscall
//                }
//
//                currentcommand->syscallcount++;