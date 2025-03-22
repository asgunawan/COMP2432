
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_NAME_LENGTH 50
#define MAX_DATE_LENGTH 20
#define MAX_MEMBERS 5
#define MAX_FACILITIES 6
#define MAX_FACILITY_NAME_LENGTH 20

typedef enum {
    CMD_ADD_PARKING,
    CMD_ADD_RESERVATION,
    CMD_ADD_EVENT,
    CMD_BOOK_ESSENTIALS,
    CMD_INVALID
} CommandType;

typedef struct {
    char memberName[MAX_NAME_LENGTH];
    char date[MAX_DATE_LENGTH];
    char time[MAX_DATE_LENGTH];
    float duration;
    char facilities[MAX_FACILITIES][MAX_FACILITY_NAME_LENGTH];
    int facilityCount;
} Booking;

bool validateMember(const char* memberName);
bool validateDateTime(const char* date, const char* time);
bool validateFacilities(char facilities[][MAX_FACILITY_NAME_LENGTH], int count);
void processBooking(const char* command);
void processInput(FILE* input, bool isBatchFile);
CommandType parseCommandType(const char* command);
bool validateCommandFormat(const char* command, CommandType cmdType);
bool validateFacilityCombination(const char* facility1, const char* facility2);
bool validateAddBatchCommand(const char* command);

int main() {
    printf("~~Welcome to PolyU~~\n");
    processInput(stdin, false);
    return 0;
}

void processInput(FILE* input, bool isBatchFile) {
    char line[MAX_COMMAND_LENGTH];
    int lineCount = 0;

    while (1) {
        if (!isBatchFile) {
            printf("Please enter booking:\n");
        }

        // Read input until a newline character
        if (fgets(line, sizeof(line), input) == NULL) {
            break; // Exit loop on read failure
        }

        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        // Check for empty input
        if (strlen(line) == 0) {
            printf("Error: No command entered. Please enter a valid command.\n");
            continue;
        }

        // Check if command ends with a semicolon
        if (line[strlen(line) - 1] != ';') {
            printf("Error: Command must end with a semicolon (;).\n");
            continue;
        }

        // Remove the semicolon
        line[strlen(line) - 1] = '\0';

        // Process other command logic
        if (!isBatchFile && strcmp(line, "endProgram") == 0) {
            printf("->Bye!");
            break;
        }

        if (!isBatchFile && strncmp(line, "addBatch", 8) == 0) {
            if (!validateAddBatchCommand(line)) {
                    printf("Error: Invalid addBatch command format. It should be in the format: addBatch -filename.dat;\n");
                    continue;
            }
            
            char* filename = strchr(line, '-');
            if (filename) {
                filename++; // skip '-'
                FILE* batchFile = fopen(filename, "r");
                if (!batchFile) {
                    printf("Error: Cannot open batch file: %s\n", filename);
                    continue;
                }
                printf("\nProcessing batch file: %s\n", filename);
                processInput(batchFile, true);
                fclose(batchFile);
                printf("\nBatch processing completed.\n");
            }
        } 
        else {
            // Check the command type
            CommandType cmdType = parseCommandType(line);
            if (cmdType == CMD_INVALID) {
                printf("Error: Invalid command type\n");
                continue;
            }

            // Validate command format
            if (!validateCommandFormat(line, cmdType)) {
                printf("Error: Invalid command format\n");
                printf("Expected format:\n");
                switch (cmdType) {
                    case CMD_ADD_PARKING:
                        printf("addParking -member_X YYYY-MM-DD HH:MM n.n [facility1 facility2];\n");
                        break;
                    case CMD_ADD_RESERVATION:
                        printf("addReservation -member_X YYYY-MM-DD HH:MM n.n facility1 facility2(facility must contains battery and cable);\n");
                        break;
                    case CMD_ADD_EVENT:
                        printf("addEvent -member_X YYYY-MM-DD HH:MM n.n facility1 facility2 facility3(facility must contains umbrella valetpark);\n");
                        break;
                    case CMD_BOOK_ESSENTIALS:
                        printf("bookEssentials -member_X YYYY-MM-DD HH:MM n.n facility;\n");
                        break;
                    default:
                        break;
                }
                continue;
            }

            // Process the valid command
            processBooking(line);
        }
    }
}

