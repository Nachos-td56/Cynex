// parser.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "variable.h"
#include "value.h"
#include "platform.h"

// helpers
void parser_init(Parser* p, const char* s) {
    p->lx.src = s;
    p->lx.pos = 0;
    lexer_next(&p->lx);
}

int accept(Parser* p, TokenType t) {
    if (p->lx.cur.type == t) {
        lexer_next(&p->lx);
        return 1;
    }
    return 0;
}

int expect(Parser* p, TokenType t, const char* errmsg) {
    if (p->lx.cur.type == t) {
        lexer_next(&p->lx);
        return 1;
    }
    if (errmsg) fprintf(stderr, "Parse error: %s\n", errmsg);
    return 0;
}

static int var_index(const char* name) {
    VarEntry* v = find_var(name);
    if (!v) v = create_var(name);
    if (!v) return -1;
    return (int)(v - vars);
}

// expr compilers
static void compile_expr(Parser* p, BytecodeChunk* c);

static void compile_primary(Parser* p, BytecodeChunk* c) {
    if (p->lx.cur.type == T_NUMBER) {
        uint8_t idx = chunk_add_const(c, make_number(p->lx.cur.number));
        chunk_write(c, OP_LOAD_CONST);
        chunk_write(c, idx);
        lexer_next(&p->lx);
        return;
    }

    if (p->lx.cur.type == T_STRING) {
        uint8_t idx = chunk_add_const(c, make_string(p->lx.cur.text));
        chunk_write(c, OP_LOAD_CONST);
        chunk_write(c, idx);
        lexer_next(&p->lx);
        return;
    }

    if (p->lx.cur.type == T_IDENT) {
        char name[MAX_NAME];
        strncpy(name, p->lx.cur.text, MAX_NAME - 1);
        name[MAX_NAME - 1] = '\0';
        lexer_next(&p->lx);

        if (strcmp(name, "true") == 0) {
            uint8_t idx = chunk_add_const(c, make_number(1));
            chunk_write(c, OP_LOAD_CONST);
            chunk_write(c, idx);
            return;
        }
        if (strcmp(name, "false") == 0) {
            uint8_t idx = chunk_add_const(c, make_number(0));
            chunk_write(c, OP_LOAD_CONST);
            chunk_write(c, idx);
            return;
        }

        // print works both as statement and as expression
        if (strcmp(name, "print") == 0) {
            if (!accept(p, T_LPAREN)) {
                fprintf(stderr, "Parse error: expected '(' after print\n");
                return;
            }
            int first = 1;
            while (p->lx.cur.type != T_RPAREN && p->lx.cur.type != T_EOF) {
                if (!first) accept(p, T_COMMA);
                first = 0;
                compile_expr(p, c);
                chunk_write(c, OP_PRINT);
            }
            expect(p, T_RPAREN, "expected ')' after print args");

            // leave a dummy empty string so the expression still has a value
            uint8_t empty = chunk_add_const(c, make_string(""));
            chunk_write(c, OP_LOAD_CONST);
            chunk_write(c, empty);
            return;
        }

        // normal variable
        int idx = var_index(name);
        if (idx < 0) {
            fprintf(stderr, "Undefined variable: %s\n", name);
            uint8_t z = chunk_add_const(c, make_number(0));
            chunk_write(c, OP_LOAD_CONST);
            chunk_write(c, z);
            return;
        }
        chunk_write(c, OP_LOAD_VAR);
        chunk_write(c, (uint8_t)idx);
        return;
    }

    if (accept(p, T_LPAREN)) {
        compile_expr(p, c);
        expect(p, T_RPAREN, "expected ')'");
        return;
    }

    fprintf(stderr, "Unexpected token: %s\n", p->lx.cur.text);
    lexer_next(&p->lx);
    uint8_t z = chunk_add_const(c, make_number(0));
    chunk_write(c, OP_LOAD_CONST);
    chunk_write(c, z);
}

static void compile_factor(Parser* p, BytecodeChunk* c) {
    if (accept(p, T_PLUS)) {
        compile_factor(p, c);
        return;
    }
    if (accept(p, T_MINUS)) {
        compile_factor(p, c);
        chunk_write(c, OP_NEG);
        return;
    }
    compile_primary(p, c);
}

static void compile_term(Parser* p, BytecodeChunk* c) {
    compile_factor(p, c);
    while (p->lx.cur.type == T_STAR || p->lx.cur.type == T_SLASH) {
        Opcode op = (p->lx.cur.type == T_STAR) ? OP_MUL : OP_DIV;
        lexer_next(&p->lx);
        compile_factor(p, c);
        chunk_write(c, op);
    }
}

static void compile_additive(Parser* p, BytecodeChunk* c) {
    compile_term(p, c);
    while (p->lx.cur.type == T_PLUS || p->lx.cur.type == T_MINUS) {
        Opcode op = (p->lx.cur.type == T_PLUS) ? OP_ADD : OP_SUB;
        lexer_next(&p->lx);
        compile_term(p, c);
        chunk_write(c, op);
    }
}

static void compile_expr(Parser* p, BytecodeChunk* c) {
    compile_additive(p, c);
    while (p->lx.cur.type == T_CONCAT) {
        lexer_next(&p->lx);
        compile_additive(p, c);
        chunk_write(c, OP_CONCAT);
    }
}

