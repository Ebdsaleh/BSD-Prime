/* src/nv_load_gsp.c  */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


/* Use our organized hardware headers */
#include "api/prime.h"
#include "api/hardware_profile.h"
#include "dev/turing_gsp.h"
#include "dev/turing_pmc.h"



int main() {
    struct stat firmware_stats;
    uint32_t *gpu_bar0_map;
    uint32_t *gpu_vram_map;
    uint8_t *firmware_system_buffer;

    /* Create the Manifest for the VRAM to receive. */
    Gsp_Fw_Wpr_Meta manifest;
    
    printf("[*] GSP Injection: Starting clean hardware bring-up...\n");

    /* 1. Map BAR0 using the verified address from our ghost probe */
    gpu_bar0_map = (uint32_t *)map_physical_memory(0xAD000000, 0x200000);
    if (!gpu_bar0_map) return 1;
    

    /* 2. THE DEEP SCRUB: Sequential Partition Reset */
    printf("[*] Performing Deep Scrub to clear 0xBADF lockout...\n");

    /* A. FORCE ISOLATION: Put the unit in reset BEFORE disabling power */
    gpu_bar0_map[NV_PMC_DEVICE_RESET / 4] &= ~NV_PMC_GSP_ENABLE_BIT; // Hold Reset (Bit 31)
    usleep(50000);

    /* B. POWER DOWN: Kill the Master Toggles */
    gpu_bar0_map[NV_PMC_DEVICE_ENABLE / 4]   &= ~NV_PMC_GSP_ENABLE_BIT; 
    gpu_bar0_map[NV_PMC_DEVICE_ENABLE_2 / 4] &= ~NV_PMC_GSP_ENABLE_BIT; 
    usleep(200000); // 200ms "Cold Sleep"

    /* C. POWER UP: Re-electrify the gates while STILL in reset */
    gpu_bar0_map[NV_PMC_DEVICE_ENABLE / 4]   |= NV_PMC_GSP_ENABLE_BIT;
    gpu_bar0_map[NV_PMC_DEVICE_ENABLE_2 / 4] |= NV_PMC_GSP_ENABLE_BIT;
    usleep(50000);

    /* D. RELEASE RESET: Finally let the logic gates flow */
    gpu_bar0_map[NV_PMC_DEVICE_RESET / 4]    |= NV_PMC_GSP_ENABLE_BIT;
    usleep(50000);

    /* E. BRIDGE BYPASS: Re-insert the Golden Key */
    gpu_bar0_map[0x0000121C / 4] = 0x00000001; 
    gpu_bar0_map[REG_PMC_ELCG_DIS / 4] = 0xFFFFFFFF; // Disable Clock Gating    



    /* 3. Pulse Check: Informative only, do not Abort */
    uint32_t status = gpu_bar0_map[NV_PMC_HOST_UPPER_BRIDGE / 4];
    printf("[*] Host Bridge Readback (0x210): 0x%08x\n", status);

    if (status == 0xBADF5040) {
        printf("[!] Bridge reports BAD FEED, but checking Falcon mailboxes for life...\n");
        // Force a read of the Falcon ID to see if the bridge is actually open
        uint32_t falcon_id = gpu_bar0_map[0x00110000 / 4]; 
        if (falcon_id != 0xBADF5040 && falcon_id != 0xFFFFFFFF) {
            printf("[+] Falcon ID detected: 0x%08x. Bridge is OPEN. Proceeding!\n", falcon_id);
        } else {
            printf("[-] Falcon is unreachable. Lockout is real.\n");
            // Only return 1 here if the Falcon ID itself is BADF
            return 1; 
        }
    }

    
    /* 4. Load Firmware and Manifest */
    FILE *firmware_file = fopen("/etc/firmware/nouveau/tu104/gsp/gsp.bin", "rb");
    if (!firmware_file) return 1;
    fstat(fileno(firmware_file), &firmware_stats);
    firmware_system_buffer = malloc(firmware_stats.st_size);
    fread(firmware_system_buffer, 1, firmware_stats.st_size, firmware_file);
    fclose(firmware_file);

    // Prepare the Manifest
    memset(&manifest, 0, sizeof(manifest));
    manifest.magic = 0xdc3aae21371a60b3ULL;
    manifest.revision = 1;


    // GPS Coordinates: Base 0x80000000 + Offset 0x08001000 = 0x88001000
    manifest.sysmem_addr_of_radix3_elf = 0x88001000;
    manifest.sizeof_radix3_elf = firmware_stats.st_size; // Ensure this is > 0
    manifest.sysmem_addr_of_bootloader = 0x88001000;
    manifest.verified = 0xa0a0a0a0a0a0a0a0ULL;

    // Map a slightly larger area to fit both (Manifest + Firmware)
    gpu_vram_map = (uint32_t *)map_physical_memory(0x88000000, firmware_stats.st_size + 0x1000);

    // Copy Manifest to 0x88000000
    memcpy(gpu_vram_map, &manifest, sizeof(manifest));

    // Copy Firmware to 0x88001000 (Room B)
    memcpy((uint8_t*)gpu_vram_map + 0x1000, firmware_system_buffer, firmware_stats.st_size);
    

    /* 5. Point and Shoot Handshake */
    printf("[*] Executing RISC-V Handshake...\n");
    
    // Tell GSP where the Manifest GPS coordinate is
    gpu_bar0_map[NV_PGSP_FALCON_MAILBOX0 / 4] = 0x88000000; // Lower bits
    gpu_bar0_map[NV_PGSP_FALCON_MAILBOX1 / 4] = 0x00000000; // Upper bits

    // Set RISC-V Version to satisfy kernel_gsp_tu102.c
    gpu_bar0_map[NV_PGSP_FALCON_OS / 4] = 0x1;

    // Execute
    gpu_bar0_map[NV_PGSP_FALCON_CPUCTL / 4] = 0x1;


    /*Cleanup */
    unmap_physical_memory(gpu_bar0_map, 0x200000);
    unmap_physical_memory(gpu_vram_map, firmware_stats.st_size + 0x1000);
    free(firmware_system_buffer);

    printf("[+] GSP Load Sequence Complete. Check scratch mailboxes at 0x110804.\n");
    return 0;
}
