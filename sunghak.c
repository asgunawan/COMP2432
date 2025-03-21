#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_BOOKINGS 100
#define MAX_NAME_LEN 50
#define NUM_OF_MEMBER 5
#define MAX_DATE_LENGTH 20
#define MAX_FACILITIES 6
#define MAX_FACILITY_NAME_LENGTH 20

typedef struct {
    char memberName[MAX_NAME_LEN];
    char date[MAX_DATE_LENGTH];
    char time[MAX_DATE_LENGTH];
    float duration;
    char facilities[MAX_FACILITIES][MAX_FACILITY_NAME_LENGTH];
    int facilityCount;
} Booking;

typedef struct {
    int id;
    char client[MAX_NAME_LEN];
    char type[MAX_NAME_LEN];
    int parking_slot;
    char date[MAX_DATE_LENGTH];
    int start_time;
    int end_time;
    char status[MAX_NAME_LEN];
    char device[MAX_NAME_LEN];
} Schedule;

Schedule accepted[NUM_OF_MEMBER][MAX_BOOKINGS];
int accepted_count[NUM_OF_MEMBER] = {0};

Schedule rejected[NUM_OF_MEMBER][MAX_BOOKINGS];
int rejected_count[NUM_OF_MEMBER] = {0};

int get_index_from_member(char* member) {
    if (strncmp(member, "member_", 7) == 0) {
        char ch = member[7];
        if (ch >= 'A' && ch <= 'E') return ch - 'A';
    }
    return -1;
}

void parse_and_separate(const char* line) {
    Schedule s;
    if (sscanf(line, "%d %s %s %d %d %d %s", 
        &s.id, s.client, s.type, &s.parking_slot, &s.start_time, &s.end_time, s.status) != 7) {
        return;
    }

    int member_idx = get_index_from_member(s.client);
    if (member_idx == -1) return;

    if (strcmp(s.status, "Scheduled") == 0) {
        accepted[member_idx][accepted_count[member_idx]++] = s;
    } else {
        rejected[member_idx][rejected_count[member_idx]++] = s;
    }
}

void printBookings(const char* algorithm) {
    if (strcmp(algorithm, "ALL"))
    {
        summary_report();
        return;
    }
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
            printf("%-12s%-8d%-8s%-12s%-8d\n", s.date, s.start_time, s.end_time, s.type, s.device);
        }
        printf("\n");
    }

    printf("\n*** Parking Booking - REJECTED / %s ***\n\n", algorithm);
    for (int i = 0; i < NUM_OF_MEMBER; i++) {
        if (rejected_count[i] == 0) continue;

        printf("Member_%c (%d bookings rejected):\n\n", 'A' + i, rejected_count[i]);
        printf("%-12s%-8s%-8s%-20s%-8s\n", "Date", "Start", "End", "Type", "Essentials");
        printf("==========================================================\n");

        for (int j = 0; j < rejected_count[i]; j++) {
            Schedule s = rejected[i][j];
            printf("%-12s%-8d%-8s%-12s%-8d\n", s.date, s.start_time, s.end_time, s.type, s.device);
        }
        printf("\n");
    }
}
void summary_report()
{
    int total = 0, acc = 0, rej = 0;
    for (int i = 0; i < NUM_OF_MEMBER; i++) {
        acc += accepted_count[i];
        rej += rejected_count[i];
    }
    total = acc + rej;

    //for each algorithm
    printf("\n*** Parking Booking Manager - Summary Report ***\n\n");
    printf("Performance:\n\n");
    printf("  For %s", );
    printf("Total Bookings Received: %d\n", total);
    printf("Bookings Accepted: %d (%.2f%%)\n", acc, total ? (acc * 100.0 / total) : 0);
    printf("Bookings Rejected: %d (%.2f%%)\n", rej, total ? (rej * 100.0 / total) : 0);
}
int main() {
    char buffer[MAX_LINE];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        parse_and_classify(buffer);
    }
    printBookings("FCFS");
    return 0;
}
