// variable.c
#include <stdio.h>
#include <string.h>
#include "variable.h"
#include "value.h"

VarEntry vars[MAX_VARS] = { 0 };   // global array, zero-initialized

VarEntry* find_var(const char* name) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (vars[i].used && strcmp(vars[i].name, name) == 0) {
            return &vars[i];
        }
    }
    return NULL;
}

VarEntry* create_var(const char* name) {
    VarEntry* v = find_var(name);
    if (v) return v;   // already exists, reuse

    for (int i = 0; i < MAX_VARS; i++) {
        if (!vars[i].used) {
            vars[i].used = 1;
            strncpy(vars[i].name, name, MAX_NAME - 1);
            vars[i].name[MAX_NAME - 1] = '\0';
            vars[i].value = make_number(0);
            return &vars[i];
        }
    }

    fprintf(stderr, "Variable table full\n");
    return NULL;
}

void free_all_variables(void) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (vars[i].used) {
            free_value(&vars[i].value);
            vars[i].used = 0;
            vars[i].name[0] = '\0';
        }
    }
}
