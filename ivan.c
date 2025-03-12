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

// 命令类型枚举
typedef enum {
    CMD_ADD_PARKING,
    CMD_ADD_RESERVATION,
    CMD_ADD_EVENT,
    CMD_BOOK_ESSENTIALS,
    CMD_INVALID
} CommandType;

// 预约信息结构
typedef struct {
    char memberName[MAX_NAME_LENGTH];
    char date[MAX_DATE_LENGTH];
    char time[MAX_DATE_LENGTH];
    float duration;
    char facilities[MAX_FACILITIES][MAX_FACILITY_NAME_LENGTH];
    int facilityCount;
} Booking;

// 函数声明
bool validateMember(const char* memberName);
bool validateDateTime(const char* date, const char* time);
bool validateFacilities(char facilities[][MAX_FACILITY_NAME_LENGTH], int count);
void processBooking(const char* command);
void processInput(FILE* input, bool isBatchFile);
CommandType parseCommandType(const char* command);
bool validateCommandFormat(const char* command, CommandType cmdType);
bool validateFacilityCombination(const char* facility1, const char* facility2);

int main() {
    char command[MAX_COMMAND_LENGTH];
    printf("Welcome to Smart Parking Management System (SPMS)\n");
    printf("Enter commands (type 'exit;' to quit):\n");

    // 处理标准输入
    processInput(stdin, false);
    return 0;
}

// 统一的输入处理函数
void processInput(FILE* input, bool isBatchFile) {
    char line[MAX_COMMAND_LENGTH];
    int lineCount = 0;

    while (1) {
        // 对于标准输入，显示提示符
        if (!isBatchFile) {
            printf("> ");
        }

        // 读取输入直到分号
        // fscanf可以读取file参数，从而减少了在面对batch file文件中检查输入参数需要的额外方程代码
        if (fscanf(input, "%[^;]", line) != 1) {
            break;
        }
        fgetc(input); // 读取分号

        // 去除命令前后的空格
        char* start = line;
        while (*start == ' ') start++;
        char* end = start + strlen(start) - 1;
        while (end > start && *end == ' ') end--;
        *(end + 1) = '\0';

        // 批处理文件的行计数和显示
        if (isBatchFile) {
            lineCount++;
            printf("\nProcessing command %d: %s\n", lineCount, start);
        }

        // 检查退出命令（仅适用于标准输入）
        if (!isBatchFile && strcmp(start, "exit") == 0) {
            break;
        }

        // 检查是否是批处理命令（仅适用于标准输入）
        if (!isBatchFile && strncmp(start, "addBatch", 8) == 0) {
            char* filename = strchr(start, '-');
            if (filename) {
                filename++; // 跳过'-'符号
                FILE* batchFile = fopen(filename, "r");
                if (!batchFile) {
                    printf("Error: Cannot open batch file: %s\n", filename);
                    continue;
                }
                printf("\nProcessing batch file: %s\n", filename);
                processInput(batchFile, true);
                fclose(batchFile);
                printf("\nBatch processing completed: %d commands processed\n\n", lineCount);
            }
        } 
        else {
            // 首先验证命令格式
            CommandType cmdType = parseCommandType(start);
            if (cmdType == CMD_INVALID) {
                printf("Error: Invalid command type\n");
                continue;
            }

            // 验证命令格式
            if (!validateCommandFormat(start, cmdType)) {
                printf("Error: Invalid command format\n");
                printf("Expected format:\n");
                switch (cmdType) {
                    case CMD_ADD_PARKING:
                        printf("addParking -member_X YYYY-MM-DD HH:MM n.n [facility1 facility2];\n");
                        break;
                    case CMD_ADD_RESERVATION:
                        printf("addReservation -member_X YYYY-MM-DD HH:MM n.n facility1 facility2;\n");
                        break;
                    case CMD_ADD_EVENT:
                        printf("addEvent -member_X YYYY-MM-DD HH:MM n.n facility1 facility2;\n");
                        break;
                    case CMD_BOOK_ESSENTIALS:
                        printf("bookEssentials -member_X YYYY-MM-DD HH:MM n.n facility;\n");
                        break;
                    default:
                        break;
                }
                continue;
            }

            processBooking(start);
        }
    }
}

