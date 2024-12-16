#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

// Structure for storing configuration
typedef struct {
    char username[50];
    char password[50];
    char loggerFilePath[256];
    char rootDirectory[256];
} Config;

// Function to get the current timestamp in IST
void get_timestamp(char* timestamp) {
    SYSTEMTIME st;
    GetSystemTime(&st);
    TIME_ZONE_INFORMATION tz;
    GetTimeZoneInformation(&tz);

    SYSTEMTIME localTime;
    SystemTimeToTzSpecificLocalTime(&tz, &st, &localTime);

    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", localTime.wYear, localTime.wMonth, localTime.wDay,
            localTime.wHour, localTime.wMinute, localTime.wSecond);
}

// Function to log events
void log_event(const char* message, const char* loggerFilePath) {
    char timestamp[20];
    get_timestamp(timestamp);

    FILE* file = fopen(loggerFilePath, "a");
    if (file != NULL) {
        fprintf(file, "[%s] %s\n", timestamp, message);
        fclose(file);
    }
}

// Function to read or create config file
int load_config(Config* config) {
    FILE* file = fopen("config.dat", "rb");
    if (file != NULL) {
        fread(config, sizeof(Config), 1, file);
        fclose(file);
        return 1; // Config file exists
    }
    return 0; // Config file doesn't exist
}

// Function to save config file
void save_config(Config* config) {
    FILE* file = fopen("config.dat", "wb");
    if (file != NULL) {
        fwrite(config, sizeof(Config), 1, file);
        fclose(file);
    }
}

// Function to monitor file system changes
void monitor_files(const char* directory, const char* loggerFilePath) {
    HANDLE hDir = CreateFile(directory, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    if (hDir == INVALID_HANDLE_VALUE) {
        log_event("Failed to monitor directory", loggerFilePath);
        return;
    }

    char buffer[1024];
    DWORD bytesReturned;

    while (1) {
        BOOL result = ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), TRUE,
                                            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                            FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                                            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY,
                                            &bytesReturned, NULL, NULL);

        if (result) {
            FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buffer;
            char action[50];

            switch (fni->Action) {
                case FILE_ACTION_ADDED:
                    strcpy(action, "File Added");
                    break;
                case FILE_ACTION_REMOVED:
                    strcpy(action, "File Removed");
                    break;
                case FILE_ACTION_MODIFIED:
                    strcpy(action, "File Modified");
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    strcpy(action, "File Renamed (Old)");
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    strcpy(action, "File Renamed (New)");
                    break;
                default:
                    strcpy(action, "Unknown Action");
                    break;
            }

            // Convert the wide-character file name to a multibyte string
            char filePath[256];
            int len = WideCharToMultiByte(CP_UTF8, 0, fni->FileName, fni->FileNameLength / sizeof(WCHAR), filePath, sizeof(filePath), NULL, NULL);
            filePath[len] = '\0';  // Null-terminate the string

            char logMessage[512];
            sprintf(logMessage, "%s: %s %s", action, filePath, directory);
            log_event(logMessage, loggerFilePath);
        }
    }

    CloseHandle(hDir);
}

int main() {
    Config config;
    
    // Try loading existing config
    if (!load_config(&config)) {
        // Ask user for input if no config file exists
        printf("Enter username: ");
        scanf("%s", config.username);
        printf("Enter password: ");
        scanf("%s", config.password);
        printf("Enter logger file path: ");
        scanf("%s", config.loggerFilePath);
        printf("Enter root directory path: ");
        scanf("%s", config.rootDirectory);

        // Save config for next time
        save_config(&config);
    }

    // Start monitoring the specified directory
    monitor_files(config.rootDirectory, config.loggerFilePath);

    return 0;
}
