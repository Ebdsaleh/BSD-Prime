/* src/gpu_ghost_probe.c */

#include <stdio.h>
#include <stdint.h>
#include "api/prime.h"

/* Standard Intel PCIe Config Base (PCIEXBAR) + Bus 1, Dev 0, Func 0 */
#define GPU_CONFIG_BASE    0xE0100000 

int main() {
    uint32_t *gpu_cfg = (uint32_t *)map_physical_memory(GPU_CONFIG_BASE, 0x1000);
    if (!gpu_cfg) return 1;

    printf("[*] Probing PCIe Bus 1, Device 0, Function 0...\n");
    uint32_t id = gpu_cfg[0]; // Offset 0x00: Vendor/Device ID
    
    if (id == 0xFFFFFFFF) {
        printf("[-] The ghost is silent. PCIe Link is still DOWN.\n");
    } else {
        printf("[+] GHOST FOUND! ID: 0x%08X (NVIDIA detected)\n", id);
        
        printf("[*] Assigning BAR0 to 0xAD000000...\n");
        gpu_cfg[4] = 0xAD000000; // Offset 0x10: BAR0
        
        printf("[*] Enabling Memory Space and Bus Mastering...\n");
        gpu_cfg[1] |= 0x6; // Offset 0x04: Command Register (Bit 1: Mem, Bit 2: Master)
        
        printf("[!] Configuration complete. Try 'dd' now.\n");
    }

    unmap_physical_memory(gpu_cfg, 0x1000);
    return 0;
}
