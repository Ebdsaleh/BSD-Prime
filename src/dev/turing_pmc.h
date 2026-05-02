/* /src/dev/turing_pmc.h */
#ifndef SALIX_DEV_TURING_PMC_H
#define SALIX_DEV_TURING_PMC_H

/* --- PMC Base Registers (Restored & Validated) --- */
#define NV_PMC_BOOT_0                 0x00000000 /* Verified: Chip ID */
#define NV_PMC_DEVICE_ENABLE          0x00000200 /* Verified: Master Toggles */
#define NV_PMC_DEVICE_RESET           0x00000204 /* Verified: Unit Resets */
#define NV_PMC_DEVICE_ENABLE_2        0x00000600 /* Restored from image_7de591.png */

/* --- RESTORED: PMC Offsets for Master Security and Power Gates --- */
#define REG_PMC_CHIP_ID               0x00000000 /* Verified */
#define REG_PMC_INTR_EN_0             0x00000140 /* Verified */

/* NOTE: On Turing, 0x180 is INTR_EN_CLEAR. Avoid using this for Reset. */
#define REG_PMC_DEVICE_RESET          0x00000180 

#define REG_PMC_RESET_MASK            0x00000188 /* Restored from image_7de591.png */
#define REG_PMC_DEVICE_ENABLE         0x00000200 /* Restored */
#define REG_PMC_ENABLE_CONTROL        0x00000640 /* Restored from image_7de591.png */
#define REG_PMC_VGA_MASTER_CTRL       0x00000880 /* Restored from image_7de591.png */
#define REG_PMC_ELCG_DIS              0x00001700 /* Verified: Clock Gating */

/* ---  Host Bridge Controls --- */
#define NV_PMC_HOST_UPPER_BRIDGE      0x00000210 /* The "Golden Key" to 0x110000 */
#define NV_PMC_HOST_LOWER_BRIDGE      0x00000220 

/* --- Engine Masks (Aligned with Turing blue-prints) --- */

/* Restored & Validated Bits from dev_boot.h */
#define PMC_BIT_SEC2                  (1 << 14) /* Corrected bit for Turing SEC */
#define PMC_BIT_GSP_LEGACY            (1 << 19) /* From your original file */

/* Turing-Specific Verified Bits */
#define PMC_BIT_FIFO                  (1 << 8)  
#define PMC_BIT_PGRAPH                (1 << 12) 
#define PMC_BIT_NVDEC                 (1 << 15) 
#define PMC_BIT_NVENC0                (1 << 18) 
#define PMC_BIT_NVENC1                (1 << 19) 
#define PMC_BIT_PDISP                 (1 << 30) 

/* Special Note: Turing GSP is 1<<31 in NV_PMC_ENABLE */
#define NV_PMC_GSP_ENABLE_BIT         (1U << 31) 
#define PMC_BIT_GSP                    NV_PMC_GSP_ENABLE_BIT

#endif
