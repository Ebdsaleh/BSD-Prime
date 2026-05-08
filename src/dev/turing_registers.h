/* src/dev/turing_registers.h */
#ifndef SALIX_DEV_TURING_REGISTERS_H
#define SALIX_DEV_TURING_REGISTERS_H
RegisterMap dictionary[] = {

    /* Intel PCIe Root Port Configuration Space (Standard Offsets) */
    {"INTEL_BRIDGE_VID_DID",     0x00},    /* Vendor/Device ID */
    {"INTEL_BRIDGE_CMD",         0x04},    /* Command Register (The Master Switch) */
    {"INTEL_BRIDGE_STS",         0x06},    /* Status Register (Check for Errors) */
    {"INTEL_BRIDGE_BCTRL",       0x3E},    /* Bridge Control (Secondary Bus Reset) */
    {"INTEL_BRIDGE_PMCSR",       0xA4},    /* Power Management Control/Status (D-State) */

    /* Internal Bus & Bridge (The "Doorbell" Area) */   
    {"NV_PGRAPH_PRI_GSP_CTL", 0x400A30}, /* Bridge Control: Set bit 0 to 1 to unmask GSP MMIO */ 
    {"NV_PGRAPH_PRI_GSP_ADR", 0x400A34}, /* Bridge Target: Should point to 0x110000 base */
    {"NV_PMC_BOOT_0",         0x000000}, /* Master ID: Must read 0x164xxxxx for TU104 */
    {"NV_PTERMINATION",       0x000600}, /* */

    /* Power and Bridge */
    {"PMC_ENABLE",      0x200},        /* Master Power Gate Control (GSP bit 31, FIFO bit 8, Master bit 0) */
    {"PMC_RESET",       0x204},        /* Master Hardware Reset (Hold or release sub-engines in reset) */
    {"HOST_BRIDGE",     0x210},        /* Host-to-GSP Aperture Key (The "Golden Key" to unlock 0x110000 range) */
    {"PMC_INTR_EN",     0x140},        /* Master Interrupt Enable (Route GSP hardware IRQs to the Host CPU) */
    
    /* GSP Engine (Base 0x110000) */
    {"GSP_FALCON_BASE", 0x110000},      /* Falcon Base Offset */
    {"GSP_ID",          0x11012C},      /* Hardware ID Pulse (Should read 0x000A2000 for TU104) */
    {"MAILBOX0",        0x110040},      /* Boot Args Low Bits (Base 32-bit physical address of manifest) */     
    {"MAILBOX1",        0x110044},      /* Boot Args High Bits (Upper 32-bits for 64-bit VRAM addressing) */
    {"GSP_OS",          0x110080},      /* RISC-V Handshake / Application Version (Satisfies kernel OS check) */
    {"CPUCTL",          0x110100},      /* GSP Start / Halt / Reset Control (The "Ignition" bit 0) */
    {"BOOTADDR",        0x110140},      /* VRAM Code Offset (Physical entry point for the bootloader) */
    {"GSP_IMEMC",       0x110180},      /* Instruction Memory Control (PIO Upload Port) */
    {"GSP_IMEMD",       0x110184},      /* Instruction Memory Data (The "Pipe" for your blob) */
    {"GSP_DMEMC",       0x1101C0},      /* Data Memory Control */
    {"GSP_DMEMD",       0x1101C4},      /* Data Memory Data */
    {"GSP_STATUS",      0x110214},      /* Execution Status (Registers if GSP is 'Busy', 'Idle', or 'Faulted') */
    {"GSP_ENGINE",      0x1103C0},      /* Falcon Engine Reset (Target for kflcnReset engine cycle) */
    
    /* Status & Debug Handshaking */
    {"GSP_FALCON_CPUCTL_ALIAS", 0x110100}, /* Alias for CPUCTL */
    {"GSP_FALCON_IDLESTATE",    0x110214}, /* Alias for GSP_STATUS */
    {"GSP_SCRATCH_0",           0x110040}, /* Alias for MAILBOX0 (Used for Signature/Handshake) */
    {"GSP_SCRATCH_1",           0x110044}, /* Alias for MAILBOX1 (Used for Success/Failure codes) */
    {NULL, 0} // Sentinel to mark end of array
};



#endif      /* #define SALIX_DEV_TURING_REGISTERS_H */

