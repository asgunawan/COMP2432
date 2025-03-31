#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h> 
#include <sys/types.h>

// Constants
#define INF 1000000000
#define MAX_LINE 256
#define MAX_BOOKINGS 100
#define MAX_NAME_LEN 50
#define NUM_OF_MEMBER 5
#define MAX_DATE_LEN 20
#define MAX_FACILITIES 6
#define MAX_FACILITY_NAME_LENGTH 20
#define TOTAL_PARKING_SLOTS 10
#define MAX_COMMAND_LENGTH 256
#define NUM_ALGORITHMS 3

// Enums
typedef enum {
    CMD_INVALID,
    CMD_ADD_PARKING,
    CMD_ADD_RESERVATION,
    CMD_ADD_EVENT,
    CMD_BOOK_ESSENTIALS
} CommandType;

// Structures
typedef struct {
    int id;
    char client[MAX_NAME_LEN];
    char type[MAX_NAME_LEN];
    char date[MAX_DATE_LEN];
    char time[MAX_NAME_LEN];
    float duration;
    char facilities[MAX_FACILITIES][MAX_FACILITY_NAME_LENGTH];
    int facility_count;
} Booking;

typedef struct {
    int id;
    int parking_slot;
    int start_time;
    int end_time;
    char status[MAX_NAME_LEN];
} Schedule;

// Global Variables
Schedule accepted[NUM_OF_MEMBER][MAX_BOOKINGS];
int accepted_count[NUM_OF_MEMBER] = {0};

Schedule rejected[NUM_OF_MEMBER][MAX_BOOKINGS];
int rejected_count[NUM_OF_MEMBER] = {0};

Booking refer_booking[MAX_BOOKINGS]; // Refer bookings with s.id
Booking bookings[MAX_BOOKINGS];
Schedule schedule[MAX_BOOKINGS];

int facility_availability[MAX_FACILITIES] = {3, 3, 3, 3, 3, 3}; // 3 of each facility
const char* facility_names[MAX_FACILITIES] = {"battery", "cable", "umbrella", "locker", "valetpark", "inflation"};

int summary[NUM_ALGORITHMS][4] = {0}; // [algorithm][metric]
// Metrics: [0] = Total bookings, [1] = Assigned, [2] = Rejected, [3] = Invalid

int booking_count = 0;
int schedule_count = 0;

// Function Prototypes
void processInput(FILE* input, bool isBatchFile);
void add_booking(const char* command, int id);
void fcfs_schedule_to_pipe(int pipe_fd);
void shortest_job_first_to_pipe(int pipe_fd);
void priority_schedule_to_pipe(int pipe_fd);
void printBookings(const char* algorithm, int pipe_fd);
void run_scheduling(const char* algorithm, void (*schedule_function)(int));
bool validateAddBatchCommand(const char* command);
bool validatePrintBooking(const char* algorithm);
CommandType parseCommandType(const char* command);
bool validateCommandFormat(const char* command, CommandType cmdType);
void processBooking(const char* command);
int convert_time_to_int(const char* time_str);
int convert_date_to_int(const char* date);
void parse_and_classify_line(const char* line);
int get_priority(const char* type);
int get_index_from_member(char* member);
void load_dummy_data();
void initialize_pipe(int pipe_fd[2]);
pid_t fork_process();
void handle_child_process(int pipe_fd[2], const char* algorithm);
void handle_parent_process(int pipe_fd[2], void (*schedule_function)(int));
void reset_global_variables(bool reset_summary);
void generate_performance_report();
bool check_and_reserve_facilities(Booking* b);

void reset_global_variables(bool reset_summary) {
    // Reset accepted and rejected schedules
    for (int i = 0; i < NUM_OF_MEMBER; i++) {
        accepted_count[i] = 0;
        rejected_count[i] = 0;
    }

    // Reset schecdule
    schedule_count = 0;

    // Reset facility availability
    for (int i = 0; i < MAX_FACILITIES; i++) {
        facility_availability[i] = 3; // Reset to initial availability (3 of each facility)
    }

    // Reset summary metrics only if specified
    if (reset_summary) {
        memset(summary, 0, sizeof(summary));
    }
}

