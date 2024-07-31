/*****************************************
**      OrthoC interpreter (occ)        **
**         By: Andrew Morris            **
**     _________________________        **
**    |Licensed under GNU V3 for|       **
**    | Free use, modification  |       **
**    |    and distribution     |       **
*****************************************/

#include "orthoc.h"

void report_error(const char* message, int line_number) {
    fprintf(stderr, ANSI_BG ANSI_FG "[ERROR]" ANSI_RESET " Line %d: %s\n", line_number, message);
}

void trim(char* str) {
    char* start = str;
    char* end = str + strlen(str) - 1;
    while (isspace(*start)) start++;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';
    memmove(str, start, end - start + 2);
}

char* get_variable_value(const char* var_name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, var_name) == 0) {
            return variables[i].value;
        }
    }
    report_error("Undefined variable", current_line_number);
    return NULL;
}

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

void add_variable(const char* name, const char* value) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            // Update existing variable
            free(variables[i].value);
            variables[i].value = strdup(value);
            return;
        }
    }
    
    // Add new variable
    variables = realloc(variables, (variable_count + 1) * sizeof(Variable));
    if (variables == NULL) {
        report_error("Memory allocation failed", current_line_number);
        exit(1);
    }
    variables[variable_count].name = strdup(name);
    variables[variable_count].value = strdup(value);
    variable_count++;
}

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

int is_operator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

int precedence(char op) {
    if (op == '+' || op == '-')
        return 1;
    if (op == '*' || op == '/')
        return 2;
    return 0;
}

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

char* get_user_input(const char* prompt) {
    char* input = NULL;
    size_t len = 0;
    ssize_t read;

    printf("%s", prompt);
    read = getline(&input, &len, stdin);

    if (read == -1) {
        free(input);
        return NULL;
    }

    // Remove newline character if present
    if (input[read - 1] == '\n') {
        input[read - 1] = '\0';
    }

    return input;
}

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
                } else if (strncmp(line, "repent(", 7) == 0) {
                    // Process repent statement (user input)
                    char* prompt_start = strchr(line, '"');
                    if (prompt_start) {
                        prompt_start++;
                        char* prompt_end = strchr(prompt_start, '"');
                        if (prompt_end) {
                            *prompt_end = '\0';
                            char* var_name = strchr(prompt_end + 1, ',');
                            if (var_name) {
                                var_name++;
                                trim(var_name);
                                char* var_end = strchr(var_name, ')');
                                if (var_end) *var_end = '\0';
                                
                                char* user_input = get_user_input(prompt_start);
                                if (user_input) {
                                    add_variable(var_name, user_input);
                                    free(user_input);
                                } else {
                                    report_error("Failed to read user input", current_line_number);
                                }
                            } else {
                                report_error("Invalid repent syntax, missing variable name", current_line_number);
                            }
                        } else {
                            report_error("Unterminated string in repent", current_line_number);
                        }
                    } else {
                        report_error("Invalid repent syntax", current_line_number);
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