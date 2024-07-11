/*****************************************
***      OrthoC interpreter (occ)      ***
***         By: Andrew Morris          ***
***     _________________________      ***
***    |Licensed under GNU V3 for|     ***
***    | Free use, modification  |     ***
***    |    and distribution     |     ***
*****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1000
#define MAX_FUNCTION_NAME 100

typedef struct {
    char name[MAX_FUNCTION_NAME];
    long start_position;
} Function;

Function functions[100];
int function_count = 0;
int prayer_found = 0;

void trim(char *str) {
    char *start = str;
    char *end = str + strlen(str) - 1;

    while (isspace(*start)) start++;
    while (end > start && isspace(*end)) end--;

    *(end + 1) = '\0';
    memmove(str, start, end - start + 2);
}

void execute_function(FILE *file, const char *function_name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, function_name) == 0) {
            long current_pos = ftell(file);
            fseek(file, functions[i].start_position, SEEK_SET);
            char line[MAX_LINE_LENGTH];
            while (fgets(line, sizeof(line), file)) {
                trim(line);
                if (strncmp(line, "faithful.chant", 14) == 0) {
                    char *message = strchr(line, '"');
                    if (message) {
                        message++;
                        char *end = strchr(message, '"');
                        if (end) *end = '\0';
                        printf("%s\n", message);
                    }
                } else if (strcmp(line, "}") == 0) {
                    fseek(file, current_pos, SEEK_SET);
                    return;
                }
            }
            fseek(file, current_pos, SEEK_SET);
            return;
        }
    }
    printf("Function '%s' not found.\n", function_name);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        trim(line);
        if (strncmp(line, "Prayer:", 7) == 0) {
            prayer_found = 1;
        } else if (strstr(line, "invoke") == line) {
            char function_name[MAX_FUNCTION_NAME];
            sscanf(line, "invoke %[^(]", function_name);
            trim(function_name);
            functions[function_count].start_position = ftell(file);
            strcpy(functions[function_count].name, function_name);
            function_count++;
        }
    }

    if (!prayer_found) {
        printf("Remember to pray to our Father and to the most holy saints in heaven\n");
    }

    // Reset file pointer to the beginning
    fseek(file, 0, SEEK_SET);

    // Second pass to execute call.upon statements
    while (fgets(line, sizeof(line), file)) {
        trim(line);
        if (strncmp(line, "call.upon", 9) == 0) {
            char function_name[MAX_FUNCTION_NAME];
            sscanf(line + 9, "%s", function_name);
            execute_function(file, function_name);
        } else if (strncmp(line, "unceasingly.pray:", 17) == 0) {
            char function_name[MAX_FUNCTION_NAME];
            sscanf(line + 17, "%s", function_name);
            while (1) {
                execute_function(file, function_name);
            }
        }
    }

    fclose(file);
    return 0;
}