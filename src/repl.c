// repl.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "repl.h"
#include "parser.h"
#include "CYNB/bytecode.h"
#include "variable.h"

#define LINE_MAX   2048
#define BUFFER_MAX 65536

static char buffer[BUFFER_MAX] = { 0 };
static int  in_block = 0;

static int is_block_starter(const char* line) {
    const char* p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    return strncmp(p, "if", 2) == 0 || strncmp(p, "while", 5) == 0;
}

static int is_block_ender(const char* line) {
    const char* p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    return strncmp(p, "end", 3) == 0;
}

static int execute_buffer(void) {
    if (buffer[0] == '\0') return 0;

    Parser p;
    parser_init(&p, buffer);

    if (p.lx.cur.type == T_IDENT && strcmp(p.lx.cur.text, "exit") == 0) {
        return 1;
    }

    BytecodeChunk chunk;
    if (compile(&p, &chunk)) {
        run_cynb(&chunk);
        chunk_free(&chunk);
    }

    return 0;
}

void repl_run(void) {
    char line[LINE_MAX];

    printf("Cynex v0.17.0 REPL  (Switched to bytecode VM)\n");
    printf("Type 'exit' to quit.\n\n");

    while (1) {
        printf(in_block ? ">> " : "> ");

        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';

        if (!in_block) {
            const char* p = line;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == '\0') continue;
        }

        if (strlen(buffer) + strlen(line) + 3 >= BUFFER_MAX) {
            fprintf(stderr, "Input too large!\n");
            buffer[0] = '\0';
            in_block = 0;
            continue;
        }

        if (buffer[0]) strcat(buffer, "\n");
        strcat(buffer, line);

        if (!in_block && is_block_starter(line)) {
            in_block = 1;
            continue;
        }

        if (is_block_ender(line)) {
            int should_exit = execute_buffer();
            buffer[0] = '\0';
            in_block = 0;
            if (should_exit) break;
            continue;
        }

        if (!in_block) {
            int should_exit = execute_buffer();
            buffer[0] = '\0';
            if (should_exit) break;
        }
    }
}
