// bytecode.h
#ifndef CYNEX_BYTECODE_H
#define CYNEX_BYTECODE_H

#include <stdint.h>
#include "value.h"

#define CYN_MAGIC   "CYNB"
#define CYN_VERSION 0x03

typedef enum {
    OP_LOAD_CONST = 0xA7,
    OP_STORE_VAR = 0x3C,
    OP_LOAD_VAR = 0xE1,
    OP_PRINT = 0x5B,
    OP_CONCAT = 0x92,
    OP_ADD = 0xD4,
    OP_SUB = 0x0F,
    OP_MUL = 0x6A,
    OP_DIV = 0xC8,
    OP_NEG = 0x17,
    OP_HALT = 0xFF
} Opcode;

typedef struct {
    uint8_t* code;
    uint32_t  code_size;
    uint32_t  code_cap;

    Value* constants;
    uint32_t  const_count;
    uint32_t  const_cap;
} BytecodeChunk;

void     chunk_init(BytecodeChunk* chunk);
void     chunk_free(BytecodeChunk* chunk);
void     chunk_write(BytecodeChunk* chunk, uint8_t byte);
uint8_t  chunk_add_const(BytecodeChunk* chunk, Value v);

int      load_cynb(const char* filename, BytecodeChunk* chunk);
int      save_cynb(const char* filename, const BytecodeChunk* chunk);
void     free_cynb(BytecodeChunk* chunk);
void     run_cynb(const BytecodeChunk* chunk);

#endif
