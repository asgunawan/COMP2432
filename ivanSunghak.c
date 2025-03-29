#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_NAME_LENGTH 50
#define MAX_DATE_LENGTH 20
#define MAX_MEMBERS 5
#define MAX_BOOKINGS 500
#define MAX_FACILITIES 3
#define MAX_FACILITY_NAME_LENGTH 20
#define MAX_ALGORITHMS 3
#define TOTAL_PARKING_SLOTS 10
#define INF 1000000000

typedef enum {
    CMD_ADD_PARKING,
    CMD_ADD_RESERVATION,
    CMD_ADD_EVENT,
    CMD_BOOK_ESSENTIALS,
    CMD_PRINT_BOOKING,
    CMD_INVALID
} CommandType;

typedef struct {
    int id;
    char memberName[MAX_NAME_LENGTH];
    char date[MAX_DATE_LENGTH];
    char time[MAX_DATE_LENGTH];
    float duration;
    char type[MAX_NAME_LENGTH];
    char facilities[MAX_FACILITIES][MAX_FACILITY_NAME_LENGTH];
    int facilityCount;
} Booking;

typedef struct {
    int id;
    int parking_slot;
    int start_time;
    int end_time;
    char status[MAX_NAME_LENGTH];
} Schedule;


void initializePipes();
bool validateMember(const char* memberName);
bool validateDateTime(const char* date, const char* time);
bool validateFacilities(char facilities[][MAX_FACILITY_NAME_LENGTH], int count);
void processBooking(const char* command);
void processInput(FILE* input, bool isBatchFile);
CommandType parseCommandType(const char* command);
bool validateCommandFormat(const char* command, CommandType cmdType);
bool validateFacilityCombination(const char* facility1, const char* facility2);
bool validateAddBatchCommand(const char* command);
bool validatePrintBooking(const char* algorithm);
void printBookings(const char* algorithm, int pipe_fd);
bool validateDuration(float duration);
const char* getAlgorithmName(int index);
void fcfs_schedule_to_pipe(int pipe_fd);
void shortest_job_first_to_pipe(int pipe_fd);
void priority_schedule_to_pipe(int pipe_fd);

Booking booking[MAX_BOOKINGS];
int booking_count = 0;



