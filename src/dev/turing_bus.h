/* /src/dev/turing_bus.h  */
#ifndef SALIX_DEV_TURING_BUS_H
#define SALIX_DEV_TURING_BUS_H

/* GSP Master Base Address */
#define NV_PGSP_BASE                  0x00110000

/* GSP Message/Mailbox (How we talk to the firmware) */
#define NV_PGSP_FALCON_MAILBOX0       (NV_PGSP_BASE + 0x040)
#define NV_PGSP_FALCON_MAILBOX1       (NV_PGSP_BASE + 0x044)

/* GSP Falcon Core Registers */
#define NV_PGSP_FALCON_CPUCTL         (NV_PGSP_BASE + 0x100)
#define NV_PGSP_FALCON_ID             (NV_PGSP_BASE + 0x12c)
#define NV_PGSP_FALCON_OS_LEGACY      (NV_PGSP_BASE + 0x130)

/* GSP Memory Aperture (IMEM/DMEM access) */
/* moved to turing_gsp.h
 * #define NV_PGSP_FALCON_IMEMC(i)       (NV_PGSP_BASE + 0x180 + ((i)*16))
 * #define NV_PGSP_FALCON_IMEMD(i)       (NV_PGSP_BASE + 0x184 + ((i)*16))
 * #define NV_PGSP_FALCON_DMEMC(i)       (NV_PGSP_BASE + 0x1c0 + ((i)*16))
 * #define NV_PGSP_FALCON_DMEMD(i)       (NV_PGSP_BASE + 0x1c4 + ((i)*16))
*/
#endif
