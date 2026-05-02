/* src/api/commands.c */
#include "shell_engine.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <time.h>
#include "dev/turing_registers.h"

/* Forward declarations for engine-level utility functions */
extern void shell_execute(Prime_Shell *shell, char *line);
void cmd_help(Prime_Shell *shell, int argc, char **argv);




uint32_t resolve_name(Prime_Shell *shell, const char *name) {
    for (int i = 0; i < shell->alias_count; i++) {
        if (strcasecmp(name, shell->aliases[i].name) == 0) return shell->aliases[i].address;
    }
    for (int i = 0; dictionary[i].name != NULL; i++) {
        if (strcasecmp(name, dictionary[i].name) == 0) return dictionary[i].offset;
    }
    return 0xFFFFFFFF;
}


/* Standardized address resolver for the ecosystem */
uint32_t get_target_address(Prime_Shell *shell, const char *input) {
    /* 1. Try resolving as an Alias or Register Name first */
    uint32_t address = resolve_name(shell, input);
    
    /* 2. If that fails, parse as a raw number (Hex 0x or Decimal) */
    if (address == 0xFFFFFFFF) {
        address = (uint32_t)strtoul(input, NULL, 0);
    }
    return address;
}



void dump_log(uint32_t *bar0) {
    FILE *f = fopen("prime_silicon.log", "a");
    if (!f) return;
    time_t now = time(NULL);
    fprintf(f, "\n--- SILICON DUMP: %s", ctime(&now));
    for (int i = 0; dictionary[i].name != NULL; i++) {
        uint32_t val = bar0[dictionary[i].offset / 4];
        fprintf(f, "[%08x] %-15s : 0x%08x\n", dictionary[i].offset, dictionary[i].name, val);
    }
    fclose(f);
}

/* --- Command Implementations --- */

/* -- CLEAR -- */
void cmd_clear(Prime_Shell *shell, int argc, char **argv) {
    /* Moves cursor to top-left (H) and clears from cursor to end (J) */
    printf("\033[H\033[J");
}

/* -- WAIT -- */
void cmd_wait(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 2) {
        printf(COLOR_RED "[-] Usage: wait <milliseconds>\n" COLOR_RESET);
        return;
    }
    uint32_t milliseconds = (uint32_t)strtoul(argv[1], NULL, 10);
    usleep(milliseconds * 1000);
}

/* -- SCRIPT -- */
void cmd_script(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 2) {
        printf(COLOR_RED "[-] Usage: script <filename>\n" COLOR_RESET);
        return;
    }

    FILE *script_file = fopen(argv[1], "r");
    if (!script_file) {
        printf(COLOR_RED "[-] Error: Script '%s' not found.\n" COLOR_RESET, argv[1]);
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), script_file)) {
        buffer[strcspn(buffer, "\n")] = 0; // Strip newline
        if (strlen(buffer) > 0 && buffer[0] != '#') {
            shell_execute(shell, buffer);
        }
    }
    fclose(script_file);
}


/* -- UNTIL -- */
void cmd_until(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 5) {
        printf(COLOR_RED "[-] Usage: until [count/inf] <addr> <mask> <target> <cmd>\n" COLOR_RESET);
        return;
    }

    int is_infinite = (strcmp(argv[1], "inf") == 0);
    int iteration_limit = is_infinite ? 0 : atoi(argv[1]);
    uint32_t address = resolve_name(shell, argv[2]);
    if (address == 0xFFFFFFFF) address = (uint32_t)strtoul(argv[2], NULL, 16);

    uint32_t mask = (uint32_t)strtoul(argv[3], NULL, 16);
    uint32_t target_value = (uint32_t)strtoul(argv[4], NULL, 16);
    
    /* Reconstruct the sub-command from remaining arguments */
    char sub_command[256] = {0};
    for (int i = 5; i < argc; i++) {
        strlcat(sub_command, argv[i], sizeof(sub_command));
        if (i < argc - 1) strlcat(sub_command, " ", sizeof(sub_command));
    }

    struct termios original_terminal, raw_terminal;
    tcgetattr(STDIN_FILENO, &original_terminal);
    raw_terminal = original_terminal;
    raw_terminal.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_terminal);

    printf(COLOR_CYAN "[*] Polling 0x%08x... Press 'q' to stop.\n" COLOR_RESET, address);

    int current_cycle = 0;
    while (is_infinite || current_cycle < iteration_limit) {
        struct pollfd stdin_poll = { .fd = STDIN_FILENO, .events = POLLIN };
        if (poll(&stdin_poll, 1, 0) > 0) {
            char input_char;
            if (read(STDIN_FILENO, &input_char, 1) > 0 && (input_char == 'q' || input_char == 'Q')) break;
        }

        uint32_t current_value = shell->bar0[address / 4];
        if ((current_value & mask) == target_value) {
            printf(COLOR_GREEN "\n[+] Condition met (Read: 0x%08x)\n" COLOR_RESET, current_value);
            tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
            shell_execute(shell, sub_command);
            return;
        }
        usleep(10000);
        current_cycle++;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
}


