#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_DEPTH 1000
#define MAX_ITERATIONS 10000
#define MAX_PATH_LEN 4096

#define LINES_PER_PAGE 20
#define MAX_LINE_LENGTH 1024
#define MAX_PATH_LENGTH 1024
#define MAX_KEYWORDS_LENGTH 256

#ifdef _WIN32
    #define PATH_SEPARATOR "\\"
    #include <windows.h>
#else
    #define PATH_SEPARATOR "/"
#endif

unsigned long long totalDirectories = 0;
unsigned long long processedDirectories = 0;

void displayProgressBar(int processedDirectories, int totalDirectories) {
    if (totalDirectories < 0) {
        fprintf(stderr, "Error: totalDirectories is less than 0\n");
        return;
    }

    if (processedDirectories < 0 || processedDirectories > totalDirectories) {
        fprintf(stderr, "Error: processedDirectories is out of range\n");
        return;
    }

    int barWidth = 70;
    float progress = 0;
    if (totalDirectories != 0) {
        progress = (float)processedDirectories / totalDirectories;
    }

    if (printf("[") < 0) {
        fprintf(stderr, "Error writing to the output\n");
        return;
    }

    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) {
            if (printf("=") < 0) {
                fprintf(stderr, "Error writing to the output\n");
                return;
            }
        } else if (i == pos) {
            if (printf(">") < 0) {
                fprintf(stderr, "Error writing to the output\n");
                return;
            }
        } else {
            if (printf(" ") < 0) {
                fprintf(stderr, "Error writing to the output\n");
                return;
            }
        }
    }

    if (printf("] %d %%\r", (int)(progress * 100)) < 0) {
        fprintf(stderr, "Error writing to the output\n");
        return;
    }

    if (fflush(stdout) != 0) {
        fprintf(stderr, "Failed to flush the output: %s\n", strerror(errno));
    }
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

unsigned long long getDirectorySize(const char *dirPath) {
    if (dirPath == NULL) {
        fprintf(stderr, "Error: dirPath is NULL\n");
        return 0;
    }

    unsigned long long totalSize = 0;
    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory '%s'\n", dirPath);
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip the current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path of the file
        char fullPath[1024]; // Adjust size as needed
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);

        struct stat statbuf;
        if (stat(fullPath, &statbuf) == -1) {
            fprintf(stderr, "Failed to get stats for '%s'\n", fullPath);
            continue;
        }

        // Recursively call getDirectorySize if it is a directory
        if (S_ISDIR(statbuf.st_mode)) {
            totalSize += getDirectorySize(fullPath);
        } else if (S_ISREG(statbuf.st_mode)) {
            totalSize += statbuf.st_size; // Accumulate file size
        }
    }

    closedir(dir);
    return totalSize;
}


void listDirectories(const char *basePath, FILE *outputFile, int depth, int *processedDirectories, int totalDirectories) {
    if (basePath == NULL || outputFile == NULL) {
        fprintf(stderr, "Error: basePath or outputFile is NULL\n");
        return;
    }

    if (depth > MAX_DEPTH) {
        fprintf(stderr, "Maximum recursion depth reached in directory '%s'\n", basePath);
        return;
    }

    DIR *dir = opendir(basePath);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory '%s': %s\n", basePath, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Construct full path
        char fullPath[MAX_PATH_LEN];
        snprintf(fullPath, sizeof(fullPath), "%s%s%s", basePath, PATH_SEPARATOR, entry->d_name);

        // Use stat to determine if the entry is a directory
        struct stat statbuf;
        if (stat(fullPath, &statbuf) != 0) {
            // Failed to stat, skip this entry
            continue;
        }

        // Check if it is a directory and not "." or ".."
        if (S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            unsigned long long size = getDirectorySize(fullPath);
            if (fprintf(outputFile, "%s - %llu bytes\n", fullPath, size) < 0) {
                fprintf(stderr, "Error writing to the output file\n");
                continue;
            }

            (*processedDirectories)++;
            displayProgressBar(*processedDirectories, totalDirectories);
            listDirectories(fullPath, outputFile, depth + 1, processedDirectories, totalDirectories);
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "Failed to close directory '%s': %s\n", basePath, strerror(errno));
    }
}

