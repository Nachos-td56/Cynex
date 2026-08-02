// parser.h
#ifndef CYNEX_PARSER_H
#define CYNEX_PARSER_H

#include "lexer.h"
#include "CYNB/bytecode.h"

typedef struct {
    Lexer lx;
} Parser;

void parser_init(Parser* p, const char* s);

// Compile the current source into a BytecodeChunk (caller owns it)
int  compile(Parser* p, BytecodeChunk* out);

// Helpers still used internally
int accept(Parser* p, TokenType t);
int expect(Parser* p, TokenType t, const char* errmsg);

#endif
