/* /src/nv_falcon_test.c 
 * Part of the BSD-Prime Suite
 *
 * Purpose: Verifies internal register access to the GSP Falcon core.
 * Attempts to write and read from the Falcon Scratch-0 register.
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

/* Falcon GSP Register Offsets */
#define NV_PGSP_BASE            0x00110000
#define NV_FALCON_SCRATCH0      (NV_PGSP_BASE + 0x040) /* Often used for Mailbox/Scratch */
#define NV_FALCON_SCRATCH1      (NV_PGSP_BASE + 0x044)

#define TEST_PATTERN            0xDEADBEEF

#define COLOR_GREEN  "\033[1;32m"
#define COLOR_RED    "\033[1;31m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"


int main() {
    int mem_fd;
    void *map_base;
    volatile uint32_t *regs;

    printf(COLOR_CYAN "Salix BSD-Prime: Falcon Register Probe\n" COLOR_RESET);
    printf("==========================================================\n");

    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) { perror("open"); return 1; }

    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    if (map_base == MAP_FAILED) { perror("mmap"); return 1; }
    regs = (volatile uint32_t *)map_base;

    /* 1. Read Initial State */
    uint32_t initial = regs[NV_FALCON_SCRATCH0 / 4];
    printf("Initial Scratch0: 0x%08x\n", initial);

    /* 2. Write Test Pattern */
    printf("[*] Writing 0x%08x to GSP Scratch0...\n", TEST_PATTERN);
    regs[NV_FALCON_SCRATCH0 /4] = TEST_PATTERN;

    /* 3. Read Back and Verify */
    uint32_t result = regs[NV_FALCON_SCRATCH0 / 4];
    printf("Readback Result:  0x%08x\n", result);

    if (result == TEST_PATTERN) {
        printf(COLOR_GREEN "[+] VERIFIED: The GSP Falcon registers are R/W accessible.\n" COLOR_RESET);
    } else {
        printf(COLOR_RED "[!] FAILED: Register write did not stick.\n" COLOR_RESET);
    }

    /* 4. Restore (Optional) */
    regs[NV_FALCON_SCRATCH0 / 4] = initial;
    printf("==========================================================\n");

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