/* -- PEEK -- */
void cmd_peek(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 2) {
        printf(COLOR_RED "[-] Usage: peek <reg_name/hex_addr>\n" COLOR_RESET);
        return;
    }
    uint32_t address = resolve_name(shell, argv[1]);
    if (address == 0xFFFFFFFF) address = (uint32_t)strtoul(argv[1], NULL, 16);

    if (address >= 0x200000) {
        printf(COLOR_RED "[-] Error: Address 0x%08x out of range.\n" COLOR_RESET, address);
        return;
    }
    printf(COLOR_GREEN "[READ] 0x%08x (%s): 0x%08x\n" COLOR_RESET, address, argv[1], shell->bar0[address / 4]);
}


/* -- POKE -- */
void cmd_poke(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 3) {
        printf(COLOR_RED "[-] Usage: poke <reg/addr> <value>\n" COLOR_RESET);
        return;
    }
    
    uint32_t address = get_target_address(shell, argv[1]);
    uint32_t value   = (uint32_t)strtoul(argv[2], NULL, 0);

    if (address < 0x200000) {
        shell->bar0[address / 4] = value;
        printf(COLOR_YELLOW "[WRITE] 0x%08x (%s) -> 0x%08x\n" COLOR_RESET, 
               address, argv[1], value);
    } else {
        printf(COLOR_RED "[-] Address 0x%08x out of range.\n" COLOR_RESET, address);
    }
}



/* -- ALIAS -- */
void cmd_alias(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 3) {
        printf(COLOR_CYAN "Current Aliases:\n" COLOR_RESET);
        for (int i = 0; i < shell->alias_count; i++) {
            printf("  %-15s -> 0x%08x\n", shell->aliases[i].name, shell->aliases[i].address);
        }
        printf(COLOR_RED "\nUsage: alias <name> <addr/reg>\n" COLOR_RESET);
        return;
    }

    if (shell->alias_count >= 64) {
        printf(COLOR_RED "[-] Error: Alias table full.\n" COLOR_RESET);
        return;
    }

    const char *alias_name = argv[1];
    uint32_t target_address = resolve_name(shell, argv[2]);
    
    if (target_address == 0xFFFFFFFF) {
        target_address = (uint32_t)strtoul(argv[2], NULL, 16);
    }

    /* Store the new alias */
    strncpy(shell->aliases[shell->alias_count].name, alias_name, 31);
    shell->aliases[shell->alias_count].address = target_address;
    shell->alias_count++;

    printf(COLOR_GREEN "[+] Alias created: %s -> 0x%08x\n" COLOR_RESET, alias_name, target_address);
}


/* --LOG -- */
void cmd_log(Prime_Shell *shell, int argc, char **argv) {
    extern void dump_log(uint32_t *bar0); /* If dump_log is still in main */
    dump_log(shell->bar0);
}


/* -- REPEAT --*/
void cmd_repeat(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 3) {
        printf(COLOR_RED "[-] Usage: repeat <count> <command...>\n" COLOR_RESET);
        return;
    }

    uint32_t iteration_limit = (uint32_t)strtoul(argv[1], NULL, 10);

    /* 1. Build the "Template" once */
    char command_template[256] = {0};
    for (int iteration_index = 2; iteration_index < argc; iteration_index++) {
        strlcat(command_template, argv[iteration_index], sizeof(command_template));
        if (iteration_index < argc - 1) {
            strlcat(command_template, " ", sizeof(command_template));
        }
    }

    /* 2. Execute the loop using disposable copies */
    for (uint32_t current_cycle = 0; current_cycle < iteration_limit; current_cycle++) {
        /* Use strdup to create a fresh copy that shell_execute can safely destroy */
        char *disposable_command = strdup(command_template);
        
        if (disposable_command) {
            shell_execute(shell, disposable_command);
            free(disposable_command); /* Clean up the mess immediately */
        }

        /* If shell was told to stop (e.g. via 'exit'), break the repeat */
        if (!shell->running) break;
    }
}