void generate_performance_report() {
    printf("\n\n*** Parking Booking Manager - Summary Report ***\n\n");

    const char* algorithms[] = {"FCFS", "PRIO", "SJF"};
    for (int algo = 0; algo < NUM_ALGORITHMS; algo++) {
        printf("Performance:\n\n");
        printf("  For %s:\n", algorithms[algo]);

        int total = summary[algo][0];
        int assigned = summary[algo][1];
        int rejected = summary[algo][2];
        int invalid = summary[algo][3];

        printf("          Total Number of Bookings Received: %d (100.0%%)\n", total);
        printf("                Number of Bookings Assigned: %d (%.1f%%)\n", assigned, (total > 0) ? (assigned * 100.0 / total) : 0.0);
        printf("                Number of Bookings Rejected: %d (%.1f%%)\n", rejected, (total > 0) ? (rejected * 100.0 / total) : 0.0);
        printf("\n          Invalid request(s) made: %d\n\n", invalid);
    }
}

// Utility Functions
int convert_time_to_int(const char* time_str) {
    int h, m;
    sscanf(time_str, "%d:%d", &h, &m);
    return h;
}

int convert_date_to_int(const char* date) {
    int y, m, d;
    sscanf(date, "%d-%d-%d", &y, &m, &d);
    return y * 10000 + m * 100 + d;
}

int get_priority(const char* type) {
    if (strcmp(type, "Event") == 0) return 1;
    if (strcmp(type, "Reservation") == 0) return 2;
    if (strcmp(type, "Parking") == 0) return 3;
    if (strcmp(type, "Essentials") == 0) return 4;
    return 5; 
}

int get_index_from_member(char* member) {
    if (strncmp(member, "member_", 7) == 0) {
        char ch = member[7];
        if (ch >= 'A' && ch <= 'E') return ch - 'A';
    }
    return -1;
}

// Validation Functions
bool validateAddBatchCommand(const char* command) {
    return (strncmp(command, "addBatch -", 10) == 0);
}

bool validatePrintBooking(const char* algorithm) {
    return (strcmp(algorithm, "fcfs") == 0 ||
            strcmp(algorithm, "sjf") == 0 ||
            strcmp(algorithm, "prio") == 0 ||
            strcmp(algorithm, "all") == 0);
}

CommandType parseCommandType(const char* command) {
    if (strncmp(command, "addParking", 10) == 0) return CMD_ADD_PARKING;
    if (strncmp(command, "addReservation", 14) == 0) return CMD_ADD_RESERVATION;
    if (strncmp(command, "addEvent", 8) == 0) return CMD_ADD_EVENT;
    if (strncmp(command, "bookEssentials", 14) == 0) return CMD_BOOK_ESSENTIALS;
    return CMD_INVALID;
}

bool validateCommandFormat(const char* command, CommandType cmdType) {
    switch (cmdType) {
        case CMD_ADD_PARKING:
        case CMD_ADD_RESERVATION:
        case CMD_ADD_EVENT:
        case CMD_BOOK_ESSENTIALS:
            return (strchr(command, '-') != NULL); 
        default:
            return false;
    }
}

void processBooking(const char* command) {
    add_booking(command, booking_count); 
}

bool check_and_reserve_facilities(Booking* b) {
    int temp_availability[MAX_FACILITIES];
    memcpy(temp_availability, facility_availability, sizeof(facility_availability));

    for (int i = 0; i < b->facility_count; i++) {
        int facility_index = -1;
        for (int j = 0; j < MAX_FACILITIES; j++) {
            if (strcmp(b->facilities[i], facility_names[j]) == 0) {
                facility_index = j;
                break;
            }
        }

        if (facility_index == -1 || temp_availability[facility_index] <= 0) {
            return false; 
        }

        // Reserve the facility and its pair
        temp_availability[facility_index]--;
        if (facility_index % 2 == 0) { // Pairing logic: battery + cable, locker + umbrella, etc according to rules.
            temp_availability[facility_index + 1]--;
        } else {
            temp_availability[facility_index - 1]--;
        }
    }

    // Reserve facilities
    memcpy(facility_availability, temp_availability, sizeof(facility_availability));
    return true;
}

