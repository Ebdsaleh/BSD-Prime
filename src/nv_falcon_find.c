/* /src/nv_falcon_find.c 
 * Part of the BSD-Prime Suite
 *
 * Purpose: Probes multiple MMIO bases to find the active GSP/SEC2 Falcon.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define NV_BAR0_PHYS            0xac000000
#define MAP_SIZE                0x1000000  

/* Potential Falcon Bases on Turing */
#define BASE_GSP_A              0x00110000
#define BASE_GSP_B              0x00118000
#define BASE_SEC2               0x00007000

/* Falcon Internal Offsets */
#define FALCON_ID               0x12c  /* Version/ID Register */
#define FALCON_SCRATCH0         0x040

#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"


void probe_base(volatile uint32_t *regs, uint32_t base, const char *label) {
    uint32_t id = regs[(base + FALCON_ID) / 4];
    printf("Probing %s (0x%08x): ID = 0x%08x\n", label, base, id);

    if (id != 0 && id != 0xffffffff) { 
        printf(COLOR_GREEN "  [+] FOUND ACTIVE FALCON at %s!\n" COLOR_RESET, label);
        
        /* Try a test write to Scratch */
        regs[(base + FALCON_SCRATCH0) / 4] = 0xA5A5A5A5;
        uint32_t check = regs[(base + FALCON_SCRATCH0) /4 ];
        printf("  [>] Scratch R/W Test: %s\n", (check == 0xA5A5A5A5) ? "PASSED" : "FAILED (LOCKED)");

    }

}


int main () {

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    volatile uint32_t *regs = (volatile uint32_t *)map_base;


    printf(COLOR_CYAN "Salix BSD-Prime: Falcon Discovery Probe\n" COLOR_RESET);
    printf("==========================================================\n");

    probe_base(regs, BASE_GSP_A,    "GSP-Primary");
    probe_base(regs, BASE_GSP_B,    "GSP-Secondary");
    probe_base(regs, BASE_SEC2,     "SEC2-Engine");

    printf("==========================================================\n");

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
