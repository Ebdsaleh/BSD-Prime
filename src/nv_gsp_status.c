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
#define NV_PGSP_FALCON_IDLECTL  (NV_PGSP_BASE + 0x204)
#define NV_PGSP_FALCON_STATUS   (NV_PGSP_BASE + 0x214)
#define NV_PGSP_FALCON_BOOTADDR (NV_PGSP_BASE + 0x240)
#define NV_PGSP_FALCON_MAILBOX0 (NV_PGSP_BASE + 0x040)
#define NV_PGSP_FALCON_MAILBOX1 (NV_PGSP_BASE + 0x044)

/* Falcon Status Bit Definitions */
#define FALCON_STATUS_IDLE      (1 << 0)
#define FALCON_STATUS_HALTED    (1 << 1)
#define FALCON_STATUS_VALID     (1 << 2)



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
    uint32_t idlectl    = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_IDLECTL);
    uint32_t boot       = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_BOOTADDR);
    uint32_t mbox0      = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_MAILBOX0);
    uint32_t mbox1      = *(volatile uint32_t *)(regs + NV_PGSP_FALCON_MAILBOX1);

    printf("==========================================================\n");
    printf("CORE STATUS:      0x%08x | CPU CONTROL:  0x%08x\n", status, cpuctl);
    printf("IDLE CONTROL:     0x%08x | BOOT ADDRESS: 0x%08x\n", idlectl, boot);
    printf("MAILBOX 0 (CMD):  0x%08x | MAILBOX 1 (RET): 0x%08x\n", mbox0, mbox1);
    printf("----------------------------------------------------------\n");

   /* Granular Bit Analysis */
    printf("BIT ANALYSIS:\n");
    printf("  [ ] CPU HALTED:    %s\n", (status & FALCON_STATUS_HALTED) ? "YES" : "NO");
    printf("  [ ] ENGINE IDLE:   %s\n", (status & FALCON_STATUS_IDLE)   ? "YES" : "NO");
    printf("  [ ] STATUS VALID:  %s\n", (status & FALCON_STATUS_VALID)  ? "YES" : "NO");
    printf("----------------------------------------------------------\n");
    
    /* Interpretation */
    printf("NOTES: ");
    if (!(status & FALCON_STATUS_IDLE) && (mbox0 == 0)) {
        printf("GSP is BUSY but Mailbox is empty. This often indicates the\n");
        printf("               VBIOS is still initializing the Falcon core.\n");
    } else if (status & FALCON_STATUS_HALTED) {
        printf("GSP is HALTED, This is the ideal state for a new firmware load.\n");
    } else {
        printf("GSP is ACTIVE. Manual reset may be required via CPUCTL.\n");
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return EXIT_SUCCESS;
}
