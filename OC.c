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
#include <math.h>

typedef struct {
    char* name;
    long start_position;
} Function;

typedef struct {
    char* name;
    char* value;
} Variable;

Function* functions = NULL;
Variable* variables = NULL;
int function_count = 0;
int variable_count = 0;
int prayer_found = 0;

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
    return NULL;
}

void add_function(const char* name, long position) {
    functions = realloc(functions, (function_count + 1) * sizeof(Function));
    functions[function_count].name = strdup(name);
    functions[function_count].start_position = position;
    function_count++;
}

void add_variable(const char* name, const char* value) {
    variables = realloc(variables, (variable_count + 1) * sizeof(Variable));
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
        case '/': return a / b;
    }
    return 0;
}

double evaluate_expression(const char* expression) {
    char* token;
    char* expr_copy = strdup(expression);
    double values[100];
    char ops[100];
    int vi = 0, oi = 0;

    token = strtok(expr_copy, " ");
    while (token != NULL) {
        if (token[0] == '(') {
            ops[oi++] = '(';
        }
        else if (token[0] == ')') {
            while (oi > 0 && ops[oi - 1] != '(') {
                double val2 = values[--vi];
                double val1 = values[--vi];
                char op = ops[--oi];
                values[vi++] = apply_op(val1, val2, op);
            }
            if (oi > 0) oi--;
        }
        else if (is_operator(token[0])) {
            while (oi > 0 && precedence(ops[oi - 1]) >= precedence(token[0])) {
                double val2 = values[--vi];
                double val1 = values[--vi];
                char op = ops[--oi];
                values[vi++] = apply_op(val1, val2, op);
            }
            ops[oi++] = token[0];
        }
        else {
            values[vi++] = atof(token);
        }
        token = strtok(NULL, " ");
    }

    while (oi > 0) {
        double val2 = values[--vi];
        double val1 = values[--vi];
        char op = ops[--oi];
        values[vi++] = apply_op(val1, val2, op);
    }

    free(expr_copy);
    return values[0];
}

void execute_function(FILE* file, const char* function_name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, function_name) == 0) {
            long current_pos = ftell(file);
            fseek(file, functions[i].start_position, SEEK_SET);
            char* line = NULL;
            size_t len = 0;
            ssize_t read;

            while ((read = getline(&line, &len, file)) != -1) {
                trim(line);
                if (strncmp(line, "chant(", 6) == 0) {
                    char* content_start = strchr(line, '"');
                    if (content_start) {
                        content_start++;
                        char* content_end = strchr(content_start, '"');
                        if (content_end) {
                            *content_end = '\0';
                            char* var_name = strchr(content_end + 1, ',');
                            if (var_name) {
                                var_name++;
                                trim(var_name);
                                char* var_end = strchr(var_name, ')');
                                if (var_end) *var_end = '\0';
                                char* value = get_variable_value(var_name);
                                if (value) {
                                    printf(content_start, value);
                                } else {
                                    printf("Variable '%s' not found.\n", var_name);
                                }
                            } else {
                                printf("%s", content_start);
                            }
                            printf("\n");
                        }
                    }
                } else if (strncmp(line, "incense", 7) == 0) {
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
                                }
                            }
                        }
                    }
                } else if (strncmp(line, "calc(", 5) == 0) {
                    char* expression_start = strchr(line, '(');
                    if (expression_start) {
                        expression_start++;
                        char* expression_end = strrchr(expression_start, ')');
                        if (expression_end) {
                            *expression_end = '\0';
                            double result = evaluate_expression(expression_start);
                            
                            // Check if "> null" follows the calc() statement
                            char* null_check = strstr(expression_end + 1, "> null");
                            if (null_check == NULL) {
                                // If "> null" is not present, print the result
                                char result_str[50];
                                snprintf(result_str, sizeof(result_str), "%f", result);
                                printf("%s\n", result_str);
                            }
                            // If "> null" is present, do nothing (suppress output)
                        }
                    }
                } else if (strcmp(line, "}") == 0) {
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
    printf("Function '%s' not found.\n", function_name);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        trim(line);
        if (strncmp(line, "Prayer:", 7) == 0) {
            prayer_found = 1;
        } else if (strstr(line, "invoke") == line) {
            char* function_name = strchr(line, ' ') + 1;
            char* paren = strchr(function_name, '(');
            if (paren) *paren = '\0';
            trim(function_name);
            add_function(function_name, ftell(file));
        } else if (strncmp(line, "incense", 7) == 0) {
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
                        }
                    }
                }
            }
        }
    }

    if (!prayer_found) {
        printf("Remember to pray to our Father and to the most holy saints in heaven\n");
    }

    fseek(file, 0, SEEK_SET);

    while ((read = getline(&line, &len, file)) != -1) {
        trim(line);
        if (strncmp(line, "call.upon", 9) == 0) {
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
