// vm.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "CYNB/bytecode.h"
#include "platform.h"
#include "variable.h"
#include "value.h"

static Value stack[256];
static int   sp = 0;

static void push(Value v) {
    if (sp >= 256) {
        fprintf(stderr, "Cynex VM: stack overflow\n");
        return;
    }
    stack[sp++] = v;
}

static Value pop(void) {
    if (sp <= 0) {
        fprintf(stderr, "Cynex VM: stack underflow\n");
        return make_number(0);
    }
    return stack[--sp];
}

// Chunk helpers
void chunk_init(BytecodeChunk* chunk) {
    chunk->code = NULL;
    chunk->code_size = 0;
    chunk->code_cap = 0;
    chunk->constants = NULL;
    chunk->const_count = 0;
    chunk->const_cap = 0;
}

void chunk_free(BytecodeChunk* chunk) {
    free(chunk->code);
    for (uint32_t i = 0; i < chunk->const_count; i++) {
        free_value(&chunk->constants[i]);
    }
    free(chunk->constants);
    chunk_init(chunk);
}

void free_cynb(BytecodeChunk* chunk) {
    chunk_free(chunk);
}

void chunk_write(BytecodeChunk* chunk, uint8_t byte) {
    if (chunk->code_size + 1 > chunk->code_cap) {
        uint32_t new_cap;
        if (chunk->code_cap == 0)
            new_cap = 64;
        else if (chunk->code_cap > UINT32_MAX / 2)
            new_cap = UINT32_MAX;
        else
            new_cap = chunk->code_cap * 2;

        uint8_t* n = realloc(chunk->code, new_cap);
        if (!n) {
            fprintf(stderr, "Cynex: out of memory growing code\n");
            exit(1);
        }
        chunk->code = n;
        chunk->code_cap = new_cap;
    }
    chunk->code[chunk->code_size++] = byte;
}

uint8_t chunk_add_const(BytecodeChunk* chunk, Value v) {
    if (chunk->const_count + 1 > chunk->const_cap) {
        uint32_t new_cap;
        if (chunk->const_cap == 0)
            new_cap = 16;
        else if (chunk->const_cap > UINT32_MAX / 2)
            new_cap = UINT32_MAX;
        else
            new_cap = chunk->const_cap * 2;

        Value* n = realloc(chunk->constants, new_cap * sizeof(Value));
        if (!n) {
            fprintf(stderr, "Cynex: out of memory growing constants\n");
            exit(1);
        }
        chunk->constants = n;
        chunk->const_cap = new_cap;
    }
    chunk->constants[chunk->const_count] = v;
    return (uint8_t)chunk->const_count++;
}

// Loader
int load_cynb(const char* filename, BytecodeChunk* chunk) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("Failed to open .cynb");
        return 0;
    }

    char magic[4];
    uint8_t version, flags, const_count;
    uint32_t code_size;

    fread(magic, 1, 4, f);
    fread(&version, 1, 1, f);
    fread(&flags, 1, 1, f);
    fread(&code_size, 4, 1, f);
    fread(&const_count, 1, 1, f);

    if (memcmp(magic, CYN_MAGIC, 4) != 0 || version > CYN_VERSION) {
        fprintf(stderr, "Invalid or unsupported .cynb\n");
        fclose(f);
        return 0;
    }

    if (const_count > SIZE_MAX / sizeof(Value)) {
        fprintf(stderr, "Cynex: constant pool too large\n");
        fclose(f);
        return 0;
    }

    chunk_init(chunk);
    chunk->code = malloc(code_size);
    chunk->constants = malloc((size_t)const_count * sizeof(Value));
    if (!chunk->code || !chunk->constants) {
        fprintf(stderr, "Cynex VM: Out of memory\n");
        fclose(f);
        free(chunk->code);
        free(chunk->constants);
        return 0;
    }
    chunk->code_size = code_size;
    chunk->code_cap = code_size;
    chunk->const_count = const_count;
    chunk->const_cap = const_count;

    for (uint32_t i = 0; i < const_count; i++) {
        uint8_t type, len;
        fread(&type, 1, 1, f);
        fread(&len, 1, 1, f);

        char* data = malloc((size_t)len + 1);
        if (!data) {
            fclose(f);
            return 0;
        }
        fread(data, 1, len, f);
        data[len] = '\0';

        chunk->constants[i] = (type == 0x02) ? make_string(data) : make_number(0);
        free(data);
    }

    fread(chunk->code, 1, code_size, f);
    fclose(f);
    return 1;
}

