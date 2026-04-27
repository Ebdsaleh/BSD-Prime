/* /src/nv_gpu_repin.c 
 * Part of the BSD-Prime Suite 
 */

#include <sys/types.h>
#include <sys/pciio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define PCI_NODE      "/dev/pci0"
#define TARGET_BAR0   0xac000000

/* PCI Offsets */
#define REG_GPU_BAR0       0x10
#define REG_BRIDGE_MEM     0x20 /* Memory Base/Limit for the Bridge */

#define COLOR_RED    "\033[1;31m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"

int main() {
    int pci_fd;
    struct pci_io pio;

    printf(COLOR_CYAN "Salix BSD-Prime: PCI Address Re-Pinning\n" COLOR_RESET);
    printf("==========================================================\n");

    pci_fd = open(PCI_NODE, O_RDWR);
    if (pci_fd == -1) {
        perror(COLOR_RED "Fatal: Cannot open /dev/pci0" COLOR_RESET);
        return EXIT_FAILURE;
    }

    pio.pi_width = 4;

    /* 1. Re-Pin the GPU BAR0 (1:0:0) */
    printf("[*] Assigning BAR0 address 0x%08x to GPU (1:0:0)...\n", TARGET_BAR0);
    pio.pi_sel.pc_bus = 1;
    pio.pi_sel.pc_dev = 0;
    pio.pi_sel.pc_func = 0;
    pio.pi_reg = REG_GPU_BAR0;
    pio.pi_data = TARGET_BAR0;

    if (ioctl(pci_fd, PCIOCWRITE, &pio) < 0) {
        perror(COLOR_RED "GPU BAR0 Write Failed" COLOR_RESET);
    }

    /* 2. Update the Bridge Window (0:1:0) */
    /* We need to ensure the Bridge forwards requests for the 0xAC range */
    printf("[*] Updating Bridge (0:1:0) Memory Forwarding Window...\n");

    pio.pi_sel.pc_bus = 0;
    pio.pi_sel.pc_dev = 1;
    pio.pi_sel.pc_func = 0;
    pio.pi_reg = REG_BRIDGE_MEM;

    /* We'll set a 16MB window: Base 0xAC00, Limit 0xAD00 */
    /* (Bits 15:4 define the address in the bridge config) */

    pio.pi_data = 0xad00ac00;

    if (ioctl(pci_fd, PCIOCWRITE, &pio) < 0) {
        perror(COLOR_RED "Bridge Window Write Failed" COLOR_RESET);
    }

    /* 3. Verification */
    printf("----------------------------------------------------------\n");
    pio.pi_sel.pc_bus = 1;
    pio.pi_sel.pc_dev = 0;
    pio.pi_reg = REG_GPU_BAR0;
    ioctl(pci_fd, PCIOCREAD, &pio);

    if (pio.pi_data == TARGET_BAR0) {
        printf(COLOR_GREEN "[+] SUCCESS: GPU is pinned at 0x%08x.\n" COLOR_RESET, pio.pi_data);
    } else {
        printf(COLOR_RED "[!] FAILURE: GPU BAR0 is 0x%08x.\n" COLOR_RESET, pio.pi_data);
    }

    close(pci_fd);
    return EXIT_SUCCESS;

}

