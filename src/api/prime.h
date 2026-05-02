#ifndef SALIX_API_PRIME_H
#define SALIX_API_PRIME_H

#include <stdint.h>
#include <stddef.h>

/* The Prime Session Context 
 * Holds the state of our connection to the Turing hardware.
 */

typedef struct {
    int                     mem_fd;         /* File descriptor for /dev/mem */
    volatile uint32_t       *bar0_map;      /* Volatile pointer to the mapped MMIO range */
    size_t                  map_size;       /* Typically 16MB (0x1000000) */
    uint32_t                chip_id;        /* Cached ID for verification */


}Prime_Context;

/* API Result Codes */
typedef enum {
    PRIME_SUCCESS = 0,
    PRIME_ERR_ROOT,             /* Not running as root */
    PRIME_ERR_MAP,              /* mmap failed */
    PRIME_ERR_DEVICE,           /* GPU not found or non-NVIDIA */
    PRIME_ERR_BUSY              /* Device is locked or unavailable */
} Prime_Result;


/**
 * Initializes the Salix BSD-Prime environment.
 * Performs the root check, opens /dev/mem, and maps BAR0.
 */

Prime_Result prime_init(Prime_Context *ctx);


/**
 * Diagnostic dump of the current silicon state.
 * Prints PMC, GSP, and Bus status using expressive color coding.
 */
void prime_dump_status(Prime_Context *ctx);


/**
 * Safely unmaps memory and closes file descriptors.
 */
void prime_cleanup(Prime_Context *ctx);



/* The function that opens the window */
void *map_physical_memory(uint64_t phys_addr, size_t size);



/* The function that closes the window  */
void unmap_physical_memory(void *virt_addr, size_t size);


#endif