int main() {
    printf("~~Welcome to PolyU~~\n");

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
            printf("->Bye!\n");
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
        else if (!isBatchFile && strncmp(line, "printBookings", 12) == 0) {
                char cmd[MAX_NAME_LENGTH];
                char algorithm[10];

                if (sscanf(line, "%s -%s", cmd, algorithm) != 2) {
                    printf("Error: Invalid printBookings command format.\nUsage: printBookings -[algorithm(fcfs,sjf,prio,all)]\n");
                    continue;
                }

                if (!validatePrintBooking(algorithm)) {
                    printf("Error: Invalid algorithm '%s'. Must be 'fcfs', 'sjf', 'prio', or 'all'.\n", algorithm);
                    continue;
                }

                int pipe_fd[2];
                pid_t pid;
                if (pipe(pipe_fd) == -1) {
                    perror("pipe error");
                    exit(EXIT_FAILURE);
                }
                pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                if (pid == 0) {
                    close(pipe_fd[1]); // Close unused write end
                    printBookings(algorithm, pipe_fd[0]);
                    close(pipe_fd[0]);
                }
                else {
                    if (strcmp(algorithm, "fcfs") == 0) {
                        close(pipe_fd[0]);
                        fcfs_schedule_to_pipe(pipe_fd[1]);
                        close(pipe_fd[1]);
                    }
                    else if (strcmp(algorithm, "priority") == 0) {
                        close(pipe_fd[0]);
                        priority_schedule_to_pipe(pipe_fd[1]);
                        close(pipe_fd[1]);
                    }
                    else if (strcmp(algorithm, "sjf") == 0) {
                        close(pipe_fd[0]);
                        shortest_job_first_to_pipe(pipe_fd[1]);
                        close(pipe_fd[1]);
                    }
                    else if (strcmp(algorithm, "all") == 0) {
                        close(pipe_fd[0]);
                        fcfs_schedule_to_pipe(pipe_fd[1]);
                        priority_schedule_to_pipe(pipe_fd[1]);
                        shortest_job_first_to_pipe(pipe_fd[1]);
                        close(pipe_fd[1]);
                    }
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
                printf("Error: Invalid command format111\n");
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


void processBooking(const char* command) {

    char cmd[MAX_NAME_LENGTH];
    
    // command type and member name
    if (sscanf(command, "%s -%s", cmd, booking[booking_count].memberName) != 2) {
        printf("Error: Invalid command format_member\n");
        return;
    }
    strncpy(booking[booking_count].type, cmd, MAX_NAME_LENGTH - 1);
    booking[booking_count].type[MAX_NAME_LENGTH - 1] = '\0';

    if (!validateMember(booking[booking_count].memberName)) {
        printf("Error: Invalid member name\n");
        return;
    }

    // get the day, time, and duration
    char* ptr = strstr(command, booking[booking_count].memberName) + strlen(booking[booking_count].memberName);
    if (sscanf(ptr, "%s %s %f", booking[booking_count].date, booking[booking_count].time, &booking[booking_count].duration) != 3) {
        printf("Error: Invalid date/time format\n");
        return;
    }

    if (!validateDuration(booking[booking_count].duration)) {
        printf("Error: Invalid duration");
        return;
    }

    // check the input day and time
    if (!validateDateTime(booking[booking_count].date, booking[booking_count].time)) {
        printf("Error: Invalid date or time\n");
        return;
    }

    // get the facility
    // booking.facilityCount = 0;
    // ptr = strchr(ptr, ' '); // date
    // ptr = strchr(ptr + 1, ' '); // time
    // ptr = strchr(ptr + 1, ' '); // duration
    
    // if (ptr) {
    //     // skip space after duration
    //     ptr+=4;
    //     // Check if the next character is a semicolon
    //     if (*ptr == ';') {
    //         // No facilities provided
    //         booking.facilityCount = 0;
    //     } else {
    //         // There are facilities
    //         char temp[MAX_COMMAND_LENGTH];
    //         strcpy(temp, ptr);
    //         char* facility = strtok(temp, " ;"); // Split by space or semicolon
            
    //         while (facility && booking.facilityCount < MAX_FACILITIES) {
    //             strcpy(booking.facilities[booking.facilityCount], facility);
    //             booking.facilityCount++;
    //             facility = strtok(NULL, " ;");
    //         }
    //     }
    // }




    // print the booking message（for testing only）
    // printf("\nBooking Details:\n");
    // printf("Member: %s\n", booking.memberName);
    // printf("Date: %s\n", booking.date);
    // printf("Time: %s\n", booking.time);
    // printf("Duration: %.1f hours\n", booking.duration);
    // if (strcmp(cmd, "addParking") == 0) {
    // // Print facilities for addParking
    //     if (booking.facilityCount > 0) {
    //         printf("Facilities: ");
    //         int i = 0;
    //         for (i = 0; i < booking.facilityCount; i++) {
    //             printf("%s ", booking.facilities[i]);
    //          }
    //         printf("\n");
    //     } else {
    //     printf("Facilities: null\n"); // null for no facilities
    //     }
    // } else if (strcmp(cmd, "addReservation") == 0 || strcmp(cmd, "addEvent") == 0) {
    //     // Check the facilities input is valid for these commands
    //     if (!validateFacilities(booking.facilities, booking.facilityCount)) {
    //         printf("Error: Invalid facilities\n");
    //         return;
    //     } else {
    //     int i = 0;
    //     for (i = 0; i < booking.facilityCount; i++) {
    //         printf("Facility %d: %s\n", i + 1, booking.facilities[i]);
    //     }
    // }
    // }
    // printf("Booking request processed successfully\n\n");
    if (booking_count < MAX_BOOKINGS) {
        char* facilitiesPtr = strstr(ptr, booking[booking_count].date);
        if (facilitiesPtr) {
            facilitiesPtr += strlen(booking[booking_count].date) + 1;
            facilitiesPtr += strspn(facilitiesPtr, " "); 
            facilitiesPtr += strcspn(facilitiesPtr, " "); 
            facilitiesPtr += strspn(facilitiesPtr, " ");

            // get the duration here
            float duration = 0.0f;
            if (sscanf(facilitiesPtr, "%f", &duration) != 1) {
                printf("Error: Invalid duration format\n");
                return;
            }
            booking[booking_count].duration = duration;

            facilitiesPtr += strcspn(facilitiesPtr, " ");
            facilitiesPtr += strspn(facilitiesPtr, " ");

            // count the number of facilities
            booking[booking_count].facilityCount = 0;
            char* facility = strtok(facilitiesPtr, " ;");
            
            while (facility && booking[booking_count].facilityCount < MAX_FACILITIES) {
                strncpy(booking[booking_count].facilities[booking[booking_count].facilityCount], facility, MAX_FACILITY_NAME_LENGTH - 1);
                booking[booking_count].facilities[booking[booking_count].facilityCount][MAX_FACILITY_NAME_LENGTH - 1] = '\0';
                booking[booking_count].facilityCount++;
                facility = strtok(NULL, " ;"); 
            }
        }

        booking_count++;
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//the way to use the data inside
//make sure the index of the command is booking_count-1

//     printf("Member Name: %s\n", booking[booking_count-1].memberName);
//     printf("Date: %s\n", booking[booking_count-1].date);
//     printf("Time: %s\n", booking[booking_count-1].time);
//     printf("Duration: %.2f hours\n", booking[booking_count-1].duration);
//     printf("Type: %s\n", booking[booking_count-1].type);
//     printf("Facilities (%d): ", booking[booking_count-1].facilityCount);

// //use the for loop and index to get the facilities
//     for (int i = 0; i < booking[booking_count-1].facilityCount; i++) {
//         printf("%s ", booking[booking_count-1].facilities[i]);
//     }
//     printf("\n");
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

    if (year != 2025 || month != 5 || day > 16 || day < 10) {
        printf("the date should between 2025-5-10 to 2025-5-16\n");
        return false;
    }    

    if (hour < 0 || hour > 23) {
        return false;
    }

    if (minute != 0) {
        printf("Time must be on the hour\n");
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
    char algorithm[4] = "";

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

bool validatePrintBooking(const char* algorithm) {
    if (strcmp(algorithm, "sjf") == 0 ||
        strcmp(algorithm, "fcfs") == 0 ||
        strcmp(algorithm, "all") == 0 ||
        strcmp(algorithm, "prio") == 0) {
        return true;
    }
    
    printf("the input algorithm can be sjf, fcfs, prio & all\n");
    return false;
}

bool validateDuration(float duration) {

    if (duration < 0) {
        printf("Duration cannot be negative\n");
        return false;
    }
    

    float whole = (float)(int)duration;
    if (duration != whole && duration != whole + 0.0f) {
        printf("Duration must be a whole number or end with .0\n");
        return false;
    }
    
    return true;
}


const char* getAlgorithmName(int index) {
    switch (index) {
        case 0: return "fcfs";
        case 1: return "sjf";
        case 2: return "prio";
        default: return "unknown";
    }
}

/*added by sunghak*/
Schedule accepted[MAX_MEMBERS][MAX_BOOKINGS];
int accepted_count[MAX_MEMBERS] = {0};

Schedule rejected[MAX_MEMBERS][MAX_BOOKINGS];
int rejected_count[MAX_MEMBERS] = {0};

Booking refer_booking[MAX_BOOKINGS]; //refer bookings with s.id

Booking bookings[MAX_BOOKINGS];
Schedule schedule[MAX_BOOKINGS];


int schedule_count = 0;

int convertTimeToInt(const char* time_str) {
    int h,m;
    sscanf(time_str, "%d:%d", &h, &m);
    return h;
}
void fcfs_schedule_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; // Track end times for 3 parking slots
    char buffer[256];

    for (int i = 0; i < booking_count; i++) {
        int assigned = 0;
        int startTime = convertTimeToInt(bookings[i].time);
        for (int j = 0; j < 3; j++) {
            if (parking_slots[j] <= startTime) {
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = startTime;
                schedule[schedule_count].end_time = startTime + bookings[i].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[j] = schedule[schedule_count].end_time;

                // Write schedule to pipe
                snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                         schedule[schedule_count].id,
                         bookings[schedule_count].memberName,
                         bookings[schedule_count].type,
                         schedule[schedule_count].parking_slot,
                         schedule[schedule_count].start_time,
                         schedule[schedule_count].end_time,
                         schedule[schedule_count].status);
                write(pipe_fd, buffer, strlen(buffer));
                schedule_count++;
                assigned = 1;
                break;
            }
        }
        if (!assigned) {
            schedule[schedule_count].id = bookings[i].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = startTime;
            schedule[schedule_count].end_time = startTime + bookings[i].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            // Write rejected schedule to pipe
            snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                     schedule[schedule_count].id,
                     bookings[schedule_count].memberName,
                     bookings[schedule_count].type,
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].start_time,
                     schedule[schedule_count].end_time,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));
            schedule_count++;
        }
    }
}
void shortest_job_first_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; // Track end times for 10 parking slots
    char buffer[256];
    int curr_time = 0;
    int curr_scheduled = 0;
    int is_scheduled[MAX_BOOKINGS] = {0};

    // sort bookings in increasing order of start time, and if the same, in increasing order of duration
    for (int i = 0; i < booking_count; i++) {
        int startTime1 = convertTimeToInt(bookings[i].time);
        for (int j = i+1; j < booking_count; j++) {
            int startTime2 = convertTimeToInt(bookings[j].time);
            if (startTime1 > startTime2 || 
                (startTime1 == startTime2 && bookings[i].duration > bookings[j].duration)) {
                    Booking temp = bookings[i];
                    bookings[i] = bookings[j];
                    bookings[j] = temp;
            }
        }
    }

    while (curr_scheduled < booking_count) {
        int selected = -1; //selected booking to be scheduled
        int min_duration = INF; //big number

        //select a booking to be scheduled. It is like a selection sort.
        for (int i = 0; i < booking_count; i++) {
            int startTime = convertTimeToInt(bookings[i].time);
            if (is_scheduled[i]) continue;
            
            if (startTime <= curr_time) {
                if (bookings[i].duration < min_duration) {
                    selected = i;
                    min_duration = bookings[i].duration;
                }
            }
        }

        //when no schedule is selected for the current time, move the current time to the next schedule
        if (selected == -1) {
            int next_time = INF; //big number
            for (int i = 0; i < booking_count; i++) {
                int startTime = convertTimeToInt(bookings[i].time);
                if (!is_scheduled[i] && startTime > curr_time) {
                    if (startTime < next_time) {
                        next_time = startTime;
                    }
                }
            }
            curr_time = next_time;
            continue;
        }

        int is_assigned = 0;
        int startTime = convertTimeToInt(bookings[selected].time);

        for (int i = 0; i < TOTAL_PARKING_SLOTS; i++) {
            if (parking_slots[i] <= startTime) {
                schedule[schedule_count].id = bookings[selected].id;
                schedule[schedule_count].parking_slot = i + 1;
                schedule[schedule_count].start_time = startTime;
                schedule[schedule_count].end_time = startTime + bookings[selected].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[i] = schedule[schedule_count].end_time;

                snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                         schedule[schedule_count].id,
                         bookings[selected].memberName,
                         bookings[selected].type,
                         schedule[schedule_count].parking_slot,
                         schedule[schedule_count].start_time,
                         schedule[schedule_count].end_time,
                         schedule[schedule_count].status);
                write(pipe_fd, buffer, strlen(buffer));

                schedule_count++;
                is_scheduled[selected] = 1;
                is_assigned = 1;
                curr_scheduled++;
                break;
            }
        }

        //when rejected
        if (!is_assigned) {
            schedule[schedule_count].id = bookings[selected].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = startTime;
            schedule[schedule_count].end_time = startTime + bookings[selected].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                     schedule[schedule_count].id,
                     bookings[selected].memberName,
                     bookings[selected].type,
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].start_time,
                     schedule[schedule_count].end_time,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));

            schedule_count++;
            is_scheduled[selected] = 1;
            curr_scheduled++;
        }
    }
}
// Helper function to determine priority
int get_priority(const char *type) {
    if (strcmp(type, "Event") == 0) return 1;
    if (strcmp(type, "Reservation") == 0) return 2;
    if (strcmp(type, "Parking") == 0) return 3;
    if (strcmp(type, "Essentials") == 0) return 4;
    return 5; // Default priority for unknown types
}

