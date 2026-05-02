/* /src/api/prime.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include "prime.h"
#include "colors.h"
#include "hardware_profile.h"
#include "../dev/diag_codes.h"



Prime_Result prime_init(Prime_Context *ctx) {
    /* 1. Root Check */
    if (geteuid() != 0) {
        fprintf(stderr, COLOR_ERROR "[!] FATAL: root privileges required (use doas).\n" COLOR_RESET);
        return PRIME_ERR_ROOT;
    }


    /* 2. Open /dev/mem */
    ctx->mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (ctx->mem_fd == -1) {
        perror(COLOR_ERROR "open /dev/mem" COLOR_RESET);
        return  PRIME_ERR_MAP;
    }


    /* 3. Map BAR0 (RTX 2080mq default 16MB) */
    ctx->map_size = 0x1000000;
    ctx->bar0_map = (volatile uint32_t *)mmap(
            NULL,
            ctx->map_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            ctx->mem_fd,
            0xac000000 /* Your verified hardware address */
    );


    if (ctx->bar0_map == MAP_FAILED) {
        perror(COLOR_ERROR "mmap BAR0" COLOR_RESET);
        close(ctx->mem_fd);
        return PRIME_ERR_MAP;
    }


    /* 4. Basic Identity Check (PMC_BOOT_0) */
    ctx->chip_id = ctx->bar0_map[0];    // Offset 0x0
    return PRIME_SUCCESS;                                    
}


void prime_cleanup(Prime_Context *ctx) {
    if (ctx->bar0_map != MAP_FAILED) {
        munmap((void *) ctx->bar0_map, ctx->map_size);
    }
    if (ctx->mem_fd >= 0) {
        close(ctx->mem_fd);
    }
}


void prime_dump_status(Prime_Context *ctx) {

    printf(COLOR_INFO "\nSalix BSD-Prime: Silicon Status Audit\n" COLOR_RESET);
    printf("==========================================================\n");

    
    /* 1. Identity */
    printf("Chip ID:           0x%08x ", ctx->chip_id);
    if (ctx->chip_id == 0x164000a1) {  
        printf(COLOR_SUCCESS "(RTX 2080mq / TU104\n" COLOR_RESET);
    
    } else { 
        printf(COLOR_WARN "(Unknown Silicon)\n" COLOR_RESET);
    
    }


    /* 2. PMC State */

    uint32_t pmc_enable = ctx->bar0_map[REG_PMC_DEVICE_ENABLE / 4];
    uint32_t pmc_reset  = ctx->bar0_map[REG_PMC_DEVICE_RESET / 4];
    uint32_t vga_ctrl    = ctx->bar0_map[REG_PMC_VGA_MASTER_CTRL / 4];


    printf("\n[PMC] Power Management Controller:\n");
    printf("  Enable (0x200):  0x%08x ", pmc_enable);

    if (pmc_enable & PMC_BIT_GSP) {
        printf(COLOR_SUCCESS "[GSP POWERED] " COLOR_RESET);
    
    }
    if (pmc_enable & PMC_BIT_SEC2) {
        printf(COLOR_SUCCESS "[SEC2 POWERED]" COLOR_RESET);
    
    }
    printf("\n");

    
    printf("  Reset  (0x180):  0x%08x ", pmc_reset);
    
    if (!(pmc_reset & PMC_BIT_GSP)) {
        printf(COLOR_ERROR "[GSP FROZEN]" COLOR_RESET);
    
    } else { 
        printf(COLOR_SUCCESS "[GSP RELEASED]" COLOR_RESET);
    
    }
    printf("\n");

    
    printf("  VGA Mode (0x880): 0x%08x ", vga_ctrl);
    
    if (vga_ctrl != 0) {
        printf(COLOR_WARN "[LEGACY MODE]" COLOR_RESET);

    } else {
        printf(COLOR_SUCCESS "[MMIO MODE]" COLOR_RESET);
    
    }
    printf("\n");

    
    /* 3. GSP/Falcon State */
    uint32_t gsp_status = ctx->bar0_map[NV_PGSP_FALCON_STATUS / 4];
    printf("\n[GSP] Falcon Core Status:\n");
    printf("  Status (0x214):  0x%08x ", gsp_status);
    
    if (gsp_status == 0) {
        printf(COLOR_WARN "(Engine Idle/Dark)\n" COLOR_RESET);
    
    } else {
        printf(COLOR_SUCCESS "(Engine Active)\n" COLOR_RESET);

    }
    printf("==========================================================\n\n");

}


void motherboard_power_audit(Prime_Context *ctx) {
    /* 1. Map the Motherboard GPIO Controller */
    /* We map 64KB starting at the COM3 base */
    uint32_t *pch_map = map_physical_memory(PCH_GPIO_COM3_BASE, 0x10000);

    if (!pch_map) {
        printf(COLOR_ERROR "[-] Failed to map PCH GPIO Space\n" COLOR_RESET);
        return;
    }

    /* 2. Calculate the offsets for your specific pins */
    /* Note: These offsets are specific to the HM370/Cannon Lake-H Group Map */
    uint32_t power_pin_reg = 0x600 + (0x13 * 8); // Example offset for GPP_G19
    uint32_t reset_pin_reg = 0x400 + (0x16 * 8); // Example offset for GPP_D22
    
    
    printf(COLOR_INFO "Motherboard GPIO Audit:\n" COLOR_RESET);
    printf("[*] GPU Power Pin (0x%08X) Status: 0x%08X\n", 
            PCH_GPIO_GPU_POWER, pch_map[power_pin_reg / 4]);
    printf("[*] GPU Reset Pin (0x%08X) Status: 0x%08X\n", 
            PCH_GPIO_GPU_RESET, pch_map[reset_pin_reg / 4]);

    unmap_physical_memory(pch_map, 0x10000);

}


/**
 * map_physical_memory
 * -------------------
 * Opens the system memory device and maps a physical hardware address
 * into the process's virtual address space.
 */
void* map_physical_memory(uint64_t phys_addr, size_t size) {
    /* 1. Open the physical memory device */
    int fd = open("/dev/mem", O_RDWR);
    if (fd == -1) {
        perror("[!] Failed to open /dev/mem (Are you root/doas?)");
        return NULL;
    }

    /* 2. Map the physical address to a virtual pointer */
    /* phys_addr: The hardware address (e.g., 0xFD6B0000 from SSDT.10) */

    void* map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phys_addr);
    
    /* 3. The file descriptor is no longer needed after mapping */
    close(fd);

    if (map == MAP_FAILED) {
        perror("[!] mmap failed");
        return NULL;
    }

    return map;

}


/**
 * unmap_physical_memory
 * ---------------------
 * Standard cleanup to release the hardware mapping.
 */

void unmap_physical_memory(void *virt_addr, size_t size) {
    if (virt_addr != NULL) {
        /* munmap is the standard OpenBSD/Unix system call to clean up memory maps */
        munmap(virt_addr, size);
    }
}

