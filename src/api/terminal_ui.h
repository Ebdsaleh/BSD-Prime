/* src/api/terminal_ui.h */

#ifndef SALIX_TERMINAL_UI_H
#define SALIX_TERMINAL_UI_H

#include <stdint.h>
#include <histedit.h>

typedef struct {
    int anchor_row;             /* Absolute row of the prompt */
    int anchor_col;             /* Absolute column where the word starts */
    int columns;                /* Number of columns in the grid */
    int current_selection;      /* Index of the highlighted item */
    int match_count;            /* Total number of available suggestions */
    int rendered_rows;          /* Tracking actual rows used for cleanup */
    int draw_below;             /* 0 for above, 1 for below */
    const char** items;         /* The list of strings to display */
    char* input_snapshot;       /* Copy of the line buffer before selection */

} terminal_popup_state;

/* API Functions */

void terminal_save_state(terminal_popup_state* state, EditLine* el);
void terminal_render_popup(terminal_popup_state* state, EditLine* el);
void terminal_cleanup_popup(terminal_popup_state* state, EditLine* el);
void terminal_free_state(terminal_popup_state* state);


#endif
