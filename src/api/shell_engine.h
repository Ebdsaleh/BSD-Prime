/* src/api/shell_engine.h */

#ifndef SALIX_SHELL_ENGINE_H
#define SALIX_SHELL_ENGINE_H

#include <stdint.h>
#include <histedit.h>


/* Shorthand for custom memory mappings */
typedef struct {
    char name[32];
    uint32_t address;
} Alias_Entry;


typedef struct {
    const char *name;
    uint32_t offset;
} RegisterMap;



/* The "Shell Object" state */

typedef struct {
    uint32_t* bar0;
    EditLine* el;
    History* history;
    int running;
    /* Alias storage: 64 slots for custom shorthand */
    Alias_Entry aliases[64];
    int alias_count;
}Prime_Shell;



/* Command handler function pointer */
typedef void (*command_handler)(Prime_Shell* shell, int argc, char** argv);


typedef struct{
    const char* name;
    command_handler handler;
    const char* description;

} Shell_Command;

/* Core API */
uint32_t resolve_name(Prime_Shell *shell, const char *name);
void shell_init(Prime_Shell *shell, uint32_t *bar0);
void shell_execute(Prime_Shell *shell, char *line);
void shell_run(Prime_Shell *shell);
void shell_destroy(Prime_Shell *shell);

#endif
