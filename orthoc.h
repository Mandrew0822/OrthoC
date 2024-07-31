/*****************************************
**      OrthoC interpreter (occ)        **
**         By: Andrew Morris            **
**     _________________________        **
**    |Licensed under GNU V3 for|       **
**    | Free use, modification  |       **
**    |    and distribution     |       **
*****************************************/

#ifndef ORTHOC_H
#define ORTHOC_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#define ANSI_BG "\x1b[41m"
#define ANSI_FG "\x1b[37m"
#define ANSI_RESET "\x1b[0m"
#define MAX_EXPR_ELEMENTS 100

typedef struct {
    char* name;
    long start_position;
} Function;

typedef struct {
    char* name;
    char* value;
} Variable;

extern Function* functions;
extern Variable* variables;
extern int function_count;
extern int variable_count;
extern int prayer_found;
extern int current_line_number;

void report_error(const char* message, int line_number);
void trim(char* str);
char* get_variable_value(const char* var_name);
void add_function(const char* name, long position);
void add_variable(const char* name, const char* value);
void free_memory();
int is_operator(char c);
int precedence(char op);
double apply_op(double a, double b, char op);
double evaluate_expression(const char* expression);
char* get_user_input(const char* prompt);
void execute_function(FILE* file, const char* function_name);

#endif // ORTHOC_H