void release_facilities(Booking* b) {
    for (int i = 0; i < b->facility_count; i++) {
        for (int j = 0; j < MAX_FACILITIES; j++) {
            if (strcmp(b->facilities[i], facility_names[j]) == 0) {
                facility_availability[j]++;
                break;
            }
        }
    }
}

// Booking Functions
void add_booking(const char* command, int id) {
    if (booking_count >= MAX_BOOKINGS) {
        printf("Maximum booking limit reached.\n");
        return;
    }

    Booking* b = &bookings[booking_count];
    char facilities[MAX_LINE] = {0};

    // Parse the command
    if (sscanf(command, "%s -%s %s %s %f %[^\n]",
               b->type, b->client, b->date, b->time, &b->duration, facilities) < 5) {
        printf("Error: Invalid booking format.\n");
        return;
    }

    // Parse facilities
    b->id = id;
    b->facility_count = 0;
    char* facility = strtok(facilities, " ");
    while (facility && b->facility_count < MAX_FACILITIES) {
        strncpy(b->facilities[b->facility_count], facility, MAX_FACILITY_NAME_LENGTH - 1);
        b->facilities[b->facility_count][MAX_FACILITY_NAME_LENGTH - 1] = '\0';
        b->facility_count++;
        facility = strtok(NULL, " ");
    }

    booking_count++;
}

void fcfs_schedule_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0};
    char buffer[256];

    for (int i = 0; i < booking_count; i++) {
        summary[0][0]++; 
        int assigned = 0;
        int start_time = convert_time_to_int(bookings[i].time); 

        // Check facility availability and reject if no avaliable according to the rules (3 facilities)
        if (!check_and_reserve_facilities(&bookings[i])) {
            schedule[schedule_count].id = bookings[i].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = start_time;
            schedule[schedule_count].end_time = start_time + bookings[i].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                     schedule[schedule_count].id,
                     bookings[i].client,
                     bookings[i].type,
                     bookings[i].date,
                     bookings[i].time,
                     bookings[i].duration,
                     bookings[i].facility_count);
            for (int k = 0; k < bookings[i].facility_count; k++) {
                strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
            }
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));
            schedule_count++;
            summary[0][2]++; 
            continue;
        }

        for (int j = 0; j < TOTAL_PARKING_SLOTS; j++) {
            if (parking_slots[j] <= start_time) {
                // Assign the booking to the slot
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = start_time;
                schedule[schedule_count].end_time = start_time + bookings[i].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[j] = schedule[schedule_count].end_time;

                snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                         schedule[schedule_count].id,
                         bookings[i].client,
                         bookings[i].type,
                         bookings[i].date,
                         bookings[i].time,
                         bookings[i].duration,
                         bookings[i].facility_count);
                for (int k = 0; k < bookings[i].facility_count; k++) {
                    strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                    strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
                }
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                         schedule[schedule_count].parking_slot,
                         schedule[schedule_count].status);
                write(pipe_fd, buffer, strlen(buffer));
                schedule_count++;
                summary[0][1]++; 
                assigned = 1;
                break;
            }
        }

        if (!assigned) { // If no slot is available, reject the booking
            release_facilities(&bookings[i]); 
            schedule[schedule_count].id = bookings[i].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = start_time;
            schedule[schedule_count].end_time = start_time + bookings[i].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                     schedule[schedule_count].id,
                     bookings[i].client,
                     bookings[i].type,
                     bookings[i].date,
                     bookings[i].time,
                     bookings[i].duration,
                     bookings[i].facility_count);
            for (int k = 0; k < bookings[i].facility_count; k++) {
                strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
            }
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));
            schedule_count++;
            summary[0][2]++;
        }
    }
}

