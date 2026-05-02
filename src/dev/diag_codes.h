/* /src/dev/diag_codes.h */
#ifndef SALIX_DEV_DIAG_CODES_H
#define SALIX_DEV_DIAG_CODES_H

/* Expected Identity Values */
#define EXPECTED_CHIP_ID_TU104       0x164000a1

/* Healthy State Bitmasks (Verification Context) */
#define DIAG_MASK_GSP_IGNITED        (1 << 19)
#define DIAG_MASK_SEC2_IGNITED       (1 << 18)
#define DIAG_MASK_VGA_DISABLED       0x00000000

/* Success Patterns for Audit */
#define DIAG_PMC_ENABLE_HEALTHY      0x400c0101
#define DIAG_PMC_RESET_RELEASED      0x000c0000 /* Bits 18 & 19 HIGH */

/* GSP Falcon Signatures */
#define DIAG_GSP_STATUS_IDLE         0x00000000
#define DIAG_GSP_STATUS_BOOTING      0x00000001 

#endif