// for the single booking
void processBooking(const char* command) {
    Booking booking = {0};
    char cmd[MAX_NAME_LENGTH];
    
    // command type and member name
    if (sscanf(command, "%s -%s", cmd, booking.memberName) != 2) {
        printf("Error: Invalid command format\n");
        return;
    }

    if (!validateMember(booking.memberName)) {
        printf("Error: Invalid member name\n");
        return;
    }

    // get the day, time, and duration
    char* ptr = strstr(command, booking.memberName) + strlen(booking.memberName);
    if (sscanf(ptr, "%s %s %f", booking.date, booking.time, &booking.duration) != 3) {
        printf("Error: Invalid date/time format\n");
        return;
    }

    // check the input day and time
    if (!validateDateTime(booking.date, booking.time)) {
        printf("Error: Invalid date or time\n");
        return;
    }

    // get the facility
    booking.facilityCount = 0;
    ptr = strchr(ptr, ' '); // date
    ptr = strchr(ptr + 1, ' '); // time
    ptr = strchr(ptr + 1, ' '); // duration
    
    if (ptr) {
        // skip space after duration
        ptr+=4;
        // Check if the next character is a semicolon
        if (*ptr == ';') {
            // No facilities provided
            booking.facilityCount = 0;
        } else {
            // There are facilities
            char temp[MAX_COMMAND_LENGTH];
            strcpy(temp, ptr);
            char* facility = strtok(temp, " ;"); // Split by space or semicolon
            
            while (facility && booking.facilityCount < MAX_FACILITIES) {
                strcpy(booking.facilities[booking.facilityCount], facility);
                booking.facilityCount++;
                facility = strtok(NULL, " ;");
            }
        }
    }

    // Check facilities for addParking(testing only)


    // print the booking message（for testing only）
    printf("\nBooking Details:\n");
    printf("Member: %s\n", booking.memberName);
    printf("Date: %s\n", booking.date);
    printf("Time: %s\n", booking.time);
    printf("Duration: %.1f hours\n", booking.duration);
    if (strcmp(cmd, "addParking") == 0) {
    // Print facilities for addParking
        if (booking.facilityCount > 0) {
            printf("Facilities: ");
            int i = 0;
            for (i = 0; i < booking.facilityCount; i++) {
                printf("%s ", booking.facilities[i]);
             }
            printf("\n");
        } else {
        printf("Facilities: null\n"); // null for no facilities
        }
    } else if (strcmp(cmd, "addReservation") == 0 || strcmp(cmd, "addEvent") == 0) {
        // Check the facilities input is valid for these commands
        if (!validateFacilities(booking.facilities, booking.facilityCount)) {
            printf("Error: Invalid facilities\n");
            return;
        } else {
        int i = 0;
        for (i = 0; i < booking.facilityCount; i++) {
            printf("Facility %d: %s\n", i + 1, booking.facilities[i]);
        }
    }
    }
    printf("Booking request processed successfully\n\n");
}

// function for check the member name is valid
bool validateMember(const char* memberName) {
    const char* validMembers[] = {"member_A", "member_B", "member_C", "member_D", "member_E"};
    int i = 0;
    for (i = 0; i < MAX_MEMBERS; i++) {
        if (strcmp(memberName, validMembers[i]) == 0) {
            return true;
        }
    }
    return false;
}

// check the input date time is correct
bool validateDateTime(const char* date, const char* time) {
    int year, month, day, hour, minute;
    
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3) {
        return false;
    }
    
    if (sscanf(time, "%d:%d", &hour, &minute) != 2) {
        return false;
    }

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return false;
    }

    return true;
}

// function for check the facilities
bool validateFacilities(char facilities[][MAX_FACILITY_NAME_LENGTH], int count) {
    const char* validFacilities[] = {"battery", "cable", "locker", "umbrella", "inflation", "valetpark"};
    
    int i = 0;
    int j = 0;
    for (i = 0; i < count; i++) {
        bool valid = false;
        for (j = 0; j < 6; j++) {
            if (strcmp(facilities[i], validFacilities[j]) == 0) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            return false;
        }
    }
    return true;
}

// function for check the command type
CommandType parseCommandType(const char* command) {
    if(strncmp(command, "addParking", 10) == 0) return CMD_ADD_PARKING;
    if(strncmp(command, "addReservation", 14) == 0) return CMD_ADD_RESERVATION;
    if(strncmp(command, "addEvent", 8) == 0) return CMD_ADD_EVENT;
    if(strncmp(command, "bookEssentials", 14) == 0) return CMD_BOOK_ESSENTIALS;
    return CMD_INVALID;
}

