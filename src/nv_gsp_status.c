/* /src/nv_gsp_status.c */
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* BAR0 Physical Address from the pcidump output */
#define NV_BAR0_PHYS 0xac000000
#define MAP_SIZE 0x200000 /* Map 2MB to cover the GSP range */

/* * GSP Falcon Register Offsets (Turing TU104)
 *  These are the standard offsets in the BAR0 space for the GSP engine.
 */

#define NV_PGSP_BASE            0x00110000
#define NV_PGSP_FALCON_CPUCTL   (NV_PGSP_BASE + 0x200)
#define NV_PGSP_FALCON_IDELCTL  (NV_PGSP_BASE + 0x204)
#define NV_PGSP_FALCON_STATUS   (NV_PGSP_BASE + 0x214)
#define NV_PGSP_FALCON_BOOTADDR (NV_PGSP_BASE + 0x240)
#define NV_PGSP_FALCON_MAILBOX0 (NV_PGSP_BASE + 0x040)
#define NV_PGSP_FALCON_MAILBOX1 (NV_PGSP_BASE + 0x044)

int main() {
    int mem_fd;
    void *map_base;
    volatile uint8_t *regs;

    /* 1. Open Physical Memory */
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        perror("Error opening /dev/mem");
        return EXIT_FAILURE;

    }

    /* 2. Map BAR0 */
    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    if(map_base == MAP_FAILED) {
        perror("mmap failed for BAR0");
        close(mem_fd);
        return EXIT_FAILURE;
    }
    regs = (volatile uint8_t *)map_base;

    printf("System GSP Reconnaissance Initialized (BAR0 at 0x%08x)\n", NV_BAR0_PHYS);
    printf("----------------------------------------------------------\n");
    
    /* 3. Read Status Registers */
    uint32_t status     = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_STATUS);
    uint32_t cpuctl     = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_CPUCTL);
    uint32_t mailbox    = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_MAILBOX0);
    uint32_t boot       = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_BOOTADDR);

    printf("GSP Falcon Status:      0x%08x\n", status);
    printf("GSP CPU Control:        0x%08x\n", cpuctl);
    printf("GSP Boot Address:       0x%08x\n", boot);
    printf("GSP Mailbox 0:          0x%08x\n", mailbox);
    printf("----------------------------------------------------------\n");

    /* 4. The Architect's Interpretation */
    printf("Analysis: ");

    // Check if the GSP is halted
    if (status & 0x2) {
        printf("GSP is currently HALTED.\n");
    } else {
        printf("GSP is currently RUNNING or BUSY.\n");
    }
    
    // Check if GSP has initated a handshake (Mailbox 0 often holds status)
    if (mailbox == 0x0) {
        printf("Mailbox is empty. No firmware has been loaded yet.\n");

    } else if (mailbox == 0xFFFFFFFF) {
        printf("Mailbox read returned noise. Is the GSP Powered yet?\n");

    } else {
        printf("Mailbox contains data (0x%08x). Possible firmware signature.\n", mailbox);

    }

    /* 5. Cleanup */
    munmap(map_base, MAP_SIZE);
    close(mem_fd);

    return EXIT_SUCCESS;
}
