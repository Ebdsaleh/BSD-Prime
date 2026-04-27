/* /src/cmd_reg_on.c */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/pciio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int fd = open("/dev/pci0", O_RDWR); // PCI bus 1 where the GPU lives
    if (fd  < 0) { perror("open"); return 1; }

    struct pci_io pi;
    pi.pi_sel.pc_bus    = 1;
    pi.pi_sel.pc_dev    = 0;
    pi.pi_sel.pc_func   = 0;
    pi.pi_reg           = 0x04;     // The Command Register offset
    pi.pi_width         = 4;

    // Read current state
    ioctl(fd, PCIOCREAD, &pi);
    printf("Current Command: 0x%08x\n", pi.pi_data);

    // Attempt to flip the bit to 0x0007 (IO + MEM + MASTER)
    pi.pi_data = 0x0007;
    
    if (ioctl(fd, PCIOCWRITE, &pi) < 0) {
        perror("OCWRITE");
    }
    else {
        printf("Nudge sent. Check pcidump again.\n");
    }

    close(fd);
    return 0;
}