// if book facility combination, make sure all the facilities are valid and free for booking
bool validateFacilityCombination(const char* facility1, const char* facility2) {
    if(!facility1 || !facility2) return true; 
    
    // check valid facility combinations
    if((strcmp(facility1, "battery") == 0 && strcmp(facility2, "cable") == 0) ||
       (strcmp(facility1, "cable") == 0 && strcmp(facility2, "battery") == 0)) {
        return true;
    }
    
    if((strcmp(facility1, "locker") == 0 && strcmp(facility2, "umbrella") == 0) ||
       (strcmp(facility1, "umbrella") == 0 && strcmp(facility2, "locker") == 0)) {
        return true;
    }
    
    if((strcmp(facility1, "inflation") == 0 && strcmp(facility2, "valetpark") == 0) ||
       (strcmp(facility1, "valetpark") == 0 && strcmp(facility2, "inflation") == 0)) {
        return true;
    }
    
    return false;
}

// check if the input for the command is correct
bool validateCommandFormat(const char* command, CommandType cmdType) {
    char cmd[MAX_COMMAND_LENGTH];
    char member[MAX_NAME_LENGTH];
    char date[MAX_DATE_LENGTH];
    char time[MAX_DATE_LENGTH];
    char duration[MAX_NAME_LENGTH];
    char facility1[MAX_FACILITY_NAME_LENGTH] = "";
    char facility2[MAX_FACILITY_NAME_LENGTH] = "";
    char facility3[MAX_FACILITY_NAME_LENGTH] = "";

    int parsed;
    switch(cmdType) {
        case CMD_ADD_PARKING:
            // Allow for 5 or 7 parsed items (with or without facilities)
            parsed = sscanf(command, "%s -%s %s %s %s %s %s", 
                          cmd, member, date, time, duration, facility1, facility2);
            if (parsed < 5 || parsed > 7) return false; 

            if (facility1[0] && facility2[0]) {
                if(!validateFacilityCombination(facility1, facility2)){
                    printf("the facility should be in pair: cable&battery, locker&umbrella, valetpark&inflation");
                    return false;
                } 
            }
            break;
            
        case CMD_ADD_RESERVATION:
            // Ensure that both battery and cable are provided
            parsed = sscanf(command, "%s -%s %s %s %s %s %s", 
                          cmd, member, date, time, duration, facility1, facility2);
            if (parsed != 7 || ((strcmp(facility1, "battery") != 0 && strcmp(facility2, "cable") != 0) &&
            (strcmp(facility2, "battery") != 0 && strcmp(facility1, "cable") != 0))) {
                return false; // Must have 7 items and include both battery and cable
            }
           
            break;
            
        case CMD_ADD_EVENT:
            // Ensure that both umbrella and valetpark are provided
            parsed = sscanf(command, "%s -%s %s %s %s %s %s %s", 
                          cmd, member, date, time, duration, facility1, facility2, facility3);
            if (parsed != 8 || (strcmp(facility1, "umbrella") != 0 && strcmp(facility2, "valetpark") != 0) &&
            (strcmp(facility1, "umbrella") != 0 && strcmp(facility3, "valetpark") != 0) &&
            (strcmp(facility2, "umbrella") != 0 && strcmp(facility1, "valetpark") != 0) &&
            (strcmp(facility2, "umbrella") != 0 && strcmp(facility3, "valetpark") != 0) &&
            (strcmp(facility3, "umbrella") != 0 && strcmp(facility1, "valetpark") != 0) &&
            (strcmp(facility3, "umbrella") != 0 && strcmp(facility2, "valetpark") != 0)) {
                return false; // Must have 8 items and include both umbrella and valetpark
            }
            break;
            
        case CMD_BOOK_ESSENTIALS:
            parsed = sscanf(command, "%s -%s %s %s %s %s", 
                          cmd, member, date, time, duration, facility1);
            if(parsed != 6) return false;
            break;
            
        default:
            return false;
    }
    

    return true;
}

bool validateAddBatchCommand(const char* command) {
    if (strncmp(command, "addBatch -", 10) != 0) {
        printf("there should be a '-' before the file\n");
        return false;
    }
    const char* filename = strchr(command, '-');
    if (filename) {
        filename++;
        if (strstr(filename, ".dat") == NULL) {
            printf("the file name should be end with .dat\n");
            return false;
        }
    }

    return true;
}
