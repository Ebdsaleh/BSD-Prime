/*src/api/shell_engine.c */

#include "shell_engine.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const Shell_Command command_table[];


void shell_execute(Prime_Shell* shell, char* line) {
    /* 1. CAPTURE FOR MACRO: Save the raw line before tokenization */
    if (shell->recording_active) {
        /* Don't record the 'macro' command itself */
        if (strncasecmp(line, "macro", 5) != 0 && shell->macro_line_count < 128) {
            strncpy(shell->macro_buffer[shell->macro_line_count++], line, 255);
        }
    }

    char* argv[16];
    int argc = 0;
    const char *delimiters = " \t\n\r"; /* Define once for consistency */

    char *token = strtok(line, delimiters);
    while (token != NULL && argc < 16) {
        argv[argc++] = token;
        /* Use the SAME delimiters for all tokens */
        token = strtok(NULL, delimiters); 
    }

    if (argc == 0) return;    if (argc == 0) return;

    /* Dispatch to the command table */
    for (int i = 0; command_table[i].name != NULL; i++) {
        if (strcasecmp(argv[0], command_table[i].name) == 0) {
            command_table[i].handler(shell, argc, argv);
            return;
        }
    }

    /* Custom check for 'exit' to break the run loop */
    if (strcasecmp(argv[0], "exit") == 0) {
        shell->running = 0;
        return;
    }

    printf(COLOR_RED "[-] Unknown command: %s\n" COLOR_RESET, argv[0]);
}



void shell_init(Prime_Shell* shell, uint32_t* bar0) {
    shell->bar0 = bar0;
    shell->running = 1;
    shell->alias_count = 0;
    shell->el = el_init("prime", stdin, stdout, stderr);
    shell->history = history_init();

    HistEvent ev;
    history(shell->history, &ev, H_SETSIZE, 100);
    el_set(shell->el, EL_HIST, history, shell->history);
    el_set(shell->el, EL_EDITOR, "emacs");

}


void shell_run(Prime_Shell* shell) {
    int count;
    const char* line;

    printf(COLOR_YELLOW "\n[SANDBOX] Salix Interactive Mode. Type 'exit' to return.\n" COLOR_RESET);

    while (shell->running) {
        line = el_gets(shell->el, &count);
        if (count > 0) {
            char* buffer = strdup(line);
            buffer[strcspn(buffer, "\n")] = 0; /* Strip newline */
            
            if (strlen(buffer) > 0) {
                HistEvent ev;
                history(shell->history, &ev, H_ENTER, buffer);
                shell_execute(shell, buffer);
            }
            free(buffer);
        } else {
            break;
        }
    }
}


void shell_destroy(Prime_Shell* shell) {
    if (shell->history) history_end(shell->history);
    if (shell->el) el_end(shell->el);

}
