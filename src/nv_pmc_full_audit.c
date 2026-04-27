/* /src/nv_pmc_full_audit.c 
 * Part of the BSD-Prime Suite
 *
 * Purpose: Performs a synchronized audit of the PMC Reset and Enable blocks.
 * Attempts to clear the hardware reset bit while simultaneously enabling 
 * the GSP cluster power.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* Physical Mapping Constants */
#define NV_BAR0_PHYS            0xac000000
#define MAP_SIZE                0x1000000  

/* PMC Register Map (The GPU's Nervous System) */
#define NV_PMC_BASE             0x00000000
#define NV_PMC_DEVICE_RESET     (NV_PMC_BASE + 0x000) /* Master Reset */
#define NV_PMC_DEVICE_ENABLE    (NV_PMC_BASE + 0x200) /* Master Enable */

/* GSP Bit Definitions for Turing TU104 */
#define PMC_BIT_GSP             (1 << 17)

/* GSP Status Register (The target of our pulse) */
#define NV_PGSP_FALCON_STATUS   0x00110214

/* UI Elements */
#define COLOR_RED    "\033[1;31m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"

int main() {

    int mem_fd;
    void *map_base;
    volatile uint32_t *regs;

    printf(COLOR_CYAN "Salix BSD-Prime: PMC Master Audit\n" COLOR_RESET);
    printf("==========================================================\n");

    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == 1) {
        perror(COLOR_RED "Fatal: /dev/mem access denied" COLOR_RESET);
        return EXIT_FAILURE;
    }

    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    if (map_base == MAP_FAILED) {
        perror(COLOR_RED "Fatal: mmap failed" COLOR_RESET);
        close(mem_fd);
        return EXIT_FAILURE;
    }

    regs = (volatile uint32_t *)map_base;

    /* 1. INITIAL RECON  */
    uint32_t current_reset  = regs[NV_PMC_DEVICE_RESET / 4];
    uint32_t current_enable = regs[NV_PMC_DEVICE_ENABLE / 4];

    printf("CURRENT PMC STATE:\n");
    printf("  [0x000] RESET REGISTER:  0x%08x\n", current_reset);
    printf("  [0x200] ENABLE REGISTER: 0x%08x\n", current_enable);
    printf("----------------------------------------------------------\n");

    /* 2. THE HANDSHAKE ATTEMPT */
    printf(COLOR_CYAN "Action: Attempting Synchronized GSP Power-On Sequence...\n" COLOR_RESET);

    /* On NVIDIA, Bit 17 must be 1 in RESET (Not in Reset) AND 1 in ENABLE (Powered On) */
    regs[NV_PMC_DEVICE_RESET / 4] |= PMC_BIT_GSP;
    regs[NV_PMC_DEVICE_ENABLE / 4] |= PMC_BIT_GSP;

    /* Silicon delay for clock stabilization */
    usleep(10000);

    /* 3. POST-ACTION VERIFICATION */
    uint32_t final_reset        = regs[NV_PMC_DEVICE_RESET / 4];
    uint32_t final_enable       = regs[NV_PMC_DEVICE_ENABLE / 4];
    uint32_t gsp_status         = regs[NV_PGSP_FALCON_STATUS / 4];

    printf("POST-ACTION PMC STATE:\n");
    printf("  [0x000] RESET REGISTER:  0x%08x (%s)\n", 
           final_reset, (final_reset & PMC_BIT_GSP) ? "OUT OF RESET" : "LOCKED IN RESET");
    printf("  [0x200] ENABLE REGISTER: 0x%08x (%s)\n", 
           final_enable, (final_enable & PMC_BIT_GSP) ? "POWERED" : "GATE CLOSED");
    printf("----------------------------------------------------------\n");
    
    printf("FINAL GSP STATUS: 0x%08x\n", gsp_status);

    if (gsp_status != 0) {
        printf(COLOR_GREEN "[+] SILICON AWAKE: The GSP has responded to the PMC handshake.\n" COLOR_RESET);
    } else {
        printf(COLOR_RED "[!] SILICON DARK: The bits are set, but the GSP registers are still zero.\n" COLOR_RESET);
        printf("Analysis: The GSP is likely in D3 (Sleep) or restricted by the mobile ACPI.\n");
    }

    printf("==========================================================\n");

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return EXIT_SUCCESS;
}
