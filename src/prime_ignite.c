/* /src/prime_ignite.c */
#include <stdio.h>
#include <unistd.h>
#include "api/prime.h"
#include "api/colors.h"
#include "dev/turing_pmc.h"
#include "dev/diag_codes.h"


int main() {
    Prime_Context ctx;
    
    /* 1. Connect to the Hardware */
    if (prime_init(&ctx) != PRIME_SUCCESS) return 1;

    printf(COLOR_INFO "Salix BSD-Prime: Starting Ignition Sequence...\n" COLOR_RESET);

    /* STEP A: Wake up the "Gatekeeper" (Force lights on) */
    printf("[*] Forcing internal clocks to stay ON... ");
    ctx.bar0_map[REG_PMC_ELCG_DIS / 4] = 0xFFFFFFFF; 
    usleep(1000);
    printf("DONE\n");

    
    /* STEP B: Turn off "Safe Mode" (The Open Sesame) */
    printf("[*] Disabling Legacy VGA Mode... ");
    ctx.bar0_map[REG_PMC_VGA_MASTER_CTRL / 4] = 0x0;
    usleep(1000);
    printf("DONE\n");

    
    /* STEP C: Turn on the Power Breakers */
    printf("[*] Powering up the GSP Brain... ");
    ctx.bar0_map[REG_PMC_DEVICE_ENABLE / 4] |= (PMC_BIT_GSP | PMC_BIT_SEC2);
    usleep(5000);
    printf("DONE\n");

    
    /* STEP D: Finally, hit the Reset Button */
    printf("[*] Releasing GSP Reset... ");
    ctx.bar0_map[REG_PMC_DEVICE_RESET / 4] |= PMC_BIT_GSP;
    usleep(1000);

    
    /* Check the result */
    printf("\n" COLOR_INFO "Current Status:\n" COLOR_RESET);
    prime_dump_status(&ctx);

    prime_cleanup(&ctx);
    return 0;
}