int countTotalDirectories(const char *basePath, int depth) {
    if (basePath == NULL) {
        fprintf(stderr, "Error: basePath is NULL\n");
        return 0;
    }

    if (depth > MAX_DEPTH) {
        fprintf(stderr, "Maximum recursion depth reached in directory '%s'\n", basePath);
        return 0;
    }

    DIR *dir = opendir(basePath);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory '%s': %s\n", basePath, strerror(errno));
        return 0;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Construct the full path for each directory entry
        char fullPath[MAX_PATH_LEN];
        snprintf(fullPath, sizeof(fullPath), "%s%s%s", basePath, PATH_SEPARATOR, entry->d_name);

        // Use stat to check if the entry is a directory
        struct stat statbuf;
        if (stat(fullPath, &statbuf) != 0) {
            // Skip this entry if we can't get stats (it might not exist, be accessible, etc.)
            continue;
        }

        // Check if it is a directory and not "." or ".."
        if (S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count += 1 + countTotalDirectories(fullPath, depth + 1); // Increment count and recurse
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "Failed to close directory '%s': %s\n", basePath, strerror(errno));
    }

    return count;
}

bool askYesNoQuestion(const char *question) {
    if (question == NULL) {
        fprintf(stderr, "Error: question is NULL\n");
        return false;
    }

    char response[10];
    printf("%s (yes/no): ", question);
    if (fgets(response, sizeof(response), stdin) == NULL) {
        fprintf(stderr, "Error reading input\n");
        return false;
    }

    response[strcspn(response, "\n")] = 0;

    for (int i = 0; response[i]; i++) {
        response[i] = tolower((unsigned char) response[i]);
    }

    return (strcmp(response, "yes") == 0);
}

unsigned long long askForSizeThreshold() {
    unsigned long long threshold = 0;
    printf("Enter size threshold in bytes: ");
    int result = scanf("%llu", &threshold);

    while (getchar() != '\n');

    if (result != 1) {
        printf("Invalid input. Using default threshold of 0 (no filtering).\n");
        threshold = 0;
    }

    return threshold;
}

void toLowerString(char *str) {
    if (str == NULL) {
        fprintf(stderr, "Error: str is NULL\n");
        return;
    }

    for (int i = 0; str[i]; i++) {
        if (isalpha((unsigned char) str[i])) {
            str[i] = tolower((unsigned char) str[i]);
        }
    }
}

void printLine(const char *path, unsigned long long size) {
    printf("%s - %llu bytes\n", path, size);
}

int exportResults(const char *inputFilePath, const char *outputFilePath, const char *searchFilter) {
    if (inputFilePath == NULL || outputFilePath == NULL || searchFilter == NULL) {
        perror("Error: inputFilePath, outputFilePath, or searchFilter is NULL");
        return -1;
    }

    FILE *inputFile = fopen(inputFilePath, "r");
    if (inputFile == NULL) {
        perror("Error opening the input file for reading");
        return -1;
    }

    FILE *outputFile = fopen(outputFilePath, "w");
    if (outputFile == NULL) {
        perror("Error opening the output file for writing");
        fclose(inputFile);
        return -1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), inputFile) != NULL) {
        unsigned long long size;
        char path[1024];

        if (sscanf(line, "%1023s - %llu", path, &size) == 2) {
            if (strstr(path, searchFilter) || strlen(searchFilter) == 0) {
                if (fprintf(outputFile, "%s\n", line) < 0) {
                    perror("Error writing to the output file");
                    fclose(inputFile);
                    fclose(outputFile);
                    return -1;
                }
            }
        }
    }

    if (fclose(inputFile) == EOF) {
        perror("Error closing the input file");
        fclose(outputFile);
        return -1;
    }

    if (fclose(outputFile) == EOF) {
        perror("Error closing the output file");
        return -1;
    }

    printf("Exported results to %s\n", outputFilePath);

    return 0;
}