void priority_schedule_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; // Track end times for 3 parking slots
    char buffer[256];

    // Sort bookings by priority (lower priority value = higher priority)
    for (int i = 0; i < booking_count - 1; i++) {
        for (int j = i + 1; j < booking_count; j++) {
            if (get_priority(bookings[i].type) > get_priority(bookings[j].type)) {
                Booking temp = bookings[i];
                bookings[i] = bookings[j];
                bookings[j] = temp;
            }
        }
    }

    // Schedule bookings based on priority
    for (int i = 0; i < booking_count; i++) {
        int assigned = 0;
        int startTime = convertTimeToInt(bookings[i].time);
        for (int j = 0; j < 3; j++) { // Check each parking slot

            if (parking_slots[j] <= startTime) {
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = startTime;
                schedule[schedule_count].end_time = startTime + bookings[i].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[j] = schedule[schedule_count].end_time;

                // Write schedule to pipe
                snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                         schedule[schedule_count].id,
                         bookings[i].memberName,
                         bookings[i].type,
                         schedule[schedule_count].parking_slot,
                         schedule[schedule_count].start_time,
                         schedule[schedule_count].end_time,
                         schedule[schedule_count].status);
                write(pipe_fd, buffer, strlen(buffer));
                schedule_count++;
                assigned = 1;
                break;
            }
        }
        if (!assigned) { // If no slot is available, reject the booking
            schedule[schedule_count].id = bookings[i].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = startTime;
            schedule[schedule_count].end_time = startTime + bookings[i].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            // Write rejected schedule to pipe
            snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                     schedule[schedule_count].id,
                     bookings[i].memberName,
                     bookings[i].type,
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].start_time,
                     schedule[schedule_count].end_time,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));
            schedule_count++;
        }
    }
}


