#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    int start_time;
    int duration;
    int priority;
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

Booking refer_booking[MAX_BOOKINGS]; //refer bookings with s.id

Booking bookings[MAX_BOOKINGS];
Schedule schedule[MAX_BOOKINGS];

int booking_count = 0;
int schedule_count = 0;

void add_booking(int id, const char *client, const char *type, int start_time, int duration, int priority) {
    if (booking_count < MAX_BOOKINGS) {
        bookings[booking_count].id = id;
        strncpy(bookings[booking_count].client, client, MAX_NAME_LEN - 1);
        strncpy(bookings[booking_count].type, type, MAX_NAME_LEN - 1);
        bookings[booking_count].start_time = start_time;
        bookings[booking_count].duration = duration;
        bookings[booking_count].priority = priority;
        booking_count++;
    } else {
        printf("Maximum booking limit reached.\n");
    }
}

void fcfs_schedule_to_pipe(int pipe_fd) {
    int parking_slots[TOTAL_PARKING_SLOTS] = {0}; // Track end times for 3 parking slots
    char buffer[256];

    for (int i = 0; i < booking_count; i++) {
        int assigned = 0;
        for (int j = 0; j < 3; j++) {
            if (parking_slots[j] <= bookings[i].start_time) {
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = bookings[i].start_time;
                schedule[schedule_count].end_time = bookings[i].start_time + bookings[i].duration;
                strcpy(schedule[schedule_count].status, "Scheduled");
                parking_slots[j] = schedule[schedule_count].end_time;

                // Write schedule to pipe
                snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                         schedule[schedule_count].id,
                         bookings[schedule_count].client,
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
            schedule[schedule_count].start_time = bookings[i].start_time;
            schedule[schedule_count].end_time = bookings[i].start_time + bookings[i].duration;
            strcpy(schedule[schedule_count].status, "Rejected");

            // Write rejected schedule to pipe
            snprintf(buffer, sizeof(buffer), "%d %s %s %d %d %d %s\n",
                     schedule[schedule_count].id,
                     bookings[schedule_count].client,
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
        for (int j = i+1; j < booking_count; j++) {
            if (bookings[i].start_time > bookings[j].start_time || 
                (bookings[i].start_time == bookings[j].start_time && bookings[i].duration > bookings[j].duration)) {
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
            if (is_scheduled[i]) continue;
            
            if (bookings[i].start_time <= curr_time) {
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
                if (!is_scheduled[i] && bookings[i].start_time > curr_time) {
                    if (bookings[i].start_time < next_time) {
                        next_time = bookings[i].start_time;
                    }
                }
            }
            curr_time = next_time;
            continue;
        }

        int is_assigned = 0;
        for (int i = 0; i < TOTAL_PARKING_SLOTS; i++) {
            if (parking_slots[i] <= bookings[selected].start_time) {
                schedule[schedule_count].id = bookings[selected].id;
                schedule[schedule_count].parking_slot = i + 1;
                schedule[schedule_count].start_time = bookings[selected].start_time;
                schedule[schedule_count].end_time = bookings[selected].start_time + bookings[selected].duration;
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

        //when rejected
        if (!is_assigned) {
            schedule[schedule_count].id = bookings[selected].id;
            schedule[schedule_count].parking_slot = -1;
            schedule[schedule_count].start_time = bookings[selected].start_time;
            schedule[schedule_count].end_time = bookings[selected].start_time + bookings[selected].duration;
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
        for (int j = 0; j < 3; j++) { // Check each parking slot
            if (parking_slots[j] <= bookings[i].start_time) {
                schedule[schedule_count].id = bookings[i].id;
                schedule[schedule_count].parking_slot = j + 1;
                schedule[schedule_count].start_time = bookings[i].start_time;
                schedule[schedule_count].end_time = bookings[i].start_time + bookings[i].duration;
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
            schedule[schedule_count].start_time = bookings[i].start_time;
            schedule[schedule_count].end_time = bookings[i].start_time + bookings[i].duration;
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
               &s.id, b.client, b.type, &s.parking_slot, &s.start_time, &s.end_time, s.status, b.date) != 8) {
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

        char *line = strtok(buffer, '\n');
        while (line != NULL) {
            parse_and_classify_line(line);
            line = strtok(NULL, '\n');
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

            printf("%-12s%-8d%-8s%-12s%-8d\n", b.date, s.start_time, s.end_time, b.type, s.device);
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

            printf("%-12s%02d:00   %02d:00   %-20s\n", b.date, s.start_time, s.end_time, b.type, s.device);
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

int main() {
    int pipe_fd[2];
    pid_t pid;

    // Dummy data
    add_booking(0, "member_A", "Event", 8, 3, 1);  // Ends at 11, Slot 1
    add_booking(1, "member_B", "Parking", 8, 3, 3); // Ends at 11, Slot 2
    add_booking(2, "member_C", "Reservation", 8, 3, 2); // Ends at 11, Slot 3
    add_booking(3, "member_D", "Essentials", 9, 2, 4); // Should be Rejected
    add_booking(4, "member_E", "Event", 11, 3, 1); // Ends at 14, Slot 1
    add_booking(5, "member_A", "Parking", 12, 2, 3); // Should be Rejected

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
        printBookings("Priority",pipe_fd[0]);
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