// DEBUG: Short dummy data
// void load_dummy_data() {
//     int id = 0;
//     add_booking("addParking -member_A 2025-05-12 08:00 3.0 battery cable", id++);
//     add_booking("addParking -member_B 2025-05-10 08:00 3.0 battery cable", id++);
//     add_booking("addReservation -member_C 2025-05-10 08:00 3.0 battery cable", id++);
//     add_booking("addEssentials -member_D 2025-05-10 09:00 2.0 battery", id++);
//     add_booking("addEvent -member_E 2025-05-10 11:00 3.0 umbrella valetpark", id++);
//     add_booking("addParking -member_A 2025-05-10 12:00 2.0 battery cable", id++);
// }

void shortest_job_first_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0};
    char buffer[256];
    int curr_time = 0;
    int curr_scheduled = 0;
    int is_scheduled[MAX_BOOKINGS] = {0};

    // Sort bookings in increasing order of date, start time, and duration.
    for (int i = 0; i < booking_count; i++) {
        for (int j = i + 1; j < booking_count; j++) {
            int start_time_i = convert_time_to_int(bookings[i].time);
            int start_time_j = convert_time_to_int(bookings[j].time);
            int date_i = convert_date_to_int(bookings[i].date);
            int date_j = convert_date_to_int(bookings[j].date);

            if (date_i > date_j || (date_i == date_j && start_time_i > start_time_j) || 
                (date_i == date_j && start_time_i == start_time_j && bookings[i].duration > bookings[j].duration)) {
                Booking temp = bookings[i];
                bookings[i] = bookings[j];
                bookings[j] = temp;
            }
        }
    }

    while (curr_scheduled < booking_count) {
        int selected = -1; 
        int min_duration = INF; // Big number

        // Select a booking to be scheduled
        for (int i = 0; i < booking_count; i++) {
            if (is_scheduled[i]) continue;

            int start_time = convert_time_to_int(bookings[i].time);
            if (start_time <= curr_time) {
                if (bookings[i].duration < min_duration) {
                    selected = i;
                    min_duration = bookings[i].duration;
                }
            }
        }

        // When no schedule is selected for the current time, move the current time to the next schedule
        if (selected == -1) {
            int next_time = INF; // Big number
            for (int i = 0; i < booking_count; i++) {
                int start_time = convert_time_to_int(bookings[i].time);
                if (!is_scheduled[i] && start_time > curr_time) {
                    if (start_time < next_time) {
                        next_time = start_time;
                    }
                }
            }
            curr_time = next_time;
            continue;
        }

        summary[2][0]++;
        int is_assigned = 0;
        int start_time = convert_time_to_int(bookings[selected].time);

        // Check facility availability and reject if no avaliable according to the rules (3 facilities)
        if (!check_and_reserve_facilities(&bookings[selected])) {
            schedule[schedule_count].id = bookings[selected].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = start_time;
            schedule[schedule_count].end_time = start_time + bookings[selected].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                     schedule[schedule_count].id,
                     bookings[selected].client,
                     bookings[selected].type,
                     bookings[selected].date,
                     bookings[selected].time,
                     bookings[selected].duration,
                     bookings[selected].facility_count);
            for (int k = 0; k < bookings[selected].facility_count; k++) {
                strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                strncat(buffer, bookings[selected].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
            }
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));

            schedule_count++;
            is_scheduled[selected] = 1;
            curr_scheduled++;
            summary[2][2]++; 
            continue;
        }

        for (int i = 0; i < TOTAL_PARKING_SLOTS; i++) {
            if (parking_slots[i] <= start_time) {
                schedule[schedule_count].id = bookings[selected].id;
                schedule[schedule_count].parking_slot = i + 1;
                schedule[schedule_count].start_time = start_time;
                schedule[schedule_count].end_time = start_time + bookings[selected].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[i] = schedule[schedule_count].end_time;

                snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                         schedule[schedule_count].id,
                         bookings[selected].client,
                         bookings[selected].type,
                         bookings[selected].date,
                         bookings[selected].time,
                         bookings[selected].duration,
                         bookings[selected].facility_count);
                for (int k = 0; k < bookings[selected].facility_count; k++) {
                    strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                    strncat(buffer, bookings[selected].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
                }
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                         schedule[schedule_count].parking_slot,
                         schedule[schedule_count].status);
                write(pipe_fd, buffer, strlen(buffer));

                schedule_count++;
                is_scheduled[selected] = 1;
                is_assigned = 1;
                curr_scheduled++;
                summary[2][1]++;
                break;
            }
        }

        if (!is_assigned) {
            release_facilities(&bookings[selected]);
        }
    }
}