//ex) member_A returns 0, member_C returns 2
int get_index_from_member(char* member) {
    if (strncmp(member, "member_", 7) == 0) {
        char ch = member[7];
        if (ch >= 'A' && ch <= 'E') return ch - 'A';
    }
    return -1;
}

void parse_and_classify_line(const char* line) {
    Schedule s;
    Booking b;
    if (sscanf(line, "%d %s %s %d %d %d %s %s",
               &s.id, b.memberName, b.type, &s.parking_slot, &s.start_time, &s.end_time, s.status, b.date) != 8) {
        return;
    }
    int member_idx = get_index_from_member(b.memberName);
    if (member_idx == -1) return;

    refer_booking[s.id] = b;

    if (strcmp(s.status, "Scheduled") == 0) {
        accepted[member_idx][accepted_count[member_idx]++] = s;
    } else {
        rejected[member_idx][rejected_count[member_idx]++] = s;
    }
}
// should replace by the printBooking function;
void printBookings(const char* algorithm, int pipe_fd) {
    char buffer[256];
    int bytes_read;
    int buf_len = 0;

    //read lines from parent
    while ((bytes_read = read(pipe_fd, buffer+buf_len, sizeof(buffer)-buf_len-1)) > 0) {
        buf_len += bytes_read;
        buffer[buf_len] = '\0';

        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            parse_and_classify_line(line);
            line = strtok(NULL, "\n");
        }
        buf_len = 0;
    }

    //accepted bookings
    printf("\n*** Parking Booking - ACCEPTED / %s ***\n\n", algorithm);
    for (int i = 0; i < MAX_MEMBERS; i++) {
        printf("Member_%c has the following bookings:\n\n", 'A' + i);

        if (accepted_count[i] == 0) {
            printf("No accepted bookings.\n\n");
            continue;
        }
        
        printf("%-12s%-8s%-8s%-12s%-10s\n", "Date", "Start", "End", "Type", "Device");
        printf("==========================================================\n");
        for (int j = 0; j < accepted_count[i]; j++) {
            Schedule s = accepted[i][j];
            Booking b = refer_booking[s.id];

            printf("%-12s%-8d%-8s\n", b.date, s.start_time, s.end_time, b.type);
        }
        printf("\n");
    }

    //rejected bookings
    printf("\n*** Parking Booking - REJECTED / %s ***\n\n", algorithm);
    for (int i = 0; i < MAX_MEMBERS; i++) {
        if (rejected_count[i] == 0) continue;

        printf("Member_%c (%d bookings rejected):\n\n", 'A' + i, rejected_count[i]);
        printf("%-12s%-8s%-8s%-20s%-8s\n", "Date", "Start", "End", "Type", "Essentials");
        printf("==========================================================\n");

        for (int j = 0; j < rejected_count[i]; j++) {
            Schedule s = rejected[i][j];
            Booking b = refer_booking[s.id];

            printf("%-12s%02d:00   %02d:00   %-20s\n", b.date, s.start_time, s.end_time, b.type);
        }
        printf("\n");
    }
}