// 处理单个预约命令
void processBooking(const char* command) {
    Booking booking = {0};
    char cmd[MAX_NAME_LENGTH];
    
    // 解析命令类型和成员名
    if (sscanf(command, "%s -%s", cmd, booking.memberName) != 2) {
        printf("Error: Invalid command format\n");
        return;
    }

    // 验证成员名
    if (!validateMember(booking.memberName)) {
        printf("Error: Invalid member name\n");
        return;
    }

    // 解析日期、时间和持续时间
    char* ptr = strstr(command, booking.memberName) + strlen(booking.memberName);
    if (sscanf(ptr, "%s %s %f", booking.date, booking.time, &booking.duration) != 3) {
        printf("Error: Invalid date/time format\n");
        return;
    }

    // 验证日期和时间
    if (!validateDateTime(booking.date, booking.time)) {
        printf("Error: Invalid date or time\n");
        return;
    }

    // 解析设施
    booking.facilityCount = 0;
    ptr = strchr(ptr, ' '); // 跳过日期
    ptr = strchr(ptr + 1, ' '); // 跳过时间
    ptr = strchr(ptr + 1, ' '); // 跳过持续时间
    
    if (ptr) {
        char* facility;
        ptr++; // 跳过空格
        char temp[MAX_COMMAND_LENGTH];
        strcpy(temp, ptr);
        facility = strtok(temp, " ");
        
        while (facility && booking.facilityCount < MAX_FACILITIES) {
            strcpy(booking.facilities[booking.facilityCount], facility);
            booking.facilityCount++;
            facility = strtok(NULL, " ");
        }
    }

    // 验证设施
    if (!validateFacilities(booking.facilities, booking.facilityCount)) {
        printf("Error: Invalid facilities\n");
        return;
    }

    // 打印预约信息
    printf("\nBooking Details:\n");
    printf("Member: %s\n", booking.memberName);
    printf("Date: %s\n", booking.date);
    printf("Time: %s\n", booking.time);
    printf("Duration: %.1f hours\n", booking.duration);
    if (booking.facilityCount > 0) {
        printf("Facilities: ");
        for (int i = 0; i < booking.facilityCount; i++) {
            printf("%s ", booking.facilities[i]);
        }
        printf("\n");
    }
    printf("Booking request processed successfully\n\n");
}

// 验证成员名
bool validateMember(const char* memberName) {
    const char* validMembers[] = {"member_A", "member_B", "member_C", "member_D", "member_E"};
    for (int i = 0; i < MAX_MEMBERS; i++) {
        if (strcmp(memberName, validMembers[i]) == 0) {
            return true;
        }
    }
    return false;
}

// 验证日期和时间格式
bool validateDateTime(const char* date, const char* time) {
    int year, month, day, hour, minute;
    
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3) {
        return false;
    }
    
    if (sscanf(time, "%d:%d", &hour, &minute) != 2) {
        return false;
    }

    // 简单的日期时间验证
    if (year < 2024 || month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }
    
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return false;
    }

    return true;
}

// 验证设施
bool validateFacilities(char facilities[][MAX_FACILITY_NAME_LENGTH], int count) {
    const char* validFacilities[] = {"battery", "cable", "locker", "umbrella", "inflation", "valetpark"};
    
    for (int i = 0; i < count; i++) {
        bool valid = false;
        for (int j = 0; j < 6; j++) {
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

// 解析命令类型
CommandType parseCommandType(const char* command) {
    if(strncmp(command, "addParking", 10) == 0) return CMD_ADD_PARKING;
    if(strncmp(command, "addReservation", 14) == 0) return CMD_ADD_RESERVATION;
    if(strncmp(command, "addEvent", 8) == 0) return CMD_ADD_EVENT;
    if(strncmp(command, "bookEssentials", 14) == 0) return CMD_BOOK_ESSENTIALS;
    return CMD_INVALID;
}

// 验证设施组合是否有效
bool validateFacilityCombination(const char* facility1, const char* facility2) {
    if(!facility1 || !facility2) return true; // 单个设施或无设施是允许的
    
    // 检查有效的设施组合
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

// 验证完整命令格式
bool validateCommandFormat(const char* command, CommandType cmdType) {
    char cmd[MAX_COMMAND_LENGTH];
    char member[MAX_NAME_LENGTH];
    char date[MAX_DATE_LENGTH];
    char time[MAX_DATE_LENGTH];
    char duration[MAX_NAME_LENGTH];
    char facility1[MAX_NAME_LENGTH] = "";
    char facility2[MAX_NAME_LENGTH] = "";
    
    // 根据命令类型解析参数
    int parsed;
    switch(cmdType) {
        case CMD_ADD_PARKING:
        case CMD_ADD_RESERVATION:
        case CMD_ADD_EVENT:
            parsed = sscanf(command, "%s -%s %s %s %s %s %s", 
                          cmd, member, date, time, duration, facility1, facility2);
            if(parsed < 5) return false; // 至少需要命令、成员、日期、时间和持续时间
            break;
            
        case CMD_BOOK_ESSENTIALS:
            parsed = sscanf(command, "%s -%s %s %s %s %s", 
                          cmd, member, date, time, duration, facility1);
            if(parsed != 6) return false; // 需要精确的参数数量
            break;
            
        default:
            return false;
    }
    
    // 验证各个部分
    if(!validateDateTime(date, time)) return false;
    if(!validateMember(member)) return false;
    
    // 验证设施组合（如果有）
    if(facility1[0] && facility2[0]) {
        if(!validateFacilityCombination(facility1, facility2)) return false;
    }
    
    return true;
}
