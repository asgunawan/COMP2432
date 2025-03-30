#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> // Include this for the wait function

#define INF 1000000000
#define MAX_LINE 256
#define MAX_BOOKINGS 100
#define MAX_NAME_LEN 50
#define NUM_OF_MEMBER 5
#define MAX_DATE_LEN 20
#define MAX_FACILITIES 6
#define MAX_FACILITY_NAME_LENGTH 20
#define TOTAL_PARKING_SLOTS 10

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

Schedule accepted[NUM_OF_MEMBER][MAX_BOOKINGS];
int accepted_count[NUM_OF_MEMBER] = {0};

Schedule rejected[NUM_OF_MEMBER][MAX_BOOKINGS];
int rejected_count[NUM_OF_MEMBER] = {0};

Booking refer_booking[MAX_BOOKINGS]; // Refer bookings with s.id

Booking bookings[MAX_BOOKINGS];
Schedule schedule[MAX_BOOKINGS];

int booking_count = 0;
int schedule_count = 0;

// Function prototype for convert_time_to_int
int convert_time_to_int(const char *time_str);

// Function to convert time (HH:MM) to an integer (hour)
int convert_time_to_int(const char *time_str) {
    int hour, minute;
    sscanf(time_str, "%d:%d", &hour, &minute);
    return hour;
}

void add_booking(const char *command) {
    if (booking_count >= MAX_BOOKINGS) {
        printf("Maximum booking limit reached.\n");
        return;
    }

    Booking *b = &bookings[booking_count];
    char facilities[MAX_LINE] = {0};

    // Parse the command
    if (sscanf(command, "%s -%s %s %s %f %[^\n]",
               b->type, b->client, b->date, b->time, &b->duration, facilities) < 5) {
        printf("Error: Invalid booking format.\n");
        return;
    }

    // Parse facilities
    b->facility_count = 0;
    char *facility = strtok(facilities, " ");
    while (facility && b->facility_count < MAX_FACILITIES) {
        strncpy(b->facilities[b->facility_count], facility, MAX_FACILITY_NAME_LENGTH - 1);
        b->facilities[b->facility_count][MAX_FACILITY_NAME_LENGTH - 1] = '\0';
        b->facility_count++;
        facility = strtok(NULL, " ");
    }

    booking_count++;
}

