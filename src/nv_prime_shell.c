/* src/nv_prime_shell.c */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>     /* For isspace, isprint */
#include <strings.h>   /* For strncasecmp */
#include <unistd.h>    /* For read, STDIN_FILENO */
#include "api/prime.h"
#include "api/colors.h"
#include "api/shell_engine.h"
#include "api/terminal_ui.h"

extern const Shell_Command command_table[]; /* The Registry from commands.c */
extern RegisterMap dictionary[];             /* The Hardware map */


/* IntelliSense forward declaration */
unsigned char complete_context(EditLine *el, int ch) {
    const LineInfo *li = el_line(el);
    const char *word_start = li->cursor;
    while (word_start > li->buffer && !isspace(*(word_start - 1))) {
        word_start--;
    }
    int prefix_len = li->cursor - word_start;

    const char *matches[256];
    int match_count = 0;

    for (int i = 0; command_table[i].name != NULL && match_count < 64; i++) {
        if (strncasecmp(command_table[i].name, word_start, prefix_len) == 0) {
            matches[match_count++] = command_table[i].name;
        }
    }
    for (int i = 0; dictionary[i].name != NULL && match_count < 256; i++) {
        if (strncasecmp(dictionary[i].name, word_start, prefix_len) == 0) {
            matches[match_count++] = dictionary[i].name;
        }
    }
    
    /* EARLY EXIT: No matches found */
    if (match_count == 0) return CC_ERROR;

    /* UNIQUE MATCH: Instant fulfillment */
    if (match_count == 1) {
        el_deletestr(el, prefix_len);
        el_insertstr(el, matches[0]);
        el_insertstr(el, " ");
        return CC_REFRESH;
    }

    terminal_popup_state popup = {
        .columns = 3, .match_count = match_count, .items = matches
    };
    terminal_save_state(&popup, el);

    while (1) {
        terminal_render_popup(&popup, el);
        char key;
        if (read(STDIN_FILENO, &key, 1) <= 0) break;

        if (key == '\t' || key == '\n' || key == '\r') {
            el_deletestr(el, prefix_len);
            el_insertstr(el, matches[popup.current_selection]);
            if (key == '\t') el_insertstr(el, " ");
            break;
        }
        if (key == 27) {
            char buf[2];
            if (read(STDIN_FILENO, buf, 2) == 2 && buf[0] == '[') {
                if (buf[1] == 'A') popup.current_selection = (popup.current_selection - 3 + match_count) % match_count;
                else if (buf[1] == 'B') popup.current_selection = (popup.current_selection + 3) % match_count;
                else if (buf[1] == 'C') popup.current_selection = (popup.current_selection + 1) % match_count;
                else if (buf[1] == 'D') popup.current_selection = (popup.current_selection - 1 + match_count) % match_count;
                continue;
            }
            break;
        }
        if (isprint(key)) {
            char k_str[2] = {key, '\0'};
            el_insertstr(el, k_str);
            break;
        }
    }
    terminal_cleanup_popup(&popup, el);
    return CC_REFRESH;
}

/* --- Prompt Definition --- */
char* prompt_generator(EditLine *el) {
    return "\1" COLOR_MAGENTA "\1prime> \1" COLOR_RESET "\1";
}


/* --- Entry Point --- */
void print_menu() {
    printf(COLOR_BLUE "\n--- BSD-Prime Interactive Debugger ---\n" COLOR_RESET);
    printf(COLOR_YELLOW  "1) WAKE (Ignite PMC 0x200)\n" COLOR_RESET);
    printf(COLOR_GREEN   "2) UNLOCK (Force Bridge 0x210)\n" COLOR_RESET);
    printf(COLOR_WHITE   "3) SCRUB (Deep Reset Sequence)\n" COLOR_RESET);
    printf(COLOR_MAGENTA "4) SANDBOX (Interactive Shell)\n" COLOR_RESET);
    printf(COLOR_BLUE    "0) EXIT\n" COLOR_RESET);
    printf("Select > ");
}

int main() {
    uint32_t *gpu_bar0 = (uint32_t *)map_physical_memory(0xAD000000, 0x200000);
    if (!gpu_bar0) return 1;

    Prime_Shell shell;
    shell_init(&shell, gpu_bar0);

    el_set(shell.el, EL_PROMPT_ESC, prompt_generator, '\1');
    el_set(shell.el, EL_ADDFN, "intelli", "IntelliSense", complete_context);
    el_set(shell.el, EL_BIND, "\t", "intelli", NULL);

    shell_run(&shell);
    shell_destroy(&shell);
    return 0;
}
