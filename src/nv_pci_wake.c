/* /src/nv_pci_wake.c 
 * Part of the BSD-Prime Suite
 *
 * Purpose: Performs a deep audit and "kickstart" of the PCI Bridge.
 * Uses 4-byte aligned register access to satisfy OpenBSD's strict ioctl.
 */

#include <sys/types.h>
#include <sys/pciio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* Target: Intel Bridge at 0:1:0 (The root gatekeeper for the GPU) */
#define PCI_NODE      "/dev/pci0"   // was pci0
#define BRIDGE_BUS    0             // was 0
#define BRIDGE_DEV    1             // was 1
#define BRIDGE_FUNC   0

/* * Register Offsets (Must be 4-byte aligned for OpenBSD)
 * 0x80: Power Management Capability Block
 * 0x3C: Contains Interrupt info (0-15) and Bridge Control (16-31)
 */
#define REG_PM_CAP      0x80
#define REG_BRIDGE_CTRL 0x3C

/* Bitmask for Secondary Bus Reset (Bit 6 of Bridge Control, which is Bit 22 of 0x3C) */
#define MASK_BUS_RESET  (1 << 22)

/* UI Colors */
#define COLOR_RED    "\033[1;31m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"

int main() {
    int pci_fd;
    struct pci_io pio;

    printf(COLOR_CYAN "Salix BSD-Prime: PCI Bridge Kickstart\n" COLOR_RESET);
    printf("==========================================================\n");

    /* 1. Open the Root PCI Bus */
    pci_fd = open(PCI_NODE, O_RDWR);
    if (pci_fd == -1) {
        perror(COLOR_RED "Fatal: Cannot open /dev/pci0" COLOR_RESET);
        return EXIT_FAILURE;
    }

    /* Target the Bridge Selector */
    pio.pi_sel.pc_bus  = BRIDGE_BUS;
    pio.pi_sel.pc_dev  = BRIDGE_DEV;
    pio.pi_sel.pc_func = BRIDGE_FUNC;
    pio.pi_width = 4; /* Always use 4-byte width for aligned registers */

    /* 2. Audit Power State */
    pio.pi_reg = REG_PM_CAP;
    if (ioctl(pci_fd, PCIOCREAD, &pio) < 0) {
        perror(COLOR_RED "PCI Read Failed (PM Block)" COLOR_RESET);
        close(pci_fd);
        return EXIT_FAILURE;
    }

    uint32_t pm_val = pio.pi_data;
    uint8_t pwr_state = pm_val & 0x3;
    printf("Bridge Power State: D%d\n", pwr_state);

    if (pwr_state != 0) {
        printf("[*] Transitioning bridge to D0 Active...\n");
        pio.pi_data = pm_val & ~0x3;
        ioctl(pci_fd, PCIOCWRITE, &pio);
        usleep(10000);
    }

    /* 3. Perform Secondary Bus Reset */
    /* This forces the PCIe link to retrain, making the GPU visible. */
    printf("[*] Initializing Secondary Bus Reset (Bus 1)...\n");
    pio.pi_reg = REG_BRIDGE_CTRL;
    if (ioctl(pci_fd, PCIOCREAD, &pio) < 0) {
        perror(COLOR_RED "PCI Read Failed (Bridge Control)" COLOR_RESET);
        close(pci_fd);
        return EXIT_FAILURE;
    }

    uint32_t original_ctrl = pio.pi_data;

    /* Assert Reset */
    pio.pi_data = original_ctrl | MASK_BUS_RESET;
    if (ioctl(pci_fd, PCIOCWRITE, &pio) < 0) {
        perror(COLOR_RED "PCI Write Failed (Assert Reset)" COLOR_RESET);
    } else {
        printf("  [>] Reset Asserted. Holding for 100ms...\n");
        usleep(100000);

        /* De-assert Reset */
        pio.pi_data = original_ctrl & ~MASK_BUS_RESET;
        ioctl(pci_fd, PCIOCWRITE, &pio);
        printf("  [>] Reset Released. Waiting for link training...\n");
        usleep(200000);
    }

    /* 4. Verification */
    printf("----------------------------------------------------------\n");
    printf(COLOR_GREEN "[+] Kickstart sequence complete.\n" COLOR_RESET);
    printf("Please run 'doas pcidump' to check if Bus 1 is now active.\n");
    printf("==========================================================\n");

    close(pci_fd);
    return EXIT_SUCCESS;
}
