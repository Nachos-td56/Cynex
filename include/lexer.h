// lexer.h
#ifndef CYNEX_LEXER_H
#define CYNEX_LEXER_H

#include <stddef.h>  // size_t

#define LINE_MAX 2048

typedef enum {
    T_EOF, T_NUMBER, T_STRING, T_IDENT,
    T_PLUS, T_MINUS, T_STAR, T_SLASH,
    T_LPAREN, T_RPAREN, T_ASSIGN, T_COMMA,
    T_CONCAT, T_DOT, T_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char text[LINE_MAX];
    double number;       // only meaningful for T_NUMBER
} Token;

typedef struct {
    const char* src;
    size_t pos;
    Token cur;
} Lexer;

void skip_ws(Lexer* lx);
void lexer_next(Lexer* lx);

#endif /* CYNEX_LEXER_H */