// statement compiler
static void compile_statement(Parser* p, BytecodeChunk* c) {
    if (p->lx.cur.type == T_EOF) return;

    // if still stubbed
    if (p->lx.cur.type == T_IDENT &&
        (strcmp(p->lx.cur.text, "if") == 0 || strcmp(p->lx.cur.text, "while") == 0)) {
        fprintf(stderr, "Control flow (if) not yet compiled to bytecode\n");
        while (p->lx.cur.type != T_EOF) {
            if (p->lx.cur.type == T_IDENT && strcmp(p->lx.cur.text, "end") == 0) {
                lexer_next(&p->lx);
                break;
            }
            lexer_next(&p->lx);
        }
        return;
    }

    // print(...)
    if (p->lx.cur.type == T_IDENT && strcmp(p->lx.cur.text, "print") == 0) {
        lexer_next(&p->lx);
        if (!accept(p, T_LPAREN)) {
            fprintf(stderr, "Parse error: expected '(' after print\n");
            return;
        }
        int first = 1;
        while (p->lx.cur.type != T_RPAREN && p->lx.cur.type != T_EOF) {
            if (!first) accept(p, T_COMMA);
            first = 0;
            compile_expr(p, c);
            chunk_write(c, OP_PRINT);
        }
        expect(p, T_RPAREN, "expected ')' after print args");
        return;
    }

    // debug_savecynbtofile( expr )
    if (p->lx.cur.type == T_IDENT && strcmp(p->lx.cur.text, "debug_savecynbtofile") == 0) {
        lexer_next(&p->lx);

        if (!accept(p, T_LPAREN)) {
            fprintf(stderr, "Parse error: expected '(' after debug_savecynbtofile\n");
            return;
        }

        BytecodeChunk tmp;
        chunk_init(&tmp);
        compile_expr(p, &tmp);
        chunk_write(&tmp, OP_HALT);

        expect(p, T_RPAREN, "expected ')' after debug_savecynbtofile arg");

        const char* filename = "debug_out.cynb";
        if (save_cynb(filename, &tmp)) {
            printf("[debug] wrote bytecode to %s  (%u bytes code, %u constants)\n",
                filename, tmp.code_size, tmp.const_count);
        }
        else {
            fprintf(stderr, "[debug] failed to write %s\n", filename);
        }

        chunk_free(&tmp);
        return;
    }

    // declarations...yay!!
    if (p->lx.cur.type == T_IDENT &&
        (strcmp(p->lx.cur.text, "local") == 0 ||
            strcmp(p->lx.cur.text, "int") == 0 ||
            strcmp(p->lx.cur.text, "float") == 0 ||
            strcmp(p->lx.cur.text, "bool") == 0 ||
            strcmp(p->lx.cur.text, "string") == 0)) {

        char decl_kw[32];
        strncpy(decl_kw, p->lx.cur.text, sizeof(decl_kw) - 1);
        decl_kw[sizeof(decl_kw) - 1] = '\0';
        lexer_next(&p->lx);

        if (p->lx.cur.type != T_IDENT) {
            fprintf(stderr, "Parse error: expected name after %s\n", decl_kw);
            return;
        }
        char name[MAX_NAME];
        strncpy(name, p->lx.cur.text, MAX_NAME - 1);
        name[MAX_NAME - 1] = '\0';
        lexer_next(&p->lx);

        int idx = var_index(name);
        if (idx < 0) return;

        if (accept(p, T_ASSIGN)) {
            compile_expr(p, c);
        }
        else if (strcmp(decl_kw, "string") == 0) {
            uint8_t ci = chunk_add_const(c, make_string(""));
            chunk_write(c, OP_LOAD_CONST);
            chunk_write(c, ci);
        }
        else {
            uint8_t ci = chunk_add_const(c, make_number(0));
            chunk_write(c, OP_LOAD_CONST);
            chunk_write(c, ci);
        }
        chunk_write(c, OP_STORE_VAR);
        chunk_write(c, (uint8_t)idx);
        return;
    }

    // assignment
    if (p->lx.cur.type == T_IDENT) {
        char name[MAX_NAME];
        strncpy(name, p->lx.cur.text, MAX_NAME - 1);
        name[MAX_NAME - 1] = '\0';

        size_t saved_pos = p->lx.pos;
        Token  saved = p->lx.cur;

        lexer_next(&p->lx);
        if (p->lx.cur.type == T_ASSIGN) {
            lexer_next(&p->lx);

            if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0) {
                fprintf(stderr, "Parse error: cannot assign to constant '%s'\n", name);
                compile_expr(p, c);
                return;
            }

            int idx = var_index(name);
            if (idx < 0) return;

            compile_expr(p, c);
            chunk_write(c, OP_STORE_VAR);
            chunk_write(c, (uint8_t)idx);
            return;
        }

        p->lx.pos = saved_pos;
        p->lx.cur = saved;
    }

    // if its a bare expr, auto print
    compile_expr(p, c);
    chunk_write(c, OP_PRINT);
}

// public entry
int compile(Parser* p, BytecodeChunk* out) {
    chunk_init(out);
    compile_statement(p, out);
    chunk_write(out, OP_HALT);
    return 1;
}
