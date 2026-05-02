/* src/nv_prime_shell.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>      /* For usleep */
#include <histedit.h>    /* BSD-native line editing */
#include <poll.h>      /* For non-blocking input check */
#include <termios.h>   /* For terminal mode switching */
#include "api/prime.h"
#include "api/colors.h"
#include "api/terminal_ui.h"
#include "dev/turing_gsp.h"

/* To compile: cc -I. nv_prime_shell.c api/prime.c -o prime_shell -ledit -lcurses */

typedef struct {
    const char *name;
    uint32_t offset;
} RegisterMap;

#include "dev/turing_registers.h"

/* --- Utility Functions --- */

uint32_t resolve_name(const char *name) {
    for (int i = 0; dictionary[i].name != NULL; i++) {
        if (strcasecmp(name, dictionary[i].name) == 0) {
            return dictionary[i].offset;
        }
    }
    return 0xFFFFFFFF;
}


/* Standard Longest Common Prefix (LCP) helper */
int find_lcp(const char **matches, int count) {
    if (count == 0) return 0;
    int len = 0;
    while (1) {
        char c = matches[0][len];
        if (c == '\0') return len;
        for (int i = 1; i < count; i++) {
            if (matches[i][len] != c) return len;
        }
        len++;
    }
}



void dump_log(uint32_t *bar0) {
    FILE *f = fopen("prime_silicon.log", "a");
    if (!f) {
        printf(COLOR_RED "[-] Error: Could not open log file.\n" COLOR_RESET);
        return;
    }
    time_t now = time(NULL);
    fprintf(f, "\n--- SILICON DUMP: %s", ctime(&now));
    for (int i = 0; dictionary[i].name != NULL; i++) {
        uint32_t val = bar0[dictionary[i].offset / 4];
        fprintf(f, "[%08x] %-15s : 0x%08x\n",
                dictionary[i].offset, dictionary[i].name, val);
    }
    fprintf(f, "--- END DUMP ---\n");
    fclose(f);
    printf(COLOR_GREEN "[+] Register state synced to prime_silicon.log\n" COLOR_RESET);
}

char* prompt_generator(EditLine *el) {
    /* Use \1 as a literal marker to tell libedit to ignore these bytes for length calcs */
    return "\1" COLOR_MAGENTA "\1prime> \1" COLOR_RESET "\1";
}



/* --- IntelliSense Completion Logic --- */
unsigned char complete_context(EditLine *el, int ch) {
    /* 1. STATE ANALYSIS: Identify the current word and prefix */
    const LineInfo *li = el_line(el);
    const char *word_start = li->cursor;
    while (word_start > li->buffer && !isspace(*(word_start - 1))) {
        word_start--;
    }
    int prefix_len = li->cursor - word_start;
    int is_first_word = (word_start == li->buffer);

    const char *matches[256];
    int match_count = 0;

    /* 2. GATHER MATCHES: Search Command List and Hardware Dictionary */
    for (int i = 0; command_list[i] != NULL && match_count < 64; i++) {
        if (strncasecmp(command_list[i], word_start, prefix_len) == 0) {
            matches[match_count++] = command_list[i];
        }
    }
    for (int i = 0; dictionary[i].name != NULL && match_count < 256; i++) {
        if (strncasecmp(dictionary[i].name, word_start, prefix_len) == 0) {
            matches[match_count++] = dictionary[i].name;
        }
    }

    /* 3. EARLY EXITS */
    if (match_count == 0) return CC_ERROR;

    /* UNIQUE MATCH: If only one exists (e.g., 'pe' -> 'peek'), fill it and exit */
    if (match_count == 1) {
        el_deletestr(el, prefix_len);
        el_insertstr(el, matches[0]);
        el_insertstr(el, " ");
        return CC_REFRESH;
    }

    /* 4. MODAL SELECTOR: Prepare the Coordinate-Aware Layer */
    terminal_popup_state popup = {
        .columns = 3, 
        .match_count = match_count, 
        .items = matches,
        .current_selection = 0,
        .rendered_rows = 0,
        .input_snapshot = NULL
    };
    
    /* Synchronize with absolute terminal coordinates (DSR) */
    terminal_save_state(&popup, el);

    while (1) {
        /* Render frame (Space-Aware: Above or Below prompt) */
        terminal_render_popup(&popup, el);

        char key;
        /* Low-level read to bypass stdio buffering issues */
        if (read(STDIN_FILENO, &key, 1) <= 0) break;

        /* SELECTION: Tab or Enter confirms the current highlight */
        if (key == '\t' || key == '\n' || key == '\r') {
            el_deletestr(el, prefix_len);
            el_insertstr(el, matches[popup.current_selection]);
            if (key == '\t') el_insertstr(el, " ");
            break;
        }

        /* NAVIGATION: Arrow keys for the 3-column grid */
        if (key == 27) { 
            char buf[2];
            if (read(STDIN_FILENO, buf, 2) == 2 && buf[0] == '[') {
                if (buf[1] == 'A') { /* UP */
                    popup.current_selection = (popup.current_selection - 3 + match_count) % match_count;
                } else if (buf[1] == 'B') { /* DOWN */
                    popup.current_selection = (popup.current_selection + 3) % match_count;
                } else if (buf[1] == 'C') { /* RIGHT */
                    popup.current_selection = (popup.current_selection + 1) % match_count;
                } else if (buf[1] == 'D') { /* LEFT */
                    popup.current_selection = (popup.current_selection - 1 + match_count) % match_count;
                }
                continue;
            }
            break; /* ESC: Revert and exit */
        }

        /* TYPE-THROUGH: Any printable character appends to the line and closes menu */
        if (isprint(key)) {
            char k_str[2] = {key, '\0'};
            el_insertstr(el, k_str);
            break;
        }
    }

    /* 5. CLEANUP: Clear the transient UI and restore the prompt anchor */
    terminal_cleanup_popup(&popup, el);
    if (popup.input_snapshot) free(popup.input_snapshot);

    return CC_REFRESH;
}




