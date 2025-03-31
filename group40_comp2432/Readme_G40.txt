PolyU Smart Parking Management System
=====================================

Group Number: Group 40
Group Members:
1. Gunawan Aristo Sinatra - 20070491d
2. Heo Sunghak - 21097305d
3. Ma Jiawei - 23102792d

How to Compile:
---------------
We compiled and tested our program in the apollo server

To compile the program, use the following command:
1.  gcc -std=c99 ./SPMS_G40.c -o SPMS_Report_G40.txt
2.  ./SPMS_Report_G40.txt
This will now run the program

Main Functions
Enter commands interactively or use batch files:
1.  addBatch -test_data_G40.dat;

Command Format:
1.  Add Parking:
addParking -aaa YYYY-MM-DD hh:mm n.n p bbb ccc; 
Example:
addParking –member_A 2025-05-16 10:00 3.0  battery; 

2.  Add Reservation:
addReservation -aaa YYYY-MM-DD hh:mm n.n p bbb ccc; 
Example:
addReservation –member_B 2025-05-14 08:00 3.0  battery cable 

4.  Add Event:
addEvent -aaa YYYY-MM-DD hh:mm n.n bbb ccc ddd;
Example:
addEvent –member_E 2025-05-16 14:00 2.0 locker umbrella valetpark;

5.  Book Essentials:
bookEssentials -aaa YYYY-MM-DD hh:mm n.n bbb;
Example:
bookEssentials –member_C 2025-05-011 13:00 4.0 battery;

To print bookings for a specific algorithm:
1.  printBookings -fcfs;
2.  printBookings -prio;
3.  printBookings -sjf;
4.  printBookings -all;

Exit the program:
1. endProgram  