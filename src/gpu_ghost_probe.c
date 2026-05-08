/* src/gpu_ghost_probe.c */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "api/prime.h"

/* PCIe Config Space via MMIO (ECAM/MCFG) on Intel Coffee Lake */
#define INTEL_P2SB_BASE        0xE00F9000  // Bus 0, Dev 31, Fun 1
#define INTEL_ROOT_PORT_BASE   0xE0008000  // Bus 0, Dev 1, Fun 0 
#define NVIDIA_GPU_BASE        0xE0100000  // Bus 1, Dev 0, Fun 0
#define NVIDIA_USB_BASE	       0xE0102000  // ?,    ?,      Fun 2
/* NVIDIA Function 3 (UCSI/Serial) Base Address */
#define NVIDIA_UCSI_BASE       0xE0103000  // ?, ?          FUN 3       

int main() {
    printf("[*] Salix Hardware Probe: Initialising Triple-Handshake...\n");

    // 1. UNHIDE INTEL P2SB & CAPTURE SIDEBAND BAR
    uint32_t *p2sb_cfg = (uint32_t *)map_physical_memory(INTEL_P2SB_BASE, 0x1000);
    if (p2sb_cfg && p2sb_cfg[0] != 0xFFFFFFFF) {
        // Unhide bridge (Clear Bit 8 of P2SBC)
        p2sb_cfg[0xE0 / 4] &= ~(1 << 8);
        printf("[+] P2SB Bridge is now UNHIDDEN.\n");

        uint32_t sbreg_bar = p2sb_cfg[0x10 / 4] & ~0xF;
        printf("[!] SBREG_BAR Located: 0x%08X\n", sbreg_bar);
        unmap_physical_memory(p2sb_cfg, 0x1000);
    } else {
        printf("[-] Warning: P2SB Bridge is hidden or master-locked by BIOS.\n");
    }

    // 2. TARGET THE INTEL ROOT PORT
    uint32_t *intel_cfg = (uint32_t *)map_physical_memory(INTEL_ROOT_PORT_BASE, 0x1000);
    if (!intel_cfg) {
        printf("[-] Failed to map Intel Root Port at 0x%08X\n", INTEL_ROOT_PORT_BASE);
        return 1;
    }

    printf("[*] Opening Intel Root Port (Bus 0, Dev 1, Fun 0)...\n");
    intel_cfg[0xA4 / 4] &= ~0x00000003; // Force D0 Power State
    intel_cfg[0x04 / 4] |= 0x00000006;  // Enable Memory Space + Bus Mastering
    printf("[+] Intel Bridge is now ACTIVE and MASTERING.\n");
    /* // --- TEMPORARILY DISABLED: INTEL-SIDE SECONDARY BUS RESET (SBR) ---
    // We are commenting this out to see if the SBR is triggering a security latch.

    // --- NEW: INTEL-SIDE SECONDARY BUS RESET (SBR) ---
    // The Bridge Control register is at offset 0x3E (16-bit)
    uint16_t *bctrl = (uint16_t *)((uint8_t *)intel_cfg + 0x3E);

    printf("[*] Triggering Intel-side Secondary Bus Reset (SBR)...\n");
    *bctrl |= (1 << 6);  // Assert Reset (Set Bit 6)
    usleep(100000);      // Wait 100ms for the signal to propagate
    *bctrl &= ~(1 << 6); // De-assert Reset
    printf("[+] Reset pulse sent. NVIDIA state machine should be cleared.\n");

    usleep(500000); // Give NVIDIA 500ms to perform its internal boot sequence
    // --------------------------------------------------
    */


    // 2.3 TARGET NVIDIA USB-C/XHCI
    uint32_t *usb_cfg = (uint32_t *)map_physical_memory(NVIDIA_USB_BASE, 0x1000);
    if (usb_cfg && usb_cfg[0] != 0xFFFFFFFF) {
	    printf("[*] Waking NVIDIA USB-C (Function 2) to energize shared rails...\n");
	    usb_cfg[0xA4 / 4] &= ~0x3;  // Force D0 on USB-C
	    usleep(10000);
	    unmap_physical_memory(usb_cfg, 0x1000);
    } else {
	    printf("[-] Failed to Wake the NVIDIA USB-C power rails.\n");
    }

    // 2.4 TARGET NVIDIA Function 3 (UCSI/Serial)
    uint32_t *ucsi_cfg = (uint32_t *)map_physical_memory(NVIDIA_UCSI_BASE, 0x1000);
    if (ucsi_cfg && ucsi_cfg[0] != 0xFFFFFFFF) {
        printf("[*] Waking NVIDIA UCSI (Function 3) to stabilize power delivery...\n");
        ucsi_cfg[0xA4 / 4] &= ~0x3; // Force D0 on UCSI
        usleep(10000);
        unmap_physical_memory(ucsi_cfg, 0x1000);
    } else {
        printf("[-]Failed to Wake the NVIDIA USCI/Serial Function 3 and stablize power delivery.\n");
    }


    // 3. TARGET THE NVIDIA GPU
    uint32_t *gpu_cfg = (uint32_t *)map_physical_memory(NVIDIA_GPU_BASE, 0x1000);
    if (!gpu_cfg || gpu_cfg[0] == 0xFFFFFFFF) {
        printf("[-] Failed to find NVIDIA at 0x%08X. Link is DOWN after reset.\n", NVIDIA_GPU_BASE);
        unmap_physical_memory(intel_cfg, 0x1000);
        return 1;
    }

    printf("[+] NVIDIA FOUND! ID: 0x%08X\n", gpu_cfg[0]);
    gpu_cfg[0xA4 / 4] &= ~0x3; // Force NVIDIA to D0
    
    usleep(10000);

    printf("[*] Assigning BAR0 to 0xAD000000 and Enabling Bus...\n");
    gpu_cfg[0x10 / 4] = 0xAD000000; 
    gpu_cfg[0x04 / 4] |= 0x00000006; 

    printf("[*] Enabling Expansion ROM decoding...\n");
    // Offset 0x30 is the ROM BAR. Set bit 0 to 1 to enable.
    // gpu_cfg[0x30 / 4] |= 0x1;
    // disable it so we don't copy the ROM over our bar0 address (locking us out of the physical silcon
    gpu_cfg[0x30 / 4] &= ~0x00000001; // Force Expansion ROM OFF

    printf("[!] Path unlocked. Hardware reachable at 0xAD000000.\n");

    // Cleanup
    unmap_physical_memory(gpu_cfg, 0x1000);
    unmap_physical_memory(intel_cfg, 0x1000);
    return 0;
}