/* --- Command Engine --- */

void process_cmd(char *cmd, uint32_t *bar0) {
    char target[64], subcmd[256];
    uint32_t addr, val, count, step;

    /* REPEAT: Recursive automation */
    if (sscanf(cmd, "repeat %u %[^\n]", &count, subcmd) == 2) {
        for (uint32_t i = 0; i < count; i++) {
            process_cmd(subcmd, bar0);
        }
        return;
    }

    /* WAIT: Precision hardware timing */
    if (sscanf(cmd, "wait %u", &val) == 1) {
        printf(COLOR_BLUE "[*] Waiting %u ms...\n" COLOR_RESET, val);
        usleep(val * 1000);
        return;
    }

    /* LOG: */
    if (strncmp(cmd, "log", 3) == 0) {
        dump_log(bar0);
        return;
    }

    
    /* HELP: */
    if (strncmp(cmd, "help", 4) == 0) {
        printf(COLOR_GREEN "\nCOMMANDS:" COLOR_RESET "\n");
        for (int i = 0; command_list[i]; i++) printf("  %s\n", command_list[i]);
    
        printf(COLOR_GREEN "\nREGISTERS:" COLOR_RESET "\n");
        for (int i = 0; dictionary[i].name; i++) printf("  %s\n", dictionary[i].name);
        return;
    }

    
    /* CLEAR: */
    if (strncmp(cmd, "clear", 5) == 0) {
        /* \033[H moves cursor to top-left; \033[J clears from cursor to end of screen */
        const char *ansi_clear_screen_sequence = "\033[H\033[J";
        printf("%s", ansi_clear_screen_sequence);
        return;
    }


    /* FILL: fill <addr> <count> <step> <value> */
    /* Example: fill 0x110000 10 0x20 0 */
    if (sscanf(cmd, "fill %x %u %x %x", &addr, &count, &step, &val) == 4) {
        printf(COLOR_CYAN "[*] Burst-firing %u pokes...\n" COLOR_RESET, count);
        for (uint32_t i = 0; i < count; i++) {
            uint32_t current_addr = addr + (i * step);
            
            /* Safety check for the 2MB BAR0 boundary */
            if (current_addr < 0x200000) {
                bar0[current_addr / 4] = val;
                printf("  [0x%08x] -> 0x%08x (Readback: 0x%08x)\n", 
                        current_addr, val, bar0[current_addr / 4]);
            } else {
                printf(COLOR_RED "  [!] Offset 0x%08x out of bounds!\n" COLOR_RESET, current_addr);
                break;
            }
        }
        return;
    }
    

    /* SCRIPT EXECUTION: script startup.txt */
    if (strncmp(cmd, "script ", 7) == 0) {
        char filename[128];
        sscanf(cmd, "script %127s", filename);
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            printf(COLOR_RED "[-] Script not found: %s\n" COLOR_RESET, filename);
            return;
        }
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0; // Strip newline
            if (strlen(line) > 0 && line[0] != '#') { // Ignore comments
                process_cmd(line, bar0);
            }
        }
        fclose(fp);
        return;
    }



    /* UNTIL: until [limit/inf] <addr> <mask > <target> <cmd...> */
    if (strncmp(cmd, "until ", 6) == 0) {
        char limit_buffer[16], action_buffer[256], addr_name[64]; // Added addr_name buffer
        uint32_t mask, target_value, read_value;
        int iteration_limit = 10;
        int is_infinite = 0;

        struct termios original_termios, raw_termios;
        struct pollfd stdin_poll;
        stdin_poll.fd = STDIN_FILENO;
        stdin_poll.events = POLLIN;

        /* Attempt parsing with limit + string address */
        if (sscanf(cmd, "until %15s %63s %x %x %[^\n]", limit_buffer, addr_name, &mask, &target_value, action_buffer) == 5) {
            is_infinite = (strcmp(limit_buffer, "inf") == 0);
            if (!is_infinite) iteration_limit = atoi(limit_buffer);
            
            /* Resolve Name */
            addr = resolve_name(addr_name);
            if (addr == 0xFFFFFFFF) addr = (uint32_t)strtoul(addr_name, NULL, 16);
        } 
        /* Attempt parsing with default limit + string address */
        else if (sscanf(cmd, "until %63s %x %x %[^\n]", addr_name, &mask, &target_value, action_buffer) == 4) {
            addr = resolve_name(addr_name);
            if (addr == 0xFFFFFFFF) addr = (uint32_t)strtoul(addr_name, NULL, 16);
        } 
        else {
            /* Fixed the typo in the help string too! */
            printf(COLOR_RED "[-] Usage: until [count/inf] <addr> <mask> <target> <cmd>\n" COLOR_RESET);
            return;
        }

        /* Bounds check the resolved address */
        if (addr >= 0x200000) {
            printf(COLOR_RED "[-] Error: Address 0x%08x is outside 2MB BAR0 range.\n" COLOR_RESET, addr);
            return;
        }

        /* Enter "Raw Mode" to catch 'q' without Enter */
        tcgetattr(STDIN_FILENO, &original_termios);
        raw_termios = original_termios;
        raw_termios.c_lflag &= ~(ICANON | ECHO); // Disable buffering and local echo
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);

        printf(COLOR_CYAN "[*] Polling 0x%08x... Press 'q' to stop.\n" COLOR_RESET, addr);
    
        int current_cycle = 0;
        while (is_infinite || current_cycle < iteration_limit) {
            /* Check if 'q' was pressed */
            if (poll(&stdin_poll, 1, 0) > 0) {
                char input_char;
                if (read(STDIN_FILENO, &input_char, 1) > 0) {
                    if (input_char == 'q' || input_char == 'Q') {
                        printf(COLOR_RED "\n[!] User Interrupted.\n" COLOR_RESET);
                        break;
                    }
                }
            }

            read_value = bar0[addr / 4];
            if ((read_value & mask) == target_value) {
                printf(COLOR_GREEN "\n[+] CONDITION MET! Read: 0x%08x\n" COLOR_RESET, read_value);
                /* Return to cooked mode before executing sub-command to avoid prompt mangling */
                tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
                process_cmd(action_buffer, bar0);
                return; 
            }
        
            process_cmd(action_buffer, bar0);
            usleep(10000); 
            current_cycle++;
        }
    
        /* Restore original terminal state */
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    
        if (!is_infinite && current_cycle >= iteration_limit) {
            printf(COLOR_RED "[-] Stop: Limit reached.\n" COLOR_RESET);
        }
        return;
    }




    /* PEEK: Read access */
    if (sscanf(cmd, "peek %63s", target) == 1) {
        addr = resolve_name(target);
        if (addr == 0xFFFFFFFF) addr = (uint32_t)strtoul(target, NULL, 16);
        
        if (addr >= 0x200000) {
            printf(COLOR_RED "[-] Error: Address 0x%08x is outside 2MB BAR0 range.\n" COLOR_RESET, addr);
            return;
        }

        val = bar0[addr / 4];
        printf(COLOR_GREEN "[READ] 0x%08x (%s): 0x%08x\n" COLOR_RESET, addr, target, val);
    }


    /* POKE: Write access */
    else if (sscanf(cmd, "poke %63s %x", target, &val) == 2) {
        addr = resolve_name(target);
        if (addr == 0xFFFFFFFF) addr = (uint32_t)strtoul(target, NULL, 16);
        
        if (addr >= 0x200000) {
            printf(COLOR_RED "[-] Error: Address 0x%08x is outside 2MB BAR0 range.\n" COLOR_RESET, addr);
            return;
        }


        bar0[addr / 4] = val;
        printf(COLOR_YELLOW "[WRITE] 0x%08x (%s) -> 0x%08x (Readback: 0x%08x)\n" COLOR_RESET,
               addr, target, val, bar0[addr / 4]);
    }
}



