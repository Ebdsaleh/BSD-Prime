/* /src/nv_gsp_force.c 
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

/* PMC Turing Registers */
#define NV_PMC_DEVICE_RESET     0x00000180 
#define NV_PMC_RESET_MASK       0x00000188 /* The Mask: 1 = Software Control */
#define NV_PMC_DEVICE_ENABLE    0x00000200
#define NV_PMC_ELCG_DIS         0x00001700 /* Energy/Clock Gating Disable */

/* GSP is Bit 19 */
#define PMC_BIT_GSP             (1 << 19)

/* Falcon Registers */
#define NV_PGSP_BASE            0x00110000
#define FALCON_SCRATCH0         (NV_PGSP_BASE + 0x040)

#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"


int main() {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    volatile uint32_t *regs = (volatile uint32_t *)map_base;
    
    printf(COLOR_CYAN "Salix BSD-Prime: GSP Software-Override Force\n" COLOR_RESET);
    printf("==========================================================\n");

    
    /* 1. Disable Clock Gating for the GSP */
    printf("[*] Disabling Hardware Clock Gating (0x1700)...\n");
    regs[NV_PMC_ELCG_DIS / 4] = PMC_BIT_GSP;

    
    /* 2. Set the Reset Mask */
    printf("[*] Taking Manual Control of Reset Line (0x188)...\n");
    regs[NV_PMC_RESET_MASK / 4] = PMC_BIT_GSP;


    /* 3. Force Reset Release and Enable */
    printf("[*] Forcing GSP Power-On Sequence...\n");
    regs[NV_PMC_DEVICE_RESET / 4] = PMC_BIT_GSP;
    regs[NV_PMC_DEVICE_ENABLE / 4] = PMC_BIT_GSP;
    usleep(10000);


    /* 4. The Final Test: Scratch Write */
    printf("[*] Final Probe: Writing 0xA5A5A5A5 to GSP Scratch...\n");
    regs[FALCON_SCRATCH0 / 4] =0xA5A5A5A5;
    uint32_t result = regs[FALCON_SCRATCH0 / 4];


    printf("----------------------------------------------------------\n");
    printf("PMC Reset Status:  0x%08x\n", regs[NV_PMC_DEVICE_RESET / 4]);
    printf("Scratch Readback:  0x%08x\n", result);

    if (result == 0xA5A5A5A5) {
        printf(COLOR_GREEN "[+] SUCCESS: The Silicon is officially UNLOCKED.\n" COLOR_RESET);
    } else {
        printf("[-] Still Locked. Hardware Security (HS) is active.\n");
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
