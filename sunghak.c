#include <stdio.h>
#include "aristo.c"
#define NUM_OF_MEMBERS 5

extern Booking bookings[MAX_BOOKINGS];
extern Schedule schedule[MAX_BOOKINGS];

void printAcceptedBookings()
{
    char *algorithm = "FCFS";
    printf("*** Parking Booking - ACCEPTED / %s ***\n\n", algorithm);

    for (int i = 0; i < NUM_OF_MEMBERS; i++)
    {
        char currMember[MAX_NAME_LEN];
        sprintf(currMember, "member_%c", ('A'+i));

        printf("Member_%c has the following bookings:\n\n", (char)('A'+i));
        printf("%-10s%-10s%-10s%-10s%-10s\n", "Date", "Start", "End", "Type", "Device");
        printf("=====================================================\n");

        for (int j = 0; j < schedule_count; j++)
        {
            if (strcmp(bookings[schedule[j].id].client, currMember) == 0 && strcmp(schedule[j].status, "Accepted") == 0)
            {
                printf("%-12s%-8d%-8d%-12s%-8d\n",
                    "2025-05-10",
                    schedule[j].start_time,
                    schedule[j].end_time,
                    bookings[schedule[j].id].type,
                    schedule[j].parking_slot);
            }
        }

        printf("\n");
    }

    printf("\n-END-\n");
    printf("=====================================================\n");
}

void printRejectedBookings()
{
    char *algorithm = "FCFS";
    printf("*** Parking Booking - REJECTED / %s ***\n\n", algorithm);

    for (int i = 0; i < NUM_OF_MEMBERS; i++)
    {
        char currMember[MAX_NAME_LEN];
        sprintf(currMember, "member_%c", ('A'+i));

        /*to count number of rejected bookings first*/
        int countRejectedBookings = 0;
        for (int j = 0; j < schedule_count; j++)
        {
            if (strcmp(bookings[schedule[j].id].client, currMember) == 0 && strcmp(schedule[j].status, "Rejected") == 0)
            {
                countRejectedBookings++;
            }
        }

        printf("Member_%c (there are %d bookings rejected):\n\n", (char)('A'+i), countRejectedBookings);
        printf("%-10s%-10s%-10s%-10s%-10s\n", "Date", "Start", "End", "Type", "Essentials");
        printf("=====================================================\n");

        for (int j = 0; j < schedule_count; j++)
        {
            if (strcmp(bookings[schedule[j].id].client, currMember) == 0 && strcmp(schedule[j].status, "Rejected") == 0)
            {
                printf("%-12s%-8d%-8d%-12s%-8d\n",
                    "2025-05-10",
                    schedule[j].start_time,
                    schedule[j].end_time,
                    bookings[schedule[j].id].type,
                    schedule[j].parking_slot);
            }
        }
    }

    printf("\n-END-\n");
    printf("=====================================================\n");
}