/* --- The Sandbox (Interactive Mode) --- */

void enter_sandbox(uint32_t *bar0) {
    int char_count;
    const char* input_line;

    EditLine* el = el_init("prime", stdin, stdout, stderr);
    History* hist = history_init();
    HistEvent ev;

    history(hist, &ev, H_SETSIZE, 100);
    el_set(el, EL_HIST, history, hist);
    
    /* CRITICAL: Tell libedit how to handle your colored prompt */
    el_set(el, EL_PROMPT_ESC, prompt_generator, '\1');
    el_set(el, EL_EDITOR, "emacs");

    /* Bind IntelliSense */
    el_set(el, EL_ADDFN, "intelli", "Contextual Intellisense", complete_context);
    el_set(el, EL_BIND, "\t", "intelli", NULL);

    printf(COLOR_YELLOW "\n[SANDBOX] Interactive mode. <Tab> for Registers. Type 'exit' to quit.\n" COLOR_RESET);

    while(1) {
        input_line = el_gets(el, &char_count);
        if (char_count > 0) {
            char* buf = strdup(input_line);
            buf[strcspn(buf, "\n")] = 0;
            if (strlen(buf) > 0) {
                history(hist, &ev, H_ENTER, buf);
                if (strncmp(buf, "exit", 4) == 0) { free(buf); break; }
                process_cmd(buf, bar0);
            }
            free(buf);
        } else break;
    }

    history_end(hist);
    el_end(el);
}