/* -- FILL: Burst-firing pokes -- */
void cmd_fill(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 5) {
        printf(COLOR_RED "[-] Usage: fill <reg/addr> <count> <step> <value>\n" COLOR_RESET);
        return;
    }

    uint32_t base_address = get_target_address(shell, argv[1]);
    uint32_t count = (uint32_t)strtoul(argv[2], NULL, 0);
    uint32_t step  = (uint32_t)strtoul(argv[3], NULL, 0);
    uint32_t val   = (uint32_t)strtoul(argv[4], NULL, 0);

    printf(COLOR_CYAN "[*] Burst-filling %u offsets starting at %s (0x%08x)...\n" COLOR_RESET, 
           count, argv[1], base_address);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t current_addr = base_address + (i * step);
        if (current_addr < 0x200000) {
            shell->bar0[current_addr / 4] = val;
        } else {
            printf(COLOR_RED "[-] Abort: 0x%08x exceeds BAR0.\n" COLOR_RESET, current_addr);
            break;
        }
    }
}


/* -- SEARCH: Scan for active silicon -- */
void cmd_search(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 4) {
        printf(COLOR_RED "[-] Usage: search <start_addr> <end_addr> <mask> <target>\n" COLOR_RESET);
        return;
    }

    uint32_t start_address = (uint32_t)strtoul(argv[1], NULL, 0);
    uint32_t end_address   = (uint32_t)strtoul(argv[2], NULL, 0);
    uint32_t search_mask    = (uint32_t)strtoul(argv[3], NULL, 0);
    uint32_t search_target  = (uint32_t)strtoul(argv[4], NULL, 0);

    printf(COLOR_CYAN "[*] Scanning 0x%08x to 0x%08x...\n" COLOR_RESET, start_address, end_address);

    for (uint32_t current_offset = start_address; current_offset < end_address; current_offset += 4) {
        if (current_offset >= 0x200000) break;

        uint32_t read_value = shell->bar0[current_offset / 4];
        if ((read_value & search_mask) == search_target) {
            printf(COLOR_GREEN "  [MATCH] 0x%08x : 0x%08x\n" COLOR_RESET, current_offset, read_value);
        }
    }
}



/* -- BITS: Unified binary flag diagnostic -- */
void cmd_bits(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 2) {
        printf(COLOR_RED "[-] Usage: bits <reg/addr>\n" COLOR_RESET);
        return;
    }

    /* Resolve address via the unified ecosystem (Alias/Hex/Dec) */
    uint32_t address = get_target_address(shell, argv[1]);

    if (address >= 0x200000) {
        printf(COLOR_RED "[-] Error: Address 0x%08x out of range.\n" COLOR_RESET, address);
        return;
    }

    /* Read the actual hardware value */
    uint32_t register_value = shell->bar0[address / 4];
    
    printf(COLOR_CYAN "[BITS] 0x%08x (%s): 0x%08x\n" COLOR_RESET, 
           address, argv[1], register_value);

    printf("Binary: ");
    for (int bit_index = 31; bit_index >= 0; bit_index--) {
        /* Extract the specific bit at the current index */
        uint32_t bit_is_set = (register_value >> bit_index) & 1;

        if (bit_is_set) {
            /* Highlight active flags in green */
            printf(COLOR_GREEN "1" COLOR_RESET);
        } else {
            printf("0");
        }

        /* Formatting: Dot every 4 bits, Space every 8 bits for scannability */
        if (bit_index % 8 == 0 && bit_index != 0) {
            printf(" ");
        } else if (bit_index % 4 == 0 && bit_index != 0) {
            printf(".");
        }
    }
    
    /* Footer for bit-position reference */
    printf("\n        ^31      ^24      ^16      ^8       ^0\n");
}

/* -- MASK: Toggle a specific bit index (0-31) -- */
void cmd_mask(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 3) {
        printf(COLOR_RED "[-] Usage: mask <reg/addr> <bit_index> [0/1]\n" COLOR_RESET);
        return;
    }

    uint32_t address = resolve_name(shell, argv[1]);
    if (address == 0xFFFFFFFF) {
        address = (uint32_t)strtoul(argv[1], NULL, 0);
    }

    int bit_index = atoi(argv[2]);
    if (bit_index < 0 || bit_index > 31) {
        printf(COLOR_RED "[-] Error: Bit index must be 0-31.\n" COLOR_RESET);
        return;
    }

    uint32_t current_val = shell->bar0[address / 4];
    uint32_t new_val;

    /* If a third argument is provided (0 or 1), set the bit specifically. 
       Otherwise, toggle the current state. */
    if (argc >= 4) {
        int state = atoi(argv[3]);
        if (state == 1) {
            new_val = current_val | (1u << bit_index);
        } else {
            new_val = current_val & ~(1u << bit_index);
        }
    } else {
        new_val = current_val ^ (1u << bit_index);
    }

    shell->bar0[address / 4] = new_val;
    printf(COLOR_YELLOW "[MASK] 0x%08x (%s) Bit %d updated: 0x%08x -> 0x%08x\n" COLOR_RESET,
           address, argv[1], bit_index, current_val, new_val);
}