void priority_schedule_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; 
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

    for (int i = 0; i < booking_count; i++) {
        summary[1][0]++;
        int assigned = 0;
        int start_time = convert_time_to_int(bookings[i].time);
        int end_time = start_time + bookings[i].duration;

        // Check facility availability and reject if no avaliable according to the rules (3 facilities)
        if (!check_and_reserve_facilities(&bookings[i])) {
            schedule[schedule_count].id = bookings[i].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = start_time;
            schedule[schedule_count].end_time = end_time;
            strcpy(schedule[schedule_count].status, "Rejected");

            snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                     schedule[schedule_count].id,
                     bookings[i].client,
                     bookings[i].type,
                     bookings[i].date,
                     bookings[i].time,
                     bookings[i].duration,
                     bookings[i].facility_count);
            for (int k = 0; k < bookings[i].facility_count; k++) {
                strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
            }
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));

            schedule_count++;
            summary[1][2]++; 
            continue;
        }

        // Check parking slot availability
        for (int j = 0; j < TOTAL_PARKING_SLOTS; j++) {
            if (parking_slots[j] <= start_time) {
                // Assign the booking to the slot
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = start_time;
                schedule[schedule_count].end_time = end_time;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[j] = end_time;

                snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                         schedule[schedule_count].id,
                         bookings[i].client,
                         bookings[i].type,
                         bookings[i].date,
                         bookings[i].time,
                         bookings[i].duration,
                         bookings[i].facility_count);
                for (int k = 0; k < bookings[i].facility_count; k++) {
                    strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                    strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
                }
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                         schedule[schedule_count].parking_slot,
                         schedule[schedule_count].status);
                write(pipe_fd, buffer, strlen(buffer));

                schedule_count++;
                summary[1][1]++; 
                assigned = 1;
                break;
            }
        }

        // If no slot, displace a lower prio booking
        if (!assigned) {
            for (int j = 0; j < schedule_count; j++) {
                Schedule* s = &schedule[j];
                Booking* b = &refer_booking[s->id];

                if (get_priority(bookings[i].type) < get_priority(b->type)) {
                    // Displace the lower-priority booking, release, then mark that its rejected
                    release_facilities(b); 
                    s->parking_slot = -1; 
                    strcpy(s->status, "Rejected");
                    summary[1][2]++;

                    // Assign the higher-priority booking
                    schedule[schedule_count].id = bookings[i].id;
                    schedule[schedule_count].parking_slot = s->parking_slot;
                    schedule[schedule_count].start_time = start_time;
                    schedule[schedule_count].end_time = end_time;
                    strcpy(schedule[schedule_count].status, "Scheduled");
                    parking_slots[s->parking_slot - 1] = end_time;

                    snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                             schedule[schedule_count].id,
                             bookings[i].client,
                             bookings[i].type,
                             bookings[i].date,
                             bookings[i].time,
                             bookings[i].duration,
                             bookings[i].facility_count);
                    for (int k = 0; k < bookings[i].facility_count; k++) {
                        strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                        strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
                    }
                    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                             schedule[schedule_count].parking_slot,
                             schedule[schedule_count].status);
                    write(pipe_fd, buffer, strlen(buffer));

                    schedule_count++;
                    summary[1][1]++;
                    assigned = 1;
                    break;
                }
            }
        }

        // If still not assigned, reject the booking
        if (!assigned) {
            release_facilities(&bookings[i]); 
            schedule[schedule_count].id = bookings[i].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = start_time;
            schedule[schedule_count].end_time = end_time;
            strcpy(schedule[schedule_count].status, "Rejected");

            snprintf(buffer, sizeof(buffer), "%d %s %s %s %s %.2f %d",
                     schedule[schedule_count].id,
                     bookings[i].client,
                     bookings[i].type,
                     bookings[i].date,
                     bookings[i].time,
                     bookings[i].duration,
                     bookings[i].facility_count);
            for (int k = 0; k < bookings[i].facility_count; k++) {
                strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
                strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
            }
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %d %s\n",
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].status);
            write(pipe_fd, buffer, strlen(buffer));

            schedule_count++;
            summary[1][2]++; 
        }
    }
}

