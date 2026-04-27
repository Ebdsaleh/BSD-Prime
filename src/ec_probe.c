// /src/ec_probe


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <machine/sysarch.h>
#include <machine/pio.h>

#define EC_CMD_PORT  0x66
#define EC_DATA_PORT 0x62
#define EC_CMD_READ  0x80

void wait_ec_ibf() {
    while (inb(EC_CMD_PORT) & 0x02); // Wait for Input Buffer Free
}

void wait_ec_obf() {
    while (!(inb(EC_CMD_PORT) & 0x01)); // Wait for Output Buffer Full
}

uint8_t read_ec(uint8_t addr) {
    wait_ec_ibf();
    outb(EC_CMD_PORT, EC_CMD_READ);
    wait_ec_ibf();
    outb(EC_DATA_PORT, addr);
    wait_ec_obf();
    return inb(EC_DATA_PORT);
}

int main() {
    // OpenBSD requires iopl to touch ports
    if (amd64_iopl(1) != 0) {
        perror("amd64_iopl");
        return 1;
    }

    uint8_t tmmd = read_ec(0x5F);
    uint8_t skid = read_ec(0xED);

    printf("Salix Lab - EC Probe Result:\n");
    printf("----------------------------\n");
    printf("TMMD (Offset 0x5F): 0x%02X (%s)\n", tmmd, tmmd ? "Discrete" : "Hybrid");
    printf("SKID (Offset 0xED): 0x%02X\n", skid);

    return 0;
}