void viewLastScanResults(const char *filePath) {
    if (filePath == NULL) {
        perror("Error: filePath is NULL");
        return;
    }

    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("Error: Unable to open the file for reading");
        return;
    }

    char searchFilter[256] = "";
    bool isFilteringActive = false;
    unsigned long long sizeThreshold = 0;
    bool useSizeThreshold = false;
    char line[1024];
    long lastPosition = 0;
    int iterations = 0;

    while (iterations < MAX_ITERATIONS) {
        iterations++;
        fseek(file, lastPosition, SEEK_SET);
        int lineCount = 0;

        printf("\n--- Directory Size Scanner - Last Scan Results ---\n");
        if (isFilteringActive) {
            printf("Current Filter: %s\n", searchFilter);
        }
        if (useSizeThreshold) {
            printf("Size Threshold: %llu bytes\n", sizeThreshold);
        }
        printf("-------------------------------------------------\n");

        while (lineCount < 20 && fgets(line, sizeof(line), file) != NULL) {
            unsigned long long size;
            char path[1024];

            if (sscanf(line, "%1023s - %llu", path, &size) == 2) {
                if ((!isFilteringActive || strstr(path, searchFilter)) &&
                    (!useSizeThreshold || size >= sizeThreshold)) {
                    printf("%s - %llu bytes\n", path, size);
                    lineCount++;
                }
            }
        }

        printf("-------------------------------------------------\n");
        printf("Commands: [N]ext, [S]earch, [E]xport, [F]ilter by Size, [Q]uit: ");
        char command = getchar();
        while (getchar() != '\n'); // Clear the buffer

        switch (command) {
            case 'N':
            case 'n':
                if (feof(file)) {
                    printf("\nEnd of file reached. No more data to display.\n");
                    continue;
                }
                lastPosition = ftell(file);
                break;
            case 'S':
            case 's':
                printf("Enter search filter: ");
                if (fgets(searchFilter, sizeof(searchFilter), stdin) == NULL) {
                    perror("Error reading search filter");
                    continue;
                }
                searchFilter[strcspn(searchFilter, "\n")] = 0;
                isFilteringActive = true;
                lastPosition = 0;
                break;
            case 'F':
            case 'f':
                sizeThreshold = askForSizeThreshold();
                if (sizeThreshold == -1) {
                    perror("Error getting size threshold");
                    continue;
                }
                useSizeThreshold = sizeThreshold > 0;
                lastPosition = 0;
                break;
            case 'Q':
            case 'q':
                fclose(file);
                return;
            case 'E':
            case 'e':
                {
                    char exportFilePath[256];
                    printf("Enter export file path: ");
                    if (fgets(exportFilePath, sizeof(exportFilePath), stdin) == NULL) {
                        perror("Error reading export file path");
                        continue;
                    }
                    exportFilePath[strcspn(exportFilePath, "\n")] = 0;

                    if (exportResults(filePath, exportFilePath, searchFilter) == -1) {
                        perror("Error exporting results");
                        continue;
                    }
                }
                break;
            default:
                printf("Invalid command. Please try again.\n");
                break;
        }
    }

    if (iterations == MAX_ITERATIONS) {
        printf("Maximum number of iterations reached. Exiting...\n");
    }

    fclose(file);
}

void listApps(const char *basePath, FILE *outputFile, int isRoot, int *processedDirs, int totalDirs) {
    if (basePath == NULL || outputFile == NULL || processedDirs == NULL) {
        fprintf(stderr, "Invalid arguments to listApps\n");
        return;
    }

    char path[MAX_PATH_LENGTH];
    DIR *dir = opendir(basePath);
    struct dirent *entry;

    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", basePath);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip current and parent directories
        }

        if (strstr(entry->d_name, "com.apple") != NULL) {
            continue; // Skip Apple apps
        }

        snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);

        // Determine if the entry is a directory using stat instead of d_type
        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
            fprintf(stderr, "Failed to get file status for: %s\n", path);
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Assuming depth starts from 0 and increments for each level
            listDirectories(path, outputFile, 0, processedDirs, totalDirs); // Adjust parameters as necessary
        } else {
            fprintf(outputFile, "%s\n", path);
        }

        if (*processedDirs < totalDirs) {
            (*processedDirs)++;
            if (isRoot) {
                printf("\rScanning directories... %d/%d", *processedDirs, totalDirs);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nError: processedDirs exceeds totalDirs\n");
            break;
        }
    }

    closedir(dir);
}

void scanForApps() {
    char *homeDir = getenv("HOME");
    if (homeDir == NULL) {
        perror("Error: HOME environment variable is not set");
        return;
    }

    char containersDirPath[256];
    if (snprintf(containersDirPath, sizeof(containersDirPath), "%s/Library/Containers", homeDir) < 0) {
        perror("Error formatting containers directory path");
        return;
    }

    char appSupportDirPath[256];
    if (snprintf(appSupportDirPath, sizeof(appSupportDirPath), "%s/Library/Application Support", homeDir) < 0) {
        perror("Error formatting application support directory path");
        return;
    }

    char outputFilePath[256];
    if (snprintf(outputFilePath, sizeof(outputFilePath), "apps.txt", homeDir) < 0) {
        perror("Error formatting output file path");
        return;
    }

    FILE *outputFile = fopen(outputFilePath, "w");
    if (outputFile == NULL) {
        perror("Error opening the output file for writing");
        return;
    }

    int totalDirectories = countTotalDirectories(containersDirPath, 0);
    int processedDirectories = 0; // Declare and initialize processedDirectories
    listApps(containersDirPath, outputFile, 0, &processedDirectories, totalDirectories);

    totalDirectories = countTotalDirectories(appSupportDirPath, 0);
    listApps(appSupportDirPath, outputFile, 0, &processedDirectories, totalDirectories);

    if (fclose(outputFile) == EOF) {
        perror("Error closing the output file");
        return;
    }

    printf("Scan complete. Results have been written to %s\n", outputFilePath);
}

