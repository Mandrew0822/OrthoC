/*****************************************
**      OrthoC interpreter (occ)        **
**         By: Andrew Morris            **
**     _________________________        **
**    |Licensed under GNU V3 for|       **
**    | Free use, modification  |       **
**    |    and distribution     |       **
*****************************************/

#include "orthoc.h"

Function* functions = NULL;
Variable* variables = NULL;
int function_count = 0;
int variable_count = 0;
int prayer_found = 0;
int current_line_number = 0;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, ANSI_BG ANSI_FG "[ERROR]" ANSI_RESET " Unable to open file '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    // First pass: collect function definitions and variable declarations
    while ((read = getline(&line, &len, file)) != -1) {
        current_line_number++;
        if (ferror(file)) {
            report_error("Error reading from file", current_line_number);
            break;
        }
        trim(line);
        if (strncmp(line, "Prayer:", 7) == 0) {
            prayer_found = 1;
        } else if (strstr(line, "invoke") == line) {
            // Add function definition
            char* function_name = strchr(line, ' ') + 1;
            char* paren = strchr(function_name, '(');
            if (paren) *paren = '\0';
            trim(function_name);
            add_function(function_name, ftell(file));
        } else if (strncmp(line, "incense", 7) == 0) {
            // Add variable declaration
            char* var_name = line + 7;
            char* equals = strchr(var_name, '=');
            if (equals) {
                *equals = '\0';
                trim(var_name);
                char* var_value = equals + 1;
                trim(var_value);
                if (var_value[0] == '"') {
                    var_value++;
                    char* end_quote = strrchr(var_value, '"');
                    if (end_quote) {
                        *end_quote = '\0';
                        if (*(end_quote + 1) == ';') {
                            add_variable(var_name, var_value);
                        } else {
                            report_error("Missing semicolon after variable declaration", current_line_number);
                        }
                    } else {
                        report_error("Unterminated string literal", current_line_number);
                    }
                } else {
                    report_error("Invalid variable value format", current_line_number);
                }
            } else {
                report_error("Invalid variable declaration syntax", current_line_number);
            }
        }
    }

    if (!prayer_found) {
        printf("Remember to pray to our Father and to the most holy saints in heaven\n");
    }

    // Reset file position and line number for second pass
    fseek(file, 0, SEEK_SET);
    current_line_number = 0;

    // Second pass: execute functions
    while ((read = getline(&line, &len, file)) != -1) {
        current_line_number++;
        if (ferror(file)) {
            report_error("Error reading from file", current_line_number);
            break;
        }
        trim(line);
        if (strncmp(line, "call.upon", 9) == 0) {
            // Execute function
            char* function_name = line + 9;
            trim(function_name);
            execute_function(file, function_name);
        } else if (strncmp(line, "unceasingly.pray:", 17) == 0) {
            char* function_name = line + 17;
            trim(function_name);
            while (1) {
                execute_function(file, function_name);
            }
        }
    }

    free(line);
    fclose(file);
    free_memory();
    return 0;
}