/* /src/dev/turing_gsp.h */
#ifndef SALIX_DEV_TURING_GSP_H
#define SALIX_DEV_TURING_GSP_H

#include <stdint.h>

/* --- GSP Master Base Address --- */
#define NV_PGSP_BASE                       0x00110000 /* Verified */

/* --- Primary Falcon / RISC-V Registers --- */
/* These are the core registers used to control the GSP brain. */
#define NV_PGSP_FALCON_MAILBOX0            (NV_PGSP_BASE + 0x040) /* For Boot Args */
#define NV_PGSP_FALCON_MAILBOX1            (NV_PGSP_BASE + 0x044) /* For Boot Args */
#define NV_PGSP_FALCON_OS                  (NV_PGSP_BASE + 0x080) /* RISC-V App Version */
#define NV_PGSP_FALCON_CPUCTL              (NV_PGSP_BASE + 0x100) /* Reset / Halt / Start */
#define NV_PGSP_FALCON_ID                  (NV_PGSP_BASE + 0x12c) /* Hardware ID Pulse */
#define NV_PGSP_FALCON_BOOTADDR            (NV_PGSP_BASE + 0x140) /* VRAM Code Offset */
#define NV_PGSP_FALCON_STATUS              (NV_PGSP_BASE + 0x214) /* Execution Status */

/* --- Engine Control & Local Reset --- */
#define NV_PGSP_FALCON_ENGINE              (NV_PGSP_BASE + 0x3c0) /* */
#define NV_PGSP_FALCON_ENGINE_RESET_BIT    (1 << 0)               /* */
#define NV_PGSP_FALCON_ENGINE_RESET_TRUE   0x00000001             /* */
#define NV_PGSP_FALCON_ENGINE_RESET_FALSE  0x00000000             /* */

/* --- Scratch Mailboxes (Firmware Heartbeat) --- */
/* Firmware writes "I'm Alive" codes here. */
#define NV_PGSP_MAILBOX(i)                 (0x00110804 + (i)*4)   /* */
#define NV_PGSP_MAILBOX_SIZE               4                      /* */

/* --- GSP Command Queue Heads --- */
/* Used for Host-to-GSP communication. */
#define NV_PGSP_QUEUE_HEAD(i)              (0x00110c00 + (i)*8)   /* */
#define NV_PGSP_QUEUE_HEAD_SIZE            8                      /*[ */

/* --- EMEM (External Memory) Interface --- */
/* Used for direct IMEM/DMEM debugging and access. */
#define NV_PGSP_EMEMC(i)                   (0x00110ac0 + (i)*8)   /* Config */
#define NV_PGSP_EMEMD(i)                   (0x00110ac4 + (i)*8)   /* Data */
#define NV_PGSP_EMEM_AINCR_BIT             (1 << 25)              /* Auto-Increment Read*/

/* --- Architectural Constants --- */
#define FLCN_BLK_ALIGNMENT                 256 /* Memory granularity*/
#define FLCN_RESET_DELAY_COUNT             10  /* Reads needed for signal propagation */


/* GSP WPR Metadata Structure - Exactly 256 bytes */
typedef struct {
    uint64_t magic;                    /* Must be 0xdc3aae21371a60b3ULL */
    uint64_t revision;                 /* Must be 1 */

    /* Data in System Memory */
    uint64_t sysmem_addr_of_radix3_elf;    /* PHYSICAL address of gsp.bin */
    uint64_t sizeof_radix3_elf;          /* Size of gsp.bin */
    uint64_t sysmem_addr_of_bootloader;   /* PHYSICAL address of bootloader */
    uint64_t sizeof_bootloader;         /* Size of bootloader */

    /* Offsets inside the firmware file */
    uint64_t bootloader_code_offset;
    uint64_t bootloader_data_offset;
    uint64_t bootloader_manifest_offset;

    /* Padding and internal state (256 bytes total) */
    uint8_t  internal_state_padding[176]; 

    /* The "Open Sesame" Seal */
    uint64_t verified;                 /* Must be 0xa0a0a0a0a0a0a0a0ULL */
} Gsp_Fw_Wpr_Meta;

typedef Gsp_Fw_Wpr_Meta GspFwWprMeta; // Alias for NVIDIA source compatibility
#endif
