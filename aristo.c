#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void add_booking(int id, char* client, char* type, int start_time, int duration, int priority) {
    if (booking_count < MAX_BOOKINGS) {
        bookings[booking_count].id = id;
        strcpy(bookings[booking_count].client, client);
        strcpy(bookings[booking_count].type, type);
        bookings[booking_count].start_time = start_time;
        bookings[booking_count].duration = duration;
        bookings[booking_count].priority = priority;
        booking_count++;
    } else {
        printf("Maximum booking limit reached.\n");
    }
}

void fcfs_schedule() {
    int parking_slots[3] = {0, 0, 0}; // Track end times for 3 parking slots
    
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
            schedule_count++;
        }
    }
}

void print_schedule() {
    printf("ID\tClient\t\tType\t\tSlot\tStart\tEnd\tStatus\n");
    printf("--------------------------------------------------------------\n");
    for (int i = 0; i < schedule_count; i++) {
        printf("%d\t%-16s%-16s%d\t%d\t%d\t%s\n",
               schedule[i].id,
               bookings[schedule[i].id].client,
               bookings[schedule[i].id].type,
               schedule[i].parking_slot,
               schedule[i].start_time,
               schedule[i].end_time,
               schedule[i].status);
    }
}

int main() {
    int pipefd[2];
    pid_t pid;

    // Dummy data
    add_booking(0, "member_A", "Event", 8, 2, 1);
    add_booking(1, "member_B", "Parking", 8, 1, 3);
    add_booking(2, "member_C", "Reservation", 9, 1, 2);
    add_booking(3, "member_D", "Essentials", 10, 2, 4);
    add_booking(4, "member_E", "Event", 11, 3, 1);
    add_booking(5, "member_A", "Parking", 13, 1, 3);

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        close(pipefd[1]); // Close write end
        read(pipefd[0], bookings, sizeof(bookings));
        read(pipefd[0], &booking_count, sizeof(booking_count));
        close(pipefd[0]);

        fcfs_schedule();
        print_schedule();
    } else { // Parent process
        close(pipefd[0]); // Close read end
        write(pipefd[1], bookings, sizeof(bookings));
        write(pipefd[1], &booking_count, sizeof(booking_count));
        close(pipefd[1]);

        wait(NULL); // Wait for child process to finish
    }

    return 0;
}