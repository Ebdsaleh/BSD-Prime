/* /src/nv_gpu_persistence.c 
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

/* Power Management Overrides */
#define NV_PWR_MGMT_1           0x000010f0
#define NV_PWR_MGMT_CTRL        0x00010f2c /* Turing Host Power Control */

/* PMC Registers */
#define NV_PMC_ENABLE_CONTROL   0x00000640
#define NV_PMC_DEVICE_ENABLE    0x00000200
#define NV_PMC_DEVICE_RESET     0x00000180
#define NV_PMC_ELCG_DIS         0x00001700

/* Bits 18 (SEC2) and 19 (GSP) */
#define MASK_GSP_SEC2           ((1 << 18) | (1 << 19))

#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"


int main() {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    volatile uint32_t *regs = (volatile uint32_t *)map_base;

    printf(COLOR_CYAN "Salix BSD-Prime: Power Persistence Override\n" COLOR_RESET);
    printf("==========================================================\n");

    /* 1. Disable Host-driven Power Gating */
    printf("[*] Disabling Host Power Gating (0x10f2c)...\n");
    regs[NV_PWR_MGMT_CTRL / 4] = 0x00000000;

    /* 2. Unlock PMC */
    regs[NV_PMC_ELCG_DIS / 4] = 0x00000001;

    /* 3. Disable Clock Gating for these engines */
    printf("[*] Locking Clock Rails for GSP/SEC2...\n");
    regs[NV_PMC_ELCG_DIS / 4] |= MASK_GSP_SEC2;

    /* 4. Force Enable */
    printf("[*] Forcing Engine Wake...\n");
    regs[NV_PMC_DEVICE_ENABLE / 4] |= MASK_GSP_SEC2;
    regs[NV_PMC_DEVICE_RESET / 4] |= MASK_GSP_SEC2;

    usleep(5000);

    /* 5. Check Persistence */
    uint32_t en_val = regs[NV_PMC_DEVICE_ENABLE / 4];
    printf("----------------------------------------------------------\n");
    printf("Current Enable Register: 0x%08x\n", en_val);


    if (en_val & (1 << 19)) { 
        printf(COLOR_GREEN "[+] SUCCESS: Power rail is PERSISTENT.\n" COLOR_RESET);
    } else {
        printf("[-] FAILED: Hardware watchdog still overriding writes.\n");
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;

}
