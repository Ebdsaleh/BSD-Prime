/* terminal_ui.c */
#include "terminal_ui.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static void get_cursor_pos(int *row, int *col) {
    printf("\033[6n");
    fflush(stdout);
    char buf[32];
    int i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    sscanf(buf, "\033[%d;%dR", row, col);
}

void terminal_save_state(terminal_popup_state *state, EditLine *el) {
    get_cursor_pos(&state->anchor_row, &state->anchor_col);
    
    /* Calculate height requirement: Header (1) + Grid Rows */
    state->rendered_rows = (state->match_count / state->columns) + 
                           (state->match_count % state->columns != 0 ? 1 : 0);
    
    /* DECISION: Is there room above? (Need rendered_rows + 1 for header) */
    if (state->anchor_row > (state->rendered_rows + 1)) {
        state->draw_below = 0;
    } else {
        state->draw_below = 1;
    }

    const LineInfo *li = el_line(el);
    int len = li->lastchar - li->buffer;
    state->input_snapshot = malloc(len + 1);
    if (state->input_snapshot) {
        memcpy(state->input_snapshot, li->buffer, len);
        state->input_snapshot[len] = '\0';
    }
}

void terminal_render_popup(terminal_popup_state *state, EditLine *el) {
    int start_row;
    if (state->draw_below) {
        start_row = state->anchor_row + 1;
    } else {
        start_row = state->anchor_row - (state->rendered_rows + 1);
    }

    /* Jump to starting row of our Dirty Rect */
    printf("\033[%d;1H", start_row);
    printf("\033[2K" COLOR_CYAN "--- SELECTOR (Arrows/Tab/Enter) ---" COLOR_RESET "\n");

    for (int i = 0; i < state->match_count; i++) {
        if (i % state->columns == 0) printf("\033[2K"); 
        
        if (i == state->current_selection)
            printf(COLOR_MAGENTA " > [%-18s]" COLOR_RESET, state->items[i]);
        else
            printf("   %-20s", state->items[i]);
        
        if ((i + 1) % state->columns == 0) printf("\n");
    }

    /* Teleport back to the prompt anchor precisely */
    printf("\033[%d;%dH", state->anchor_row, state->anchor_col);
    fflush(stdout);
}

void terminal_cleanup_popup(terminal_popup_state *state, EditLine *el) {
    int start_row = state->draw_below ? (state->anchor_row + 1) : (state->anchor_row - (state->rendered_rows + 1));

    /* Wipe the Rect row by row */
    for (int i = 0; i <= state->rendered_rows; i++) {
        printf("\033[%d;1H\033[2K", start_row + i);
    }

    /* Restore cursor to the prompt */
    printf("\033[%d;%dH", state->anchor_row, state->anchor_col);
    fflush(stdout);
}
