#ifndef CYNEX_VARIABLE_H
#define CYNEX_VARIABLE_H

#include "value.h"

#define MAX_VARS 512
#define MAX_NAME 128

typedef struct {
    char name[MAX_NAME];
    Value value;
    int used;
} VarEntry;

extern VarEntry vars[MAX_VARS];   // if we keep it global for now
// (later we can hide it and only expose functions)

VarEntry* find_var(const char* name);
VarEntry* create_var(const char* name);

void free_all_variables(void);    // for cleanup at exit — optional but nice

#endif /* CYNEX_VARIABLE_H */
