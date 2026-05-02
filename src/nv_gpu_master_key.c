/* /src/nv_gpu_master_key.c 
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
#define NV_PMC_ENABLE_CONTROL   0x00000640
#define NV_PMC_DEVICE_ENABLE    0x00000200
#define NV_PMC_DEVICE_RESET     0x00000180
#define NV_PMC_RESET_MASK       0x00000188 /* THE MASTER OVERRIDE */
#define NV_PMC_ELCG_DIS         0x00001700
#define NV_PWR_MGMT_CTRL        0x00010f2c 

/* GSP is Bit 19 */
#define PMC_BIT_GSP             (1 << 19)

#define COLOR_GREEN  "\033[1;32m"
#define COLOR_RED    "\033[1;31m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"


int main() {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,  mem_fd, NV_BAR0_PHYS);
    volatile uint32_t *regs = (volatile uint32_t *)map_base;

    printf(COLOR_CYAN "Salix BSD-Prime: Master Hardware Key\n" COLOR_RESET);
    printf("==========================================================\n");

    /* 1. Kill Host Power Management (Stop the "Auto-Off") */
    regs[NV_PWR_MGMT_CTRL / 4] = 0x00000000;

    /* 2. Unlock PMC and Disable Clock Gating */
    regs[NV_PMC_ENABLE_CONTROL / 4] = 0x00000001;
    regs[NV_PMC_ELCG_DIS / 4] |= PMC_BIT_GSP;


    /* 3. Take Manual Control of the Reset Line */
    printf("[*] Seizing Manual Reset Control (0x188)...\n");
    regs[NV_PMC_RESET_MASK / 4] |= PMC_BIT_GSP;
    usleep(1000);

    
    /* 4. Release Reset and Enable */
    printf("[*] Releasing GSP from Hardware Coma...\n");
    regs[NV_PMC_DEVICE_RESET / 4] |= PMC_BIT_GSP;
    regs[NV_PMC_DEVICE_ENABLE / 4] |= PMC_BIT_GSP;
    usleep(5000);


    /* 5. Verification */
    uint32_t reset_val = regs[NV_PMC_DEVICE_RESET /4];
    uint32_t en_val = regs[NV_PMC_DEVICE_ENABLE / 4];

    printf("----------------------------------------------------------\n");
    printf("PMC Reset (0x180):  0x%08x\n", reset_val);
    printf("PMC Enable (0x200): 0x%08x\n", en_val);


    if (reset_val & PMC_BIT_GSP) { 
        printf(COLOR_GREEN "[+] SUCCESS: The Silicon is officially UNLOCKED.\n" COLOR_RESET);
    } else {
        printf(COLOR_RED "[-] FAILURE: Hardware Reset is still locked.\n" COLOR_RESET);
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
