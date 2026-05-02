/* src/dev/turing_registers.h */
#ifndef SALIX_DEV_TURING_REGISTERS_H
#define SALIX_DEV_TURING_REGISTERS_H
RegisterMap dictionary[] = {
    /* Power and Bridge */
    {"PMC_ENABLE",   0x200},        /* Master Power Gate Control (GSP bit 31, FIFO bit 8, Master bit 0) */
    {"PMC_RESET",    0x204},        /* Master Hardware Reset (Hold or release sub-engines in reset) */
    {"HOST_BRIDGE",  0x210},        /* Host-to-GSP Aperture Key (The "Golden Key" to unlock 0x110000 range) */
    {"PMC_INTR_EN",  0x140},        /* Master Interrupt Enable (Route GSP hardware IRQs to the Host CPU) */
    
    /* GSP Engine (Base 0x110000) */
    {"GSP_ID",       0x11012C},     /* Hardware ID Pulse (Should read 0x000A2000 for TU104) */
    {"MAILBOX0",     0x110040},     /* Boot Args Low Bits (Base 32-bit physical address of manifest) */     
    {"MAILBOX1",     0x110044},     /* Boot Args High Bits (Upper 32-bits for 64-bit VRAM addressing) */
    {"GSP_OS",       0x110080},     /* RISC-V Handshake / Application Version (Satisfies kernel OS check) */
    {"CPUCTL",       0x110100},     /* GSP Start / Halt / Reset Control (The "Ignition" bit 0) */
    {"BOOTADDR",     0x110140},     /* VRAM Code Offset (Physical entry point for the bootloader) */
    {"GSP_STATUS",   0x110214},     /* Execution Status (Registers if GSP is 'Busy', 'Idle', or 'Faulted') */
    {"GSP_ENGINE",   0x1103C0},     /* Falcon Engine Reset (Target for kflcnReset engine cycle) */
    
    {NULL, 0} // Sentinel to mark end of array
};

/* Formal registry for the help system and IntelliSense */
const char *command_list[] = {
    "peek", "poke", "fill", "until", "clear", 
    "script", "help", "wait", "repeat", "log", "exit", 
    NULL 
};

#endif      /* #define SALIX_DEV_TURING_REGISTERS_H */

