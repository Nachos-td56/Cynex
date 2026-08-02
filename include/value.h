// value.h
#ifndef CYNEX_VALUE_H
#define CYNEX_VALUE_H

#include <stddef.h>

typedef enum {
    VAL_NUMBER,
    VAL_STRING
} ValueType;

typedef struct {
    ValueType type;
    double number;
    char* string;
} Value;

// Constructors
Value make_number(double d);
Value make_string(const char* s);

// Cleanup
void free_value(Value* v);

// Conversions
int value_to_number(const Value* v, double* out);
char* value_to_cstring(const Value* v);   // caller must free

// Operations
Value binary_arith(Value a, Value b, char op);

// Truthiness (important for if/future while)
int is_truthy(const Value* v);

#endif /* CYNEX_VALUE_H */