/*
sample line is like this: 3 member_D addEssentials 2025-05-10 09:00 2.00 1 battery 1 Scheduled
parse each word by using strtok with "\n"
*/
void parse_and_classify_line(const char* line) {
    Schedule s;
    Booking b;
    char temp[256];
    strncpy(temp, line, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    // printf("%s\n", line);
    char *token = strtok(temp, " ");
    if (!token) return;
    s.id = atoi(token);
    b.id = s.id;

    token = strtok(NULL, " ");
    if (!token) return;
    strncpy(b.client, token, MAX_NAME_LEN);

    token = strtok(NULL, " ");
    if (!token) return;
    strncpy(b.type, token, MAX_NAME_LEN);

    token = strtok(NULL, " ");
    if (!token) return;
    strncpy(b.date, token, MAX_DATE_LEN);

    token = strtok(NULL, " ");
    if (!token) return;
    strncpy(b.time, token, MAX_DATE_LEN);

    token = strtok(NULL, " ");
    if (!token) return;
    b.duration = atof(token);

    token = strtok(NULL, " ");
    if (!token) return;
    b.facility_count = atoi(token);
    
    for (int i = 0; i < b.facility_count; i++) {
        token = strtok(NULL, " ");
        if (!token) return;
        strncpy(b.facilities[i], token, MAX_FACILITY_NAME_LENGTH - 1);
        b.facilities[i][MAX_FACILITY_NAME_LENGTH - 1] = '\0';
    }
    
    token = strtok(NULL, " ");
    if (!token) return;
    s.parking_slot = atoi(token);

    token = strtok(NULL, " ");
    if (!token) return;
    strncpy(s.status, token, MAX_NAME_LEN);
    
    s.start_time = convert_time_to_int(b.time);
    s.end_time = s.start_time + b.duration;
    
    int member_idx = get_index_from_member(b.client);
    if (member_idx == -1) return;

    refer_booking[s.id] = b;

    if (strcmp(s.status, "Scheduled") == 0) {
        accepted[member_idx][accepted_count[member_idx]++] = s;
    } else {
        rejected[member_idx][rejected_count[member_idx]++] = s;
    }

    // DEBUG: is the data correct?
    // printf("ID: %d\n", b.id);
    // printf("Client: %s\n", b.client);
    // printf("Type: %s\n", b.type);
    // printf("Date: %s\n", b.date);
    // printf("Time: %s\n", b.time);
    // printf("Duration: %.2f\n", b.duration);
    // printf("status: %s\n", s.status);
    // printf("Facilities (%d): ", b.facility_count);

    // for (int i = 0; i < b.facility_count; i++) {
    //     printf("%s ", b.facilities[i]);
    // }
    // printf("\n");
}


//add_booking("addParking -member_A 2025-05-10 08:00 3.0 battery cable;");
void printBookings(const char* algorithm, int pipe_fd) {
    char buffer[4096];
    char temp[4096] = "";
    int bytes_read;

    // Read lines from parent
    while ((bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strcat(temp, buffer);

        char* newline;

        while ((newline = strchr(temp, '\n')) != NULL) {
            *newline = '\0';
            parse_and_classify_line(temp);

            memmove(temp, newline + 1, strlen(newline + 1) + 1);
        }
    }

    // Accepted Bookings
    printf("\n*** Parking Booking - ACCEPTED / %s ***\n\n", algorithm);
    for (int i = 0; i < NUM_OF_MEMBER; i++) {
        printf("Member_%c has the following bookings:\n\n", 'A' + i);

        if (accepted_count[i] == 0) {
            printf("No accepted bookings.\n\n");
            continue;
        }
        
        printf("%-12s%-8s%-8s%-16s%-10s\n", "Date", "Start", "End", "Type", "Device");
        printf("==========================================================\n");
        for (int j = 0; j < accepted_count[i]; j++) {
            Schedule s = accepted[i][j];
            Booking b = refer_booking[s.id];

            printf("%-12s%-8d%-8d%-16s", b.date, s.start_time, s.end_time, b.type);
            
            if (b.facility_count > 0) {
                printf("%s\n", b.facilities[0]);
                for (int k = 1; k < b.facility_count; k++) {
                    printf("%44s%s\n", "", b.facilities[k]);
                }
            }
            else {
                printf("%s\n", "*"); // IF NO FACILTY
            }
            
        }
        printf("\n");
    }

    // Print Rejected bookings
    printf("\n*** Parking Booking - REJECTED / %s ***\n\n", algorithm);
    for (int i = 0; i < NUM_OF_MEMBER; i++) {
        if (rejected_count[i] == 0) continue;

        printf("Member_%c (%d bookings rejected):\n\n", 'A' + i, rejected_count[i]);
        printf("%-12s%-8s%-8s%-16s%-8s\n", "Date", "Start", "End", "Type", "Essentials");
        printf("==========================================================\n");

        for (int j = 0; j < rejected_count[i]; j++) {
            Schedule s = rejected[i][j];
            Booking b = refer_booking[s.id];

            printf("%-12s%-8d%-8d%-16s", b.date, s.start_time, s.end_time, b.type);

            if (b.facility_count > 0) {
                printf("%s\n", b.facilities[0]);
                for (int k = 1; k < b.facility_count; k++) {
                    printf("%44s%s\n", "", b.facilities[k]);
                }
            }
            else {
                printf("%s\n", "*"); // IF NO FACILITY
            }
        }
        printf("\n");
    }
}

// DEBUG: run dummy data thing 
// // Function to initialize the pipe
// void initialize_pipe(int pipe_fd[2]) {
//     if (pipe(pipe_fd) == -1) {
//         perror("pipe");
//         exit(EXIT_FAILURE);
//     }
// }

// // Function to fork the process
// pid_t fork_process() {
//     pid_t pid = fork();
//     if (pid == -1) {
//         perror("fork");
//         exit(EXIT_FAILURE);
//     }
//     return pid;
// }

// void handle_child_process(int pipe_fd[2], const char* algorithm) {
//     close(pipe_fd[1]); // Close unused write end
//     printBookings(algorithm, pipe_fd[0]);
//     close(pipe_fd[0]);
//     exit(0); // Exit the child process
// }

// void handle_parent_process(int pipe_fd[2], void (*schedule_function)(int)) {
//     close(pipe_fd[0]); 
//     schedule_function(pipe_fd[1]);
//     close(pipe_fd[1]);
//     wait(NULL); 
// }

// DEBUG: run dummy data thing 
// void run_scheduling(const char* algorithm, void (*schedule_function)(int)) {
//     int pipe_fd[2];
//     pid_t pid;

//     // Load dummy data
//     load_dummy_data();

//     // Initialize the pipe
//     initialize_pipe(pipe_fd);

//     // Fork the process
//     pid = fork_process();

//     if (pid == 0) {
//         handle_child_process(pipe_fd, algorithm);
//     } else {
//         handle_parent_process(pipe_fd, schedule_function);
//     }
// }

void trim_whitespace(char* str) {
    char* end;

    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
}

void processInput(FILE* input, bool isBatchFile) {
    char line[MAX_COMMAND_LENGTH];
    int lineCount = 0;

    while (1) {
        if (!isBatchFile) {
            printf("Please enter booking:\n");
        }

        // Input processing, read until white line, trim the whitespace, and skip emtpy lines
        if (fgets(line, sizeof(line), input) == NULL) {
            break; 
        }

        trim_whitespace(line);

        if (strlen(line) == 0) {
            continue;
        }

        // // Debug: is line correct??
        // printf("Trimmed line: '%s'\n", line);

        // Semicolon Logic
        if (line[strlen(line) - 1] != ';') {
            printf("Error: Command must end with a semicolon (;).\n");
            continue;
        }
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
                filename++;
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
        } else if (strncmp(line, "printBookings", 13) == 0) {
            // Handle printBookings command
            char cmd[MAX_NAME_LEN];
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
                // Child process (print the booking)
                close(pipe_fd[1]);
                printBookings(algorithm, pipe_fd[0]);
                close(pipe_fd[0]);
                exit(0);
            } else {
                // Parent process (run the scheduling algorithm)

                // Logic for manual algorithms
                if (strcmp(algorithm, "fcfs") == 0) {
                    reset_global_variables(true);
                    close(pipe_fd[0]);
                    fcfs_schedule_to_pipe(pipe_fd[1]);
                    close(pipe_fd[1]);
                    wait(NULL);
                } else if (strcmp(algorithm, "prio") == 0) {
                    reset_global_variables(true);
                    close(pipe_fd[0]);
                    priority_schedule_to_pipe(pipe_fd[1]);
                    close(pipe_fd[1]);
                    wait(NULL);
                } else if (strcmp(algorithm, "sjf") == 0) {
                    reset_global_variables(true);
                    close(pipe_fd[0]);
                    shortest_job_first_to_pipe(pipe_fd[1]);
                    close(pipe_fd[1]);
                    wait(NULL);
                }  
                // Logic for running all algorithms
                else if (strcmp(algorithm, "all") == 0) {
                    const char* algorithms[] = {"fcfs", "prio", "sjf"};
                    void (*functions[])(int) = {fcfs_schedule_to_pipe, priority_schedule_to_pipe, shortest_job_first_to_pipe};
                
                    // Reset the summary array at the start of the ALL command
                    memset(summary, 0, sizeof(summary));

                    for (int i = 0; i < 3; i++) {
                        int pipe_fd[2];
                        pid_t pid;
                
                        // Reset global variables 
                        reset_global_variables(false);
                        for (int j = 0; j < MAX_FACILITIES; j++) {
                            facility_availability[j] = 3;
                        }
                
                        // Initialize a new pipe for each algorithm
                        if (pipe(pipe_fd) == -1) {
                            perror("pipe error");
                            exit(EXIT_FAILURE);
                        }
                        pid = fork();
                        if (pid == -1) {
                            perror("fork error");
                            exit(EXIT_FAILURE);
                        }
                        if (pid == 0) {
                            // Print the booking
                            close(pipe_fd[1]); 
                            printBookings(algorithms[i], pipe_fd[0]);
                            close(pipe_fd[0]);
                            exit(0);
                        } else {
                            // Run each functions, "fcfs", "prio", "sjf"};
                            close(pipe_fd[0]); 
                            functions[i](pipe_fd[1]); 
                            close(pipe_fd[1]); 
                            wait(NULL); 
                        }
                    }
                    generate_performance_report();
                }
            }
        } else {
            // Validation logic
            CommandType cmdType = parseCommandType(line);
            if (cmdType == CMD_INVALID) {
                printf("Error: Invalid command type\n");
                continue;
            }

            if (!validateCommandFormat(line, cmdType)) {
                printf("Error: Invalid command format\n");
                continue;
            }
            processBooking(line);
        }
    }
}

int main() {
    printf("~~ Welcome to PolyU Smart Parking Management System ~~\n");

    // Start the user input
    processInput(stdin, false);

    return 0;
}

// addBatch -correct_testing.dat;
// addBatch -sane_data.dat;
// printBookings -fcfs;
//  printBookings -prio;
//  printBookings -sjf;
//  printBookings -all;