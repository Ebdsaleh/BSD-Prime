/* /src/nv_gpu_handshake.c 
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

/* The "Secret Handshake" Registers */
#define NV_PMC_VGA_MASTER_CTRL  0x00000880 /* Master VGA Decode Control */
#define NV_PMC_INTR_EN_0        0x00000140 /* Global Interrupt Enable */
#define NV_PMC_DEVICE_RESET     0x00000180 

/* GSP Bit 19 */
#define PMC_BIT_GSP             (1 << 19)

#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"

int main() {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);
    volatile uint32_t *regs = (volatile uint32_t *)map_base;

    printf(COLOR_CYAN "Salix BSD-Prime: The Secret Handshake (VGA-Mode Exit)\n" COLOR_RESET);
    printf("==========================================================\n");

    /* 1. Disable VGA Decoding */
    /* This is the "Open Sesame." It tells the chip to stop listening to 
       legacy VGA legacy IO and trust the MMIO BAR0 entirely. */
    printf("[*] Sending VGA-Decode Disable (Handshake Phase 1)...\n");
    regs[NV_PMC_VGA_MASTER_CTRL / 4] = 0x00000000;
    usleep(1000);

    /* 2. Global Interrupt Enable Pulse */
    /* On Turing, flipping the global interrupt switch acts as a 
       "kick" to the PMC sequencer. */
    printf("[*] Pulsing Global Interrupts (Handshake Phase 2)...\n");
    regs[NV_PMC_INTR_EN_0 / 4] = 0x00000000;
    usleep(1000);
    regs[NV_PMC_INTR_EN_0 / 4] = 0x00000001;

    /* 3. The Attempt: Release Reset */
    printf("[*] Final Attempt: Releasing GSP Reset...\n");
    regs[NV_PMC_DEVICE_RESET / 4] |= PMC_BIT_GSP;
    usleep(5000);

    /* 4. Verification */
    uint32_t reset_val = regs[NV_PMC_DEVICE_RESET / 4];
    printf("----------------------------------------------------------\n");
    printf("PMC Reset Register (0x180): 0x%08x\n", reset_val);

    if (reset_val & PMC_BIT_GSP) {
        printf(COLOR_GREEN "[+] HANDSHAKE ACCEPTED: The GSP is out of reset.\n" COLOR_RESET);
    } else {
        printf("[-] Handshake Failed. The dGPU is still physically isolated.\n");
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
