/* src/nv_check_gsp.c */
#include <stdio.h>
#include <stdint.h>
#include "api/prime.h"

/* to compile:
 * cc -I. nv_check_gsp.c api/prime.c -o nv_check_gsp
 */

int main() {
    /* Map the GSP base range (starts at 0x110000) */
    uint32_t *gsp_base = (uint32_t *)map_physical_memory(0xAD110000, 0x1000);
    
    if (!gsp_base) return 1;

    printf("[*] --- Turing GSP Handshake Check ---\n");
    
    /* Turing Mailboxes are at offset 0x40 and 0x44 */
    printf("[+] GSP_MAILBOX_0 (0x110040): 0x%08x\n", gsp_base[0x40 / 4]);
    printf("[+] GSP_MAILBOX_1 (0x110044): 0x%08x\n", gsp_base[0x44 / 4]);
    
    /* Check the GSP_FALCON_ID for sanity */
    printf("[+] GSP_FALCON_ID (0x11012C): 0x%08x\n", gsp_base[0x12c / 4]);

    unmap_physical_memory(gsp_base, 0x1000);
    return 0;
}
