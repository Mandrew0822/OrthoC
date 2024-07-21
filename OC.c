/*****************************************
**      OrthoC interpreter (occ)        **
**         By: Andrew Morris            **
**     _________________________        **
**    |Licensed under GNU V3 for|       **
**    | Free use, modification  |       **
**    |    and distribution     |       **
*****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

// ANSI color codes for error formatting
#define ANSI_BG "\x1b[41m"
#define ANSI_FG "\x1b[37m"
#define ANSI_RESET "\x1b[0m"
#define MAX_EXPR_ELEMENTS 100

// Structure definitions for functions and variables
typedef struct {
    char* name;
    long start_position;
} Function;

typedef struct {
    char* name;
    char* value;
} Variable;

// Global variables
Function* functions = NULL;
Variable* variables = NULL;
int function_count = 0;
int variable_count = 0;
int prayer_found = 0;
int current_line_number = 0;

// Function to report errors with colored [ERROR] tag
void report_error(const char* message, int line_number) {
    
  fprintf(stderr, ANSI_BG ANSI_FG "[ERROR]" ANSI_RESET " Line %d: %s\n", line_number, message);
}

// Function to remove leading and trailing whitespace from a string
void trim(char* str) {
    char* start = str;
    char* end = str + strlen(str) - 1;
    while (isspace(*start)) start++;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';
    memmove(str, start, end - start + 2);
}

// Function to retrieve the value of a variable
char* get_variable_value(const char* var_name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, var_name) == 0) {
            return variables[i].value;
        }
    }
    report_error("Undefined variable", current_line_number);
    return NULL;
}

// Function to add a new function to the functions array
void add_function(const char* name, long position) {
    functions = realloc(functions, (function_count + 1) * sizeof(Function));
    if (functions == NULL) {
        report_error("Memory allocation failed", current_line_number);
        exit(1);
    }
    functions[function_count].name = strdup(name);
    functions[function_count].start_position = position;
    function_count++;
}

// Function to add a new variable to the variables array
void add_variable(const char* name, const char* value) {
    variables = realloc(variables, (variable_count + 1) * sizeof(Variable));
    if (variables == NULL) {
        report_error("Memory allocation failed", current_line_number);
        exit(1);
    }
    variables[variable_count].name = strdup(name);
    variables[variable_count].value = strdup(value);
    variable_count++;
}

// Function to free allocated memory
void free_memory() {
    for (int i = 0; i < function_count; i++) {
        free(functions[i].name);
    }
    free(functions);

    for (int i = 0; i < variable_count; i++) {
        free(variables[i].name);
        free(variables[i].value);
    }
    free(variables);
}

// Function to check if a character is an operator
int is_operator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

// Function to determine the precedence of operators
int precedence(char op) {
    if (op == '+' || op == '-')
        return 1;
    if (op == '*' || op == '/')
        return 2;
    return 0;
}

// Function to apply an operator to two operands
double apply_op(double a, double b, char op) {
    switch(op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0) {
                report_error("Division by zero", current_line_number);
                return NAN;
            }
            return a / b;
    }
    return 0;
}

// Function to evaluate a mathematical expression
// This function uses the Shunting Yard algorithm to parse and evaluate the expression
double evaluate_expression(const char* expression) {
    char* token;
    char* expr_copy = strdup(expression);
    double values[MAX_EXPR_ELEMENTS];
    char ops[MAX_EXPR_ELEMENTS];
    int vi = 0, oi = 0;

    token = strtok(expr_copy, " ");
    while (token != NULL) {
        // Check for expression complexity limit
        if (vi >= MAX_EXPR_ELEMENTS || oi >= MAX_EXPR_ELEMENTS) {
            report_error("Expression too complex", current_line_number);
            free(expr_copy);
            return NAN;
        }

        if (token[0] == '(') {
            ops[oi++] = '(';
        }
        else if (token[0] == ')') {
            // Evaluate all operators until the matching opening parenthesis
            while (oi > 0 && ops[oi - 1] != '(') {
                double val2 = values[--vi];
                double val1 = values[--vi];
                char op = ops[--oi];
                values[vi++] = apply_op(val1, val2, op);
            }
            if (oi > 0) oi--; // Remove the opening parenthesis
        }
        else if (is_operator(token[0])) {
            // Apply operators with higher or equal precedence
            while (oi > 0 && precedence(ops[oi - 1]) >= precedence(token[0])) {
                double val2 = values[--vi];
                double val1 = values[--vi];
                char op = ops[--oi];
                values[vi++] = apply_op(val1, val2, op);
            }
            ops[oi++] = token[0];
        }
        else {
            // Parse number and add to values stack
            char* endptr;
            double value = strtod(token, &endptr);
            if (*endptr != '\0') {
                report_error("Invalid number in expression", current_line_number);
                free(expr_copy);
                return NAN;
            }
            values[vi++] = value;
        }
        token = strtok(NULL, " ");
    }

    // Apply any remaining operators
    while (oi > 0) {
        double val2 = values[--vi];
        double val1 = values[--vi];
        char op = ops[--oi];
        values[vi++] = apply_op(val1, val2, op);
    }

    free(expr_copy);
    return values[0];
}

// Function to execute a defined function
void execute_function(FILE* file, const char* function_name) {
    int function_found = 0;
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, function_name) == 0) {
            function_found = 1;
            long current_pos = ftell(file);
            fseek(file, functions[i].start_position, SEEK_SET);
            char* line = NULL;
            size_t len = 0;
            ssize_t read;

            while ((read = getline(&line, &len, file)) != -1) {
                current_line_number++;
                if (ferror(file)) {
                    report_error("Error reading from file", current_line_number);
                    break;
                }
                trim(line);
                // Handle different types of statements
                if (strncmp(line, "chant(", 6) == 0) {
                    // Process chant statement (print)
                    char* content_start = strchr(line, '"');
                    if (content_start) {
                        content_start++;
                        char* content_end = strchr(content_start, '"');
                        if (content_end) {
                            *content_end = '\0';
                            char* var_name = strchr(content_end + 1, ',');
                            if (var_name) {
                                // Print with variable substitution
                                var_name++;
                                trim(var_name);
                                char* var_end = strchr(var_name, ')');
                                if (var_end) *var_end = '\0';
                                char* value = get_variable_value(var_name);
                                if (value) {
                                    printf(content_start, value);
                                } else {
                                    report_error("Variable not found", current_line_number);
                                }
                            } else {
                                // Print without variable substitution
                                printf("%s", content_start);
                            }
                            printf("\n");
                        } else {
                            report_error("Unterminated string in chant", current_line_number);
                        }
                    } else {
                        report_error("Invalid chant syntax", current_line_number);
                    }
                } else if (strncmp(line, "incense", 7) == 0) {
                    // Process incense statement (variable declaration)
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
                } else if (strncmp(line, "theosis(", 8) == 0) {
                    // Process theosis statement (mathematical expression)
                    char* expression_start = strchr(line, '(');
                    if (expression_start) {
                        expression_start++;
                        char* expression_end = strrchr(expression_start, ')');
                        if (expression_end) {
                            *expression_end = '\0';
                            double result = evaluate_expression(expression_start);
                            if (!isnan(result)) {
                                char* null_check = strstr(expression_end + 1, "> null");
                                if (null_check == NULL) {
                                    printf("%f\n", result);
                                }
                            }
                        } else {
                            report_error("Missing closing parenthesis in theosis", current_line_number);
                        }
                    } else {
                        report_error("Invalid theosis syntax", current_line_number);
                    }
                } else if (strcmp(line, "}") == 0) {
                    // End of function
                    free(line);
                    fseek(file, current_pos, SEEK_SET);
                    return;
                }
            }
            free(line);
            fseek(file, current_pos, SEEK_SET);
            return;
        }
    }
    if (!function_found) {
        report_error("Undefined function", current_line_number);
    }
}

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
