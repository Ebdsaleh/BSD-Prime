/* /src/prime_diag.c */
#include <stdio.h>
#include <stdlib.h>

#include "api/prime.h"
#include "api/colors.h"
#include "dev/turing_pmc.h"
#include "dev/turing_gsp.h"
#include "dev/diag_codes.h"


int main() {
    Prime_Context   ctx;
    Prime_Result    result;
    
    printf(COLOR_INFO "Salix BSD-Prime: Diagnostic & Analysis Tool\n" COLOR_RESET);
    printf("Target Machine: Nergal | Chip: TU104\n");
    printf("----------------------------------------------------------\n");

    
    /* 1. Initialize API */
    result = prime_init(&ctx);
    if (result != PRIME_SUCCESS){
        return 1;
    
    }

    
    /* 2. Run the Core Audit (The API function we wrote earlier) */
    prime_dump_status(&ctx);


    /* 3. Contextual Analysis (The "Expert" Layer) */
    printf(COLOR_INFO "Contextual Analysis:\n" COLOR_RESET);
    
    
    /* Verify Chip Identity */
    if (ctx.chip_id != EXPECTED_CHIP_ID_TU104) {
        printf(COLOR_ERROR " [!] IDENTITY MISMATCH: Expected 0x%08x\n" COLOR_RESET, 
               EXPECTED_CHIP_ID_TU104);
    

    } else {
        printf(COLOR_SUCCESS " [+] IDENTITY VERIFIED: Hardware matches specification.\n" COLOR_RESET);


    }

    /* Verify Reset Interlock */
    uint32_t current_reset = ctx.bar0_map[REG_PMC_DEVICE_RESET / 4];
    if (!(current_reset & DIAG_MASK_GSP_IGNITED)) {
        printf(COLOR_WARN " [?] ANALYSIS: GSP is power-gated or reset-locked by VBIOS.\n" COLOR_RESET);
        printf("     Resolution: Ensure 'nv_gpu_handshake' has cleared VGA mode.\n");

    }


    /* Verify Scratch Register Integrity */
    printf(" [*] Testing MMIO Write-Back... ");
    ctx.bar0_map[REG_GSP_FALCON_STATUS / 4] = 0x0;  /* Clear */
    uint32_t probe = ctx.bar0_map[REG_GSP_FALCON_STATUS / 4];

    if (probe == 0) {
        printf(COLOR_SUCCESS "PASSED\n" COLOR_RESET);
    
    } else {
        printf(COLOR_ERROR "FAILED (Bus isolation active)\n" COLOR_RESET);

    }


    /* 4. Cleanup */
    printf("----------------------------------------------------------\n");
    printf("Audit Complete. Releasing hardware resources...\n");
    prime_cleanup(&ctx);

    return 0;
}
