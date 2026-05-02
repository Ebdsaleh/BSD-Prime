/* src/motherboard_init.c */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "api/hardware_profile.h" 
/* Need this for map_physical_memory and unmap_physical_memory */
#include "api/prime.h" 

/* Cannon Lake-H PCH GPIO Pad Offsets */
#define PAD_CFG_DW0_GPP_D22    0x04B0  /* Reset Pin */
#define PAD_CFG_DW0_GPP_G19    0x0698  /* Power Pin */


/* PCIe PEG Root Port (00:01.0) Offsets (From SSDT.11) */
#define PCI_PEG_BASE           0xE0008000 /* Standard Intel Root Port 1 */
#define LNK_CON_OFFSET         0x00B0     /* Contains Link Disable (Bit 4) */
#define LNK_TRN_OFFSET         0x0504     /* Contains Retrain Link (Bit 0) */

int main() {
    /* 1. Electrify the Rails */
    uint32_t *pch = (uint32_t *)map_physical_memory(PCH_GPIO_COM3_BASE, 0x1000);
    if (!pch) return 1;

    printf("[*] Flipping Hardware Switches...\n");
    pch[PAD_CFG_DW0_GPP_G19 / 4] |= 0x1;  /* Power ON */
    usleep(10000);
    pch[PAD_CFG_DW0_GPP_D22 / 4] &= ~0x1; /* Release RESET */
    printf("[+] Power rails hot. Reset released.\n");
    unmap_physical_memory(pch, 0x1000);

    /* 2. Force PCIe Link Handshake */
    uint32_t *peg = (uint32_t *)map_physical_memory(PCI_PEG_BASE, 0x1000);
    if (!peg) {
        printf("[-] Could not map PCIe Bridge. Check PCIEXBAR address.\n");
        return 1;
    }

    printf("[*] Forcing PCIe Link Training...\n");
    peg[LNK_CON_OFFSET / 4] &= ~(1 << 4); /* Clear Link Disable */
    peg[LNK_TRN_OFFSET / 4] |= 1;          /* Trigger Retrain */

    /* Wait for the hardware handshake to finish */
    usleep(50000); 
    printf("[+] Link Retrained. Handshake complete.\n");

    unmap_physical_memory(peg, 0x1000);
    return 0;
}
