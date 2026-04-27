/* /src/nv_gpu_open_valves.c 
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

/* Target the GPU directly on the now-visible Bus 1 */
#define PCI_NODE      "/dev/pci0"
#define GPU_BUS       1
#define GPU_DEV       0
#define GPU_FUNC      0

/* PCI Command Register is at Offset 0x04 */
#define REG_COMMAND    0x04
#define BIT_MEM_ENABLE (1 << 1) /* Bit 1: Memory Space Enable */
#define BIT_BUS_MASTER (1 << 2) /* Bit 2: Bus Master Enable */

#define COLOR_RED    "\033[1;31m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"


int main () {
    int pci_fd;
    struct pci_io pio;

    printf(COLOR_CYAN "Salix BSD-Prime: GPU Memory Valve Opener\n" COLOR_RESET);
    printf("==========================================================\n");
    
    /* Open the secondary bus (Bus 1) */
    pci_fd = open(PCI_NODE, O_RDWR);
    if (pci_fd == -1) {
        perror(COLOR_RED "Fatal: Cannot open /dev/pci1" COLOR_RESET);
        printf("Note: If this still fail, your bridge went back to sleep.\n");
        return EXIT_FAILURE;
    }

    pio.pi_sel.pc_bus   = GPU_BUS;
    pio.pi_sel.pc_dev   = GPU_DEV;
    pio.pi_sel.pc_func  = GPU_FUNC;
    pio.pi_width = 4;

    /* 1. Read Command Register */
    pio.pi_reg = REG_COMMAND;
    ioctl(pci_fd, PCIOCREAD, &pio);
    uint32_t cmd = pio.pi_data;
    printf("Current GPU Command: 0x%08x\n", cmd);

    /* 2. Enable Memory Access */
    printf("[*] Opening valves (Enabling Memory Space)...\n");
    pio.pi_data = cmd | BIT_MEM_ENABLE | BIT_BUS_MASTER;

    if (ioctl(pci_fd, PCIOCWRITE, &pio) < 0) {
        perror(COLOR_RED "PCI Write Failed" COLOR_RESET);
    } else {
        usleep(10000);
        ioctl(pci_fd, PCIOCREAD, &pio);
        printf("New GPU Command:     0x%08x\n", pio.pi_data);

        if (pio.pi_data & BIT_MEM_ENABLE) {
            printf(COLOR_GREEN "[+] SUCCESS: The valves are open. MMIO (0xac000000) is now live.\n" COLOR_RESET);
        }
    } 

    close(pci_fd);
    return EXIT_SUCCESS;
}