/* --- Entry Point --- */

void print_menu() {
    printf(COLOR_BLUE "\n--- BSD-Prime Interactive Debugger ---\n" COLOR_RESET);
    printf(COLOR_YELLOW "1) WAKE (Ignite PMC 0x200)\n" COLOR_RESET);
    printf(COLOR_GREEN  "2) UNLOCK (Force Bridge 0x210)\n" COLOR_RESET);
    printf(COLOR_WHITE  "3) SCRUB (Deep Reset Sequence)\n" COLOR_RESET);
    printf(COLOR_MAGENTA "4) SANDBOX (Interactive Shell)\n" COLOR_RESET);
    printf(COLOR_BLUE   "0) EXIT\n" COLOR_RESET);
    printf("Select > ");
}

int main() {
    /* Mapping the Turing BAR0 range mapped by ghost_probe */
    uint32_t *gpu_bar0 = (uint32_t *)map_physical_memory(0xAD000000, 0x200000);
    
    if (!gpu_bar0) {
        printf(COLOR_RED "[-] Critical: Failed to map GPU memory. Is ghost_probe running?\n" COLOR_RESET);
        return 1;
    }

    int choice;
    while(1) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;
        getchar(); // Clear the newline

        switch(choice) {
            case 4: 
                enter_sandbox(gpu_bar0); 
                break;
            case 0: 
                return 0;
            default: 
                printf("Option not yet implemented.\n"); 
                break;
        }
    }
    return 0;
}
