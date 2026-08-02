// main.c
#include <stdio.h>
#include <stdlib.h>
#include "repl.h"
#include "variable.h"
#include "platform.h"
#include "CYNB/bytecode.h"

int main(void) {
    printf("Cynex v0.17.0\n\n");

    // if anyone actually downloaded the cynb example
    FILE* test_file = fopen("concat_example.cynb", "rb");
    if (test_file) {
        fclose(test_file);
        printf("Found concat_example.cynb, loading...\n");
        BytecodeChunk chunk = { 0 };
        if (load_cynb("concat_example.cynb", &chunk)) {
            run_cynb(&chunk);
            free_cynb(&chunk);
        }
        printf("\n");
    }

    printf("Starting REPL...\n");
    repl_run();

    free_all_variables();
    printf("Goodbye!\n");
    cynex_sleep(1000);
    return 0;
}
