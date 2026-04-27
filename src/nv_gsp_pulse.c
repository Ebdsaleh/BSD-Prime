/* /src/nv_gsp_pulse.c 
 * Part of the BSD-Prime Suite
 *
 * Purpose: Attempts to "pulse" the GSP Falcon core by sending a 
 * hardware HALT signal. Used to verify if the GSP state machine
 * is responsive to BAR0 commands.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* Hardware Constants */
#define NV_BAR0_PHYS            0xac000000
#define MAP_SIZE                0x1000000  

/* Power Management Controller (PMC) */
#define NV_PMC_BASE             0x00000000
#define NV_PMC_ENABLE_FALCON    (NV_PMC_BASE + 0x200)
#define PMC_BIT_GSP_ENABLED     (1 << 17)

/* GSP Falcon Register Offsets */
#define NV_PGSP_BASE            0x00110000
#define NV_PGSP_FALCON_CPUCTL   (NV_PGSP_BASE + 0x200)
#define NV_PGSP_FALCON_STATUS   (NV_PGSP_BASE + 0x214)

/* Bitmask Definitions */
#define FALCON_CPUCTL_HALT      (1 << 1)
#define STATUS_BIT_HALTED       (1 << 1)
#define STATUS_BIT_VALID        (1 << 2)

/* UI Colors - Ensure underscores are present for concatenation */
#define COLOR_RED    "\033[1;31m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"

int main() {
    int mem_fd;
    void *map_base;
    volatile uint32_t *regs;

    printf(COLOR_CYAN "Salix BSD-Prime: GSP Pulse Utility\n" COLOR_RESET);
    printf("==========================================================\n");

    /* 1. Open System Memory */
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        perror(COLOR_RED "Fatal: Cannot open /dev/mem" COLOR_RESET);
        return EXIT_FAILURE;
    }

    /* 2. Map BAR0 Aperture */
    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    if (map_base == MAP_FAILED) {
        perror(COLOR_RED "Fatal: BAR0 mmap failed" COLOR_RESET);
        close(mem_fd);
        return EXIT_FAILURE;
    }
    regs = (volatile uint32_t *)map_base;

    /* 3. Pre-Pulse Audit */
    uint32_t pmc_enable = regs[NV_PMC_ENABLE_FALCON / 4];
    uint32_t initial_status = regs[NV_PGSP_FALCON_STATUS / 4];

    printf("Initial PMC State:  0x%08x\n", pmc_enable);
    printf("Initial GSP Status: 0x%08x\n", initial_status);

    if (!(pmc_enable & PMC_BIT_GSP_ENABLED)) {
        printf(COLOR_YELLOW "Warning: GSP cluster is power-gated in PMC.\n" COLOR_RESET);
        printf("Attempting to enable GSP cluster power...\n");
        regs[NV_PMC_ENABLE_FALCON / 4] |= PMC_BIT_GSP_ENABLED;
        usleep(1000); /* Wait for clock stabilization */
    }

    /* 4. The Pulse: Sending HALT command */
    printf(COLOR_CYAN "Sending HALT signal to Falcon core (CPUCTL 0x200)...\n" COLOR_RESET);
    regs[NV_PGSP_FALCON_CPUCTL / 4] = FALCON_CPUCTL_HALT;
    
    /* Small delay for silicon reaction */
    usleep(5000);

    /* 5. Post-Pulse Verification */
    uint32_t final_status = regs[NV_PGSP_FALCON_STATUS / 4];
    printf("Final GSP Status:   0x%08x\n", final_status);
    printf("----------------------------------------------------------\n");

    /* 6. Results Analysis */
    if (final_status & STATUS_BIT_HALTED) {
        printf(COLOR_GREEN "[+] SUCCESS: The GSP core is now HALTED.\n" COLOR_RESET);
        printf("The silicon is responsive and ready for firmware loading.\n");
    } else if (final_status == 0) {
        printf(COLOR_RED "[!] FAILURE: No response from GSP.\n" COLOR_RESET);
        printf("The engine is not updating status bits. Check VBIOS power state.\n");
    } else {
        printf(COLOR_YELLOW "[?] UNKNOWN: Core responded with 0x%08x.\n" COLOR_RESET, final_status);
        printf("Status VALID bit: %s\n", (final_status & STATUS_BIT_VALID) ? "YES" : "NO");
    }

    printf("==========================================================\n");

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return EXIT_SUCCESS;
}