void printMenu() {
    printf("\n+------------------------------------+\n");
    printf("| Directory Size Scanner Menu:       |\n");
    printf("+------------------------------------+\n");
    printf("| 1. Start Scan                      |\n");
    printf("| 2. Set Output File Path            |\n");
    printf("| 3. View Last Scan Results          |\n");
    printf("| 4. Search Apps                     |\n");
    printf("| 5. Scan Known Dir                  |\n");
    printf("| 00. Exit                           |\n");
    printf("+------------------------------------+\n");
}

void setTerminalTitle(const char *title) {
    if (title == NULL) {
        perror("Error: title is NULL");
        return;
    }

#ifdef _WIN32
    SetConsoleTitle(title);
#else
    printf("\033]0;%s\007", title);
    fflush(stdout);
#endif
}

int main() {
    int processedDirectories = 0;
    int choice;
    char startDir[256];
    char outputFilePath[256] = "output.txt";
    int scanResult;

    setTerminalTitle("OnionClean");

    printf("Welcome to OnionClean!\n");
    while (1) {
        printMenu();
        printf("Enter your choice: ");
        scanResult = scanf("%d", &choice);

        if (scanResult != 1) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        while (getchar() != '\n');

        switch (choice) {
            case 1:
                printf("Enter the directory you want to scan: ");
                scanResult = scanf("%255s", startDir);
                if (scanResult != 1) {
                    printf("Invalid input. Please enter a valid directory.\n");
                    break;
                }
                if (access(startDir, F_OK) == -1) {
                    printf("Directory does not exist.\n");
                    break;
                }
                printf("Starting scan...\n");
                FILE *outputFile = fopen(outputFilePath, "w");
                if (outputFile == NULL) {
                    perror("Error opening the output file");
                    break;
                }
                totalDirectories = countTotalDirectories(startDir, 0);
                listDirectories(startDir, outputFile, 0, &processedDirectories, totalDirectories);
                fclose(outputFile);
                printf("\nScan complete. Results have been written to %s\n", outputFilePath);
                break;
            case 2:
                printf("Enter new output file path: ");
                scanResult = scanf("%255s", outputFilePath);
                if (scanResult != 1) {
                    printf("Invalid input. Please enter a valid file path.\n");
                    break;
                }
                if (access(outputFilePath, W_OK) == -1) {
                    printf("File path is not writable.\n");
                    break;
                }
                printf("Output file path set to %s\n", outputFilePath);
                break;
            case 3:
                viewLastScanResults(outputFilePath);
                break;
            case 4:
                {
                    char keywords[MAX_KEYWORDS_LENGTH];
                    printf("Enter keywords to search for: ");
                    if (fgets(keywords, sizeof(keywords), stdin) == NULL) {
                        perror("Error reading keywords");
                        break;
                    }
                    keywords[strcspn(keywords, "\n")] = 0;
                    toLowerString(keywords);

                    FILE *outputFile = fopen(outputFilePath, "r");
                    if (outputFile == NULL) {
                        perror("Error opening the output file");
                        break;
                    }

                    char line[MAX_LINE_LENGTH];
                    while (fgets(line, sizeof(line), outputFile) != NULL) {
                        char path[MAX_PATH_LENGTH];
                        unsigned long long size;

                        if (sscanf(line, "%1023s - %llu", path, &size) == 2) {
                            toLowerString(path);
                            if (strstr(path, keywords)) {
                                printLine(path, size);
                            }
                        }
                    }

                    if (fclose(outputFile) == EOF) {
                        perror("Error closing the output file");
                        break;
                    }
                }
                break;
            case 5:
                #ifdef __APPLE__
                    #include <TargetConditionals.h>
                    #if TARGET_OS_MAC
                        scanForApps();
                    #endif
                #endif
                printf("MacOS Only for temporary.\n");
                break;
            case 00:
                printf("Exiting program.\n");
                return EXIT_SUCCESS;
            default:
                printf("Invalid choice. Please enter a number between 1 and 4.\n");
        }
    }

    return EXIT_SUCCESS;
}
