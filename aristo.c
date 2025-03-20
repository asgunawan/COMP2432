#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_BOOKINGS 20
#define MAX_NAME_LEN 50

typedef struct {
    int id;
    char client[MAX_NAME_LEN];
    char type[MAX_NAME_LEN];
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
    int parking_slots[3] = {0, 0, 0}; // Track end times for 3 parking slots
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

// Event (Highest priority)
// Reservation
// Parking
// Essentials (Lowest priority)
void priority_schedule_to_pipe(int pipe_fd) {
    int parking_slots[3] = {0, 0, 0}; // Track end times for 3 parking slots
    char buffer[256];

    // Helper function to determine priority
    int get_priority(const char *type) {
        if (strcmp(type, "Event") == 0) return 1;
        if (strcmp(type, "Reservation") == 0) return 2;
        if (strcmp(type, "Parking") == 0) return 3;
        if (strcmp(type, "Essentials") == 0) return 4;
        return 5; // Default priority for unknown types
    }

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

void print_schedule_from_pipe(int pipe_fd) {
    char buffer[256];
    char line[256];
    int bytes_read;
    int line_start = 0;

    printf("ID      Client          Type            Slot    Start   End     Status\n");
    printf("--------------------------------------------------------------\n");

    // Read from pipe and process each line
    while ((bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string for safety

        // Process each line in the buffer
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                // Extract a single line
                strncpy(line, buffer + line_start, i - line_start);
                line[i - line_start] = '\0'; // Null-terminate the line

                // Variables to hold parsed data
                int id, slot, start, end;
                char client[MAX_NAME_LEN], type[MAX_NAME_LEN], status[MAX_NAME_LEN];

                // Parse the line into individual fields
                sscanf(line, "%d %s %s %d %d %d %s", &id, client, type, &slot, &start, &end, status);

                // Print the parsed data with proper formatting
                printf("%-8d%-15s%-15s%-8d%-8d%-8d%-10s\n", id, client, type, slot, start, end, status);

                // Update the start of the next line
                line_start = i + 1;
            }
        }

        // Handle any remaining data in the buffer
        line_start = 0;
    }
}

//this is for FCFS
// int main() {
//     int pipe_fd[2];
//     pid_t pid;

//     // Dummy data
//     add_booking(0, "member_A", "Event", 8, 2, 1);
//     add_booking(1, "member_B", "Parking", 8, 1, 3);
//     add_booking(2, "member_C", "Reservation", 9, 1, 2);
//     add_booking(3, "member_D", "Essentials", 10, 2, 4);
//     add_booking(4, "member_E", "Event", 11, 3, 1);
//     add_booking(5, "member_A", "Parking", 13, 1, 3);

//     // Create a pipe
//     if (pipe(pipe_fd) == -1) {
//         perror("pipe");
//         exit(EXIT_FAILURE);
//     }

//     // Fork a child process
//     pid = fork();
//     if (pid == -1) {
//         perror("fork");
//         exit(EXIT_FAILURE);
//     }

//     if (pid == 0) {
//         // Child process: Read from pipe and print schedule
//         close(pipe_fd[1]); // Close unused write end
//         print_schedule_from_pipe(pipe_fd[0]);
//         close(pipe_fd[0]);
//     } else {
//         // Parent process: Write schedule to pipe
//         close(pipe_fd[0]); // Close unused read end
//         fcfs_schedule_to_pipe(pipe_fd[1]);
//         close(pipe_fd[1]);
//         wait(NULL); // Wait for child process to finish
//     }

//     return 0;
// }

// this is for priority
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
        print_schedule_from_pipe(pipe_fd[0]);
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

// Next: re-order the ID from 0 to 5 instead of it just being randomly cluttered

/*
How to Run the Code:

1. Open WSL (Windows Subsystem for Linux) or a Linux terminal.
   - If you're using WSL, search for "WSL" or "Ubuntu" in the Start menu and open it.

2. Navigate to the directory where the code is located.
   - Use the `cd` command to change directories. For example:
     cd /mnt/c/Users/User/Documents/GitHub/COMP2432

3. Compile the code using the GCC compiler.
   - Run the following command to compile the code:
     gcc aristo.c -o aristo

4. Run the compiled program.
   - Use the following command to execute the program:
     ./aristo

5. View the output.
   - The program will display the schedule in the terminal, showing which bookings are scheduled and which are rejected.

6. (Optional) Modify the dummy data.
   - You can edit the `add_booking` calls in the `main` function to test different scenarios.
   - After making changes, recompile the code using step 3 and run it again using step 4.

7. (Optional) Debugging.
   - If you encounter any issues, ensure that GCC is installed by running:
     gcc --version
   - If GCC is not installed, install it using your Linux distribution's package manager (e.g., `sudo apt install gcc` for Ubuntu).
*/