/* -- MACRO --*/ 
void cmd_macro(Prime_Shell *shell, int argc, char **argv) {
    if (!shell->recording_active) {
        /* START RECORDING */
        shell->recording_active = 1;
        shell->macro_line_count = 0;
        printf(COLOR_MAGENTA "[*] Macro recording STARTED. Type 'macro' to stop.\n" COLOR_RESET);
    } else {
        /* STOP RECORDING */
        shell->recording_active = 0;
        printf(COLOR_MAGENTA "[*] Macro recording STOPPED. (%d lines captured)\n" COLOR_RESET, shell->macro_line_count);
        if (shell->macro_line_count == 0) return;

        printf("Save as script? (y/n): ");
        char choice = getchar();
        getchar(); // Clear newline

        if (choice == 'y' || choice == 'Y') {
            char filename[64];
            printf("Enter filename: ");
            if (fgets(filename, sizeof(filename), stdin)) {
                filename[strcspn(filename, "\n")] = 0;
                
                FILE *file = fopen(filename, "w");
                if (file) {
                    fprintf(file, "# Salix Macro Export\n");
                    for (int i = 0; i < shell->macro_line_count; i++) {
                        fprintf(file, "%s\n", shell->macro_buffer[i]);
                    }
                    fclose(file);
                    printf(COLOR_GREEN "[+] Saved to %s\n" COLOR_RESET, filename);
                }
            }
        }
    }
}



/* -- STEP -- */
void cmd_step(Prime_Shell *shell, int argc, char **argv) {
    if (argc < 2) {
        printf(COLOR_RED "[-] Usage: step <filename>\n" COLOR_RESET);
        return;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) return;

    char line[256];
    printf(COLOR_CYAN "[*] Stepping through %s. Press [Enter] for next line, 'q' to abort.\n" COLOR_RESET, argv[1]);

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0 || line[0] == '#') continue;

        printf(COLOR_YELLOW "NEXT: %s" COLOR_RESET " (Enter/q) > ", line);
        char input = getchar();
        if (input == 'q' || input == 'Q') break;
        if (input != '\n') getchar(); // Clean up if they typed something before Enter

        shell_execute(shell, line);
    }
    fclose(file);
}


/* --- The Command Registry --- */
const Shell_Command command_table[] = {
    /* Basic Utility */
    { "help",    cmd_help,       "Display this help menu" },
    { "clear",   cmd_clear,      "Clear the terminal screen" },
    { "wait",    cmd_wait,       "Sleep: wait <milliseconds>" },
    
    /* System Management & Restoration */
    { "log",     cmd_log,        "Sync registers to prime_silicon.log" },
    { "macro",   cmd_macro,      "Record session to script: macro (toggle)" },
    { "script",  cmd_script,     "Run a .prime script: script <filename>" },
    { "step",    cmd_step,       "Execute script line-by-line: step <filename>" },

    /* Unified Hardware Interaction (Alias/Hex/Dec aware) */
    { "alias",   cmd_alias,      "Create shorthand: alias <name> <addr/reg>" },
    { "bits",    cmd_bits,       "Unified binary flags: bits <reg/addr>" },
    { "fill",    cmd_fill,       "Unified burst-fill: fill <reg/addr> <n> <step> <val>" },
    { "mask",    cmd_mask,       "Unified bit toggle: mask <reg/addr> <bit> [0/1]" },
    { "peek",    cmd_peek,       "Unified read: peek <reg/addr>" },
    { "poke",    cmd_poke,       "Unified write: poke <reg/addr> <val>" },
    
    /* Advanced Automation */
    { "repeat",  cmd_repeat,     "Repeat a command: repeat <n> <cmd>" },
    { "search",  cmd_search,     "Unified scan: search <start> <end> <mask> <target>" },
    { "until",   cmd_until,      "Unified poll: until <n> <addr> <mask> <target> <cmd>" },

    /* Sentinel */
    { NULL,      NULL,           NULL }
};


/* -- HELP -- */
void cmd_help(Prime_Shell *shell, int argc, char **argv) {
    printf("\n--- Salix Shell Commands ---\n");
    for (int i = 0; command_table[i].name != NULL; i++) {
        printf("  %-10s : %s\n", command_table[i].name, command_table[i].description);
    }
}
