// lexer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

static int isident_start(int c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int isident_body(int c) {
    return isalnum((unsigned char)c) || c == '_';
}

void skip_ws(Lexer* lx) {
    while (lx->src[lx->pos] && isspace((unsigned char)lx->src[lx->pos])) {
        lx->pos++;
    }
}

void lexer_next(Lexer* lx) {
    skip_ws(lx);

    // Skip line comments
    if (lx->src[lx->pos] == '-' && lx->src[lx->pos + 1] == '-') {
        while (lx->src[lx->pos] && lx->src[lx->pos] != '\n') {
            lx->pos++;
        }
        lexer_next(lx);          // just call it. Also recurse to get next real token
    }

    char c = lx->src[lx->pos];

    if (!c) {
        lx->cur.type = T_EOF;
        lx->cur.text[0] = '\0';
        return;
    }

    // Identifiers (and keywords like true/false/print/if/then/end/local/string/int) why did i type all this
    if (isident_start((unsigned char)c)) {
        size_t i = 0;
        while (lx->src[lx->pos] && isident_body((unsigned char)lx->src[lx->pos])) {
            if (i + 1 < sizeof(lx->cur.text)) {
                lx->cur.text[i++] = lx->src[lx->pos];
            }
            lx->pos++;
        }
        lx->cur.text[i] = '\0';
        lx->cur.type = T_IDENT;
        return;
    }

    // Numbers (including optional leading + or -)
    if (isdigit((unsigned char)c) ||
        ((c == '+' || c == '-') && isdigit((unsigned char)lx->src[lx->pos + 1]))) {
        size_t start = lx->pos;
        if (c == '+' || c == '-') lx->pos++;
        while (isdigit((unsigned char)lx->src[lx->pos]) || lx->src[lx->pos] == '.') {
            lx->pos++;
        }
        size_t len = lx->pos - start;
        if (len >= sizeof(lx->cur.text)) len = sizeof(lx->cur.text) - 1;
        memcpy(lx->cur.text, lx->src + start, len);
        lx->cur.text[len] = '\0';
        lx->cur.type = T_NUMBER;
        lx->cur.number = strtod(lx->cur.text, NULL);
        return;
    }

    // String literals "..."
    if (c == '"') {
        size_t i = 0;
        lx->pos++;  // skip opening "
        while (lx->src[lx->pos] && lx->src[lx->pos] != '"' && i + 1 < sizeof(lx->cur.text)) {
            if (lx->src[lx->pos] == '\\') {
                lx->pos++;
                char x = lx->src[lx->pos];
                if (!x) break;
                if (x == 'n')      lx->cur.text[i++] = '\n';
                else if (x == 't') lx->cur.text[i++] = '\t';
                else               lx->cur.text[i++] = x;
                lx->pos++;
            }
            else {
                lx->cur.text[i++] = lx->src[lx->pos++];
            }
        }
        lx->cur.text[i] = '\0';
        if (lx->src[lx->pos] == '"') lx->pos++;  // skip closing "
        lx->cur.type = T_STRING;
        return;
    }

    // Single-character tokens + ..
    lx->pos++;
    switch (c) {
    case '+': lx->cur.type = T_PLUS;   strcpy(lx->cur.text, "+"); break;
    case '-': lx->cur.type = T_MINUS;  strcpy(lx->cur.text, "-"); break;
    case '*': lx->cur.type = T_STAR;   strcpy(lx->cur.text, "*"); break;
    case '/': lx->cur.type = T_SLASH;  strcpy(lx->cur.text, "/"); break;
    case '(': lx->cur.type = T_LPAREN; strcpy(lx->cur.text, "("); break;
    case ')': lx->cur.type = T_RPAREN; strcpy(lx->cur.text, ")"); break;
    case '=': lx->cur.type = T_ASSIGN; strcpy(lx->cur.text, "="); break;
    case ',': lx->cur.type = T_COMMA;  strcpy(lx->cur.text, ","); break;
    case '.':
        if (lx->src[lx->pos] == '.') {
            lx->pos++;
            lx->cur.type = T_CONCAT;
            strcpy(lx->cur.text, "..");
        }
        else {
            lx->cur.type = T_DOT;
            strcpy(lx->cur.text, ".");
        }
        break;
    default:
        lx->cur.type = T_UNKNOWN;
        lx->cur.text[0] = c;
        lx->cur.text[1] = '\0';
        break;
    }
}