// Saver
int save_cynb(const char* filename, const BytecodeChunk* chunk) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("Failed to create .cynb");
        return 0;
    }

    fwrite(CYN_MAGIC, 1, 4, f);

    uint8_t version = CYN_VERSION;
    uint8_t flags = 0;
    fwrite(&version, 1, 1, f);
    fwrite(&flags, 1, 1, f);

    uint32_t code_size = chunk->code_size;
    fwrite(&code_size, 4, 1, f);

    uint8_t const_count = (uint8_t)chunk->const_count;
    fwrite(&const_count, 1, 1, f);

    for (uint32_t i = 0; i < chunk->const_count; i++) {
        const Value* v = &chunk->constants[i];
        uint8_t type = (v->type == VAL_STRING) ? 0x02 : 0x01;
        fwrite(&type, 1, 1, f);

        if (v->type == VAL_STRING) {
            const char* s = v->string ? v->string : "";
            uint8_t len = (uint8_t)strlen(s);
            fwrite(&len, 1, 1, f);
            fwrite(s, 1, len, f);
        }
        else {
            uint8_t len = 0;
            fwrite(&len, 1, 1, f);
        }
    }

    fwrite(chunk->code, 1, chunk->code_size, f);
    fclose(f);
    return 1;
}

// VM
void run_cynb(const BytecodeChunk* chunk) {
    sp = 0;
    size_t pc = 0;

    while (pc < chunk->code_size) {
        uint8_t op = chunk->code[pc++];

        switch (op) {
        case OP_LOAD_CONST: {
            uint8_t idx = chunk->code[pc++];
            if (idx < chunk->const_count) {
                Value v = chunk->constants[idx];
                if (v.type == VAL_STRING)
                    push(make_string(v.string ? v.string : ""));
                else
                    push(make_number(v.number));
            }
            break;
        }

        case OP_STORE_VAR: {
            uint8_t idx = chunk->code[pc++];
            Value v = pop();
            if (idx < MAX_VARS) {
                free_value(&vars[idx].value);
                vars[idx].value = v;
                vars[idx].used = 1;
            }
            else {
                free_value(&v);
            }
            break;
        }

        case OP_LOAD_VAR: {
            uint8_t idx = chunk->code[pc++];
            if (idx < MAX_VARS && vars[idx].used) {
                Value v = vars[idx].value;
                if (v.type == VAL_STRING)
                    push(make_string(v.string ? v.string : ""));
                else
                    push(make_number(v.number));
            }
            else {
                push(make_number(0));
            }
            break;
        }

        case OP_CONCAT: {
            Value b = pop(), a = pop();
            char* sa = value_to_cstring(&a);
            char* sb = value_to_cstring(&b);
            size_t len = (sa ? strlen(sa) : 0) + (sb ? strlen(sb) : 0) + 1;
            char* res = malloc(len);
            if (res) {
                strcpy(res, sa ? sa : "");
                if (sb) strcat(res, sb);
                push(make_string(res));
                free(res);
            }
            else {
                push(make_string(""));
            }
            free(sa); free(sb);
            free_value(&a); free_value(&b);
            break;
        }

        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: {
            Value b = pop(), a = pop();
            char opch = (op == OP_ADD) ? '+' :
                (op == OP_SUB) ? '-' :
                (op == OP_MUL) ? '*' : '/';
            Value r = binary_arith(a, b, opch);
            free_value(&a); free_value(&b);
            push(r);
            break;
        }

        case OP_NEG: {
            Value v = pop();
            double n;
            if (value_to_number(&v, &n)) {
                free_value(&v);
                push(make_number(-n));
            }
            else {
                fprintf(stderr, "Unary - on non-number\n");
                free_value(&v);
                push(make_number(0));
            }
            break;
        }

        case OP_PRINT: {
            Value v = pop();
            char* s = value_to_cstring(&v);
            if (s) {
                printf("%s\n", s);
                free(s);
            }
            free_value(&v);
            break;
        }

        case OP_HALT:
            return;

        default:
            fprintf(stderr, "Cynex VM: unknown opcode 0x%02X\n", op);
            return;
        }
    }
}