void fcfs_schedule_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; // Track end times for parking slots
    char buffer[256];

    for (int i = 0; i < booking_count; i++) {
        int assigned = 0;
        int start_time = convert_time_to_int(bookings[i].time); // Calculate start time

        for (int j = 0; j < 3; j++) {
            if (parking_slots[j] <= start_time) {
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = start_time;
                schedule[schedule_count].end_time = start_time + bookings[i].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[j] = schedule[schedule_count].end_time;

                // Write schedule to pipe
                snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                         schedule[schedule_count].id,
                         bookings[i].client,
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
            schedule[schedule_count].start_time = start_time;
            schedule[schedule_count].end_time = start_time + bookings[i].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            // Write rejected schedule to pipe
            snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                     schedule[schedule_count].id,
                     bookings[i].client,
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

// Updated dummy data to match the new format
void load_dummy_data() {
    add_booking("addParking -member_A 2025-05-10 08:00 3.0 battery cable;");
    add_booking("addParking -member_B 2025-05-10 08:00 3.0 battery cable;");
    add_booking("addReservation -member_C 2025-05-10 08:00 3.0 battery cable;");
    add_booking("addEssentials -member_D 2025-05-10 09:00 2.0 battery;");
    add_booking("addEvent -member_E 2025-05-10 11:00 3.0 umbrella valetpark;");
    add_booking("addParking -member_A 2025-05-10 12:00 2.0 battery cable;");
}

void shortest_job_first_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; // Track end times for parking slots
    char buffer[256];
    int curr_time = 0;
    int curr_scheduled = 0;
    int is_scheduled[MAX_BOOKINGS] = {0};

    // Sort bookings in increasing order of start time, and if the same, in increasing order of duration
    for (int i = 0; i < booking_count; i++) {
        for (int j = i + 1; j < booking_count; j++) {
            int start_time_i = convert_time_to_int(bookings[i].time);
            int start_time_j = convert_time_to_int(bookings[j].time);

            if (start_time_i > start_time_j || 
                (start_time_i == start_time_j && bookings[i].duration > bookings[j].duration)) {
                Booking temp = bookings[i];
                bookings[i] = bookings[j];
                bookings[j] = temp;
            }
        }
    }

    while (curr_scheduled < booking_count) {
        int selected = -1; // Selected booking to be scheduled
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

        int is_assigned = 0;
        int start_time = convert_time_to_int(bookings[selected].time);

        for (int i = 0; i < TOTAL_PARKING_SLOTS; i++) {
            if (parking_slots[i] <= start_time) {
                schedule[schedule_count].id = bookings[selected].id;
                schedule[schedule_count].parking_slot = i + 1;
                schedule[schedule_count].start_time = start_time;
                schedule[schedule_count].end_time = start_time + bookings[selected].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[i] = schedule[schedule_count].end_time;

                snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                         schedule[schedule_count].id,
                         bookings[selected].client,
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

        // When rejected
        if (!is_assigned) {
            schedule[schedule_count].id = bookings[selected].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = start_time;
            schedule[schedule_count].end_time = start_time + bookings[selected].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                     schedule[schedule_count].id,
                     bookings[selected].client,
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
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; // Track end times for parking slots
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

        for (int j = 0; j < TOTAL_PARKING_SLOTS; j++) { // Check each parking slot
            if (parking_slots[j] <= startTime) {
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = startTime;
                schedule[schedule_count].end_time = startTime + bookings[i].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[j] = schedule[schedule_count].end_time;

                // Write schedule to pipe, including facilities
                snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s ",
                         schedule[schedule_count].id,
                         bookings[i].memberName,
                         bookings[i].type,
                         schedule[schedule_count].parking_slot,
                         schedule[schedule_count].start_time,
                         schedule[schedule_count].end_time,
                         schedule[schedule_count].status);

                // Append facilities to the buffer
                for (int k = 0; k < bookings[i].facilityCount; k++) {
                    strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
                    if (k < bookings[i].facilityCount - 1) {
                        strncat(buffer, ",", sizeof(buffer) - strlen(buffer) - 1); // Add a comma between facilities
                    }
                }
                strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1); // Add a newline at the end

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

            // Write rejected schedule to pipe, including facilities
            snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s ",
                     schedule[schedule_count].id,
                     bookings[i].memberName,
                     bookings[i].type,
                     schedule[schedule_count].parking_slot,
                     schedule[schedule_count].start_time,
                     schedule[schedule_count].end_time,
                     schedule[schedule_count].status);

            // Append facilities to the buffer
            for (int k = 0; k < bookings[i].facilityCount; k++) {
                strncat(buffer, bookings[i].facilities[k], sizeof(buffer) - strlen(buffer) - 1);
                if (k < bookings[i].facilityCount - 1) {
                    strncat(buffer, ",", sizeof(buffer) - strlen(buffer) - 1); // Add a comma between facilities
                }
            }
            strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1); // Add a newline at the end

            write(pipe_fd, buffer, strlen(buffer));
            schedule_count++;
        }
    }
}

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
    if (sscanf(line, "%d %s %s %d %d %d %s",
               &s.id, b.client, b.type, &s.parking_slot, &s.start_time, &s.end_time, s.status) != 7) {
        return;
    }
    int member_idx = get_index_from_member(b.client);
    if (member_idx == -1) return;

    refer_booking[s.id] = b;

    if (strcmp(s.status, "Scheduled") == 0) {
        accepted[member_idx][accepted_count[member_idx]++] = s;
    } else {
        rejected[member_idx][rejected_count[member_idx]++] = s;
    }
}
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
    for (int i = 0; i < NUM_OF_MEMBER; i++) {
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

            printf("%-10s%-8d%-8d%-12s\n", b.date, s.start_time, s.end_time, b.type);
        }
        printf("\n");
    }

    //rejected bookings
    printf("\n*** Parking Booking - REJECTED / %s ***\n\n", algorithm);
    for (int i = 0; i < NUM_OF_MEMBER; i++) {
        if (rejected_count[i] == 0) continue;

        printf("Member_%c (%d bookings rejected):\n\n", 'A' + i, rejected_count[i]);
        printf("%-12s%-8s%-8s%-20s%-8s\n", "Date", "Start", "End", "Type", "Essentials");
        printf("==========================================================\n");

        for (int j = 0; j < rejected_count[i]; j++) {
            Schedule s = rejected[i][j];
            Booking b = refer_booking[s.id];

            printf("%-10s%-8d%-8d%-12s\n", b.date, s.start_time, s.end_time, b.type);

            //printf("%-12s%02d:00   %02d:00   %-20s\n", b.date, s.start_time, s.end_time, b.type);
        }
        printf("\n");
    }
}

// void summary_report()
// {
//     int total = 0, acc = 0, rej = 0;
//     for (int i = 0; i < NUM_OF_MEMBER; i++) {
//         acc += accepted_count[i];
//         rej += rejected_count[i];
//     }
//     total = acc + rej;

//     //for each algorithm
//     printf("\n*** Parking Booking Manager - Summary Report ***\n\n");
//     printf("Performance:\n\n");
//     printf("  For %s", );
//     printf("Total Bookings Received: %d\n", total);
//     printf("Bookings Accepted: %d (%.2f%%)\n", acc, total ? (acc * 100.0 / total) : 0);
//     printf("Bookings Rejected: %d (%.2f%%)\n", rej, total ? (rej * 100.0 / total) : 0);
// }

// Main function
int main() {
    int pipe_fd[2];
    pid_t pid;

    // Load dummy data
    load_dummy_data();

    // Create a pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process: Read from pipe and print schedule
        close(pipe_fd[1]); // Close unused write end
        printBookings("Priority", pipe_fd[0]);
        close(pipe_fd[0]);
    } else {
        // Parent process: Write schedule to pipe using priority scheduling
        close(pipe_fd[0]); // Close unused read end
        priority_schedule_to_pipe(pipe_fd[1]);
        close(pipe_fd[1]);
        wait(NULL); // Wait for child process to finish
    }

    return 0;
}