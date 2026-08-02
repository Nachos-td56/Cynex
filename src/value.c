// value.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "value.h"

// Utilities
static char* xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* r = malloc(n);
    if (!r) {
        fprintf(stderr, "Error: Out of memory. Cynex cannot continue and must close.\n");
        exit(1);
    }
    memcpy(r, s, n);
    return r;
}

// Value constructors
Value make_number(double d) {
    Value v = { VAL_NUMBER, d, NULL };
    return v;
}

Value make_string(const char* s) {
    Value v = { VAL_STRING, 0, xstrdup(s) };
    return v;
}

// Cleanup
void free_value(Value* v) {
    if (!v) return;
    if (v->type == VAL_STRING && v->string) {
        free(v->string);
    }
    v->type = VAL_NUMBER;
    v->number = 0;
    v->string = NULL;
}

// Conversions
int value_to_number(const Value* v, double* out) {
    if (v->type == VAL_NUMBER) {
        *out = v->number;
        return 1;
    }
    if (v->type == VAL_STRING && v->string && v->string[0] != '\0') {
        char* end;
        double d = strtod(v->string, &end);
        if (end != v->string) {
            *out = d;
            return 1;
        }
    }
    return 0;
}

char* value_to_cstring(const Value* v) {
    if (v->type == VAL_STRING) {
        return xstrdup(v->string ? v->string : "");
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", v->number);
    return xstrdup(buf);
}

// Arithmetic
Value binary_arith(Value a, Value b, char op) {
    double n1 = 0, n2 = 0;
    if (!value_to_number(&a, &n1) || !value_to_number(&b, &n2)) {
        fprintf(stderr, "Arithmetic on non-number\n");
        return make_number(0);
    }
    switch (op) {
    case '+': return make_number(n1 + n2);
    case '-': return make_number(n1 - n2);
    case '*': return make_number(n1 * n2);
    case '/': return make_number(n1 / n2);
    default:  return make_number(0);
    }
}

// Truthiness
int is_truthy(const Value* v) {
    if (!v) return 0;

    if (v->type == VAL_NUMBER) {
        return v->number != 0.0;
    }

    if (v->type == VAL_STRING) {
        return v->string && v->string[0] != '\0';   // empty string = false
    }

    return 1;   // future types (tables, functions...) should be truthy
}
