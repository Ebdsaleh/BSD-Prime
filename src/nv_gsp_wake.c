/* /src/nv_gsp_wake.c 
 * Part of the BSD-Prime Suite
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

/* PMC Register Map (Turing TU104 Specific) */
#define NV_PMC_BASE             0x00000000
#define NV_PMC_BOOT_0           (NV_PMC_BASE + 0x000) /* Read-Only ID */
#define NV_PMC_DEVICE_RESET     (NV_PMC_BASE + 0x180) /* Reset Block */
#define NV_PMC_DEVICE_ENABLE    (NV_PMC_BASE + 0x200) /* Enable Block */
#define NV_PMC_ENABLE_CONTROL   (NV_PMC_BASE + 0x640) /* The "Unlock" Switch */

/* Turing Engine Bits */
#define PMC_BIT_SEC2            (1 << 18)
#define PMC_BIT_GSP             (1 << 19)

/* GSP Falcon Vitals */
#define NV_PGSP_FALCON_STATUS   0x00110214

#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"

int main() {
    int mem_fd;
    void *map_base;
    volatile uint32_t *regs;

    printf(COLOR_CYAN "Salix BSD-Prime: GSP Hardware Ignition\n" COLOR_RESET);
    printf("==========================================================\n");

    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) { perror("Fatal: /dev/mem"); return 1; }

    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    if (map_base == MAP_FAILED) { perror("mmap"); return 1; }
    regs = (volatile uint32_t *)map_base;

    /* 1. Preliminary Check */
    printf("Detected Chip ID: 0x%08x\n", regs[NV_PMC_BOOT_0 / 4]);

    /* 2. Unlock the PMC Engine Controls */
    printf("[*] Unlocking PMC Enable Control (0x640)...\n");
    regs[NV_PMC_ENABLE_CONTROL / 4] = 0x00000001;
    usleep(1000);

    /* 3. Pull the SEC2 and GSP out of Reset */
    /* Writing a 1 to these bits at 0x180 TAKES THEM OUT of reset */
    printf("[*] Releasing SEC2 and GSP from Reset (0x180)...\n");
    regs[NV_PMC_DEVICE_RESET / 4] |= (PMC_BIT_SEC2 | PMC_BIT_GSP);
    usleep(1000);

    /* 4. Enable the Clusters */
    printf("[*] Enabling SEC2 and GSP Engines (0x200)...\n");
    regs[NV_PMC_DEVICE_ENABLE / 4] |= (PMC_BIT_SEC2 | PMC_BIT_GSP);
    usleep(10000);

    /* 5. Verification */
    uint32_t reset_val = regs[NV_PMC_DEVICE_RESET / 4];
    uint32_t en_val    = regs[NV_PMC_DEVICE_ENABLE / 4];
    uint32_t gsp_status = regs[NV_PGSP_FALCON_STATUS / 4];

    printf("----------------------------------------------------------\n");
    printf("Final PMC Reset (0x180):  0x%08x\n", reset_val);
    printf("Final PMC Enable (0x200): 0x%08x\n", en_val);
    printf("GSP Falcon Status (0x214): 0x%08x\n", gsp_status);

    if (en_val & PMC_BIT_GSP) {
        printf(COLOR_GREEN "[+] SUCCESS: The GSP Engine is Powered and Enabled.\n" COLOR_RESET);
    } else {
        printf("[-] Result: Bits did not stick. Hardware may be locked by ACPI.\n");
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
