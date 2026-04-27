/* src/nv_load_gsp.c  */
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



// gsp filepath: /etc/firmware/nouveau/tu104/gsp/gsp.bin
#define GSP_FIRMWARE_PATH "/etc/firmware/nouveau/tu104/gsp/gsp.bin"
#define NV_BAR0_PHYS 0xac000000
#define NV_BAR1_PHYS 0x80000000
#define VRAM_APERTURE_SIZE (256 * 1024 * 1024)

/* GSP Landing Pad: 128MB offset in BAR1 (Safe from the 256MB Wall) */
#define GSP_VRAM_OFFSET 0x08000000

/* GPS Falcon Registers */
#define NV_PGSP_BASE            0x00110000
#define NV_PGSP_FALCON_CPUCTL   (NV_PGSP_BASE + 0x100)
#define NV_PGSP_FALCON_STATUS   (NV_PGSP_BASE + 0x114)
#define NV_PGSP_FALCON_BOOTADDR (NV_PGSP_BASE + 0x140)


int main() {
    int mem_fd, fw_fd;
    struct stat st;
    void *bar0, *bar1_pad;
    uint8_t *fw_buffer;

    printf("System GSP Injection Sequence Initiated\n");
    printf("----------------------------------------------------------\n");

    /* 1. Load Firmware from Disk to System RAM */
    fw_fd = open(GSP_FIRMWARE_PATH, O_RDONLY);
    if (fw_fd < 0) {perror("Failed to open gsp.bin"); return 1;}
    fstat(fw_fd, &st);

    /* Safety Check */
    if (GSP_VRAM_OFFSET + st.st_size > VRAM_APERTURE_SIZE) {
        fprintf(stderr, "Error: Firmware (%.2f MiB) + Offset exceeds Aperture!\n",
                (float)st.st_size / 1024 / 1024);
    }

    
    fw_buffer = malloc(st.st_size);
    read(fw_fd, fw_buffer, st.st_size);
    close(fw_fd);
    printf("Step 1: Loaded %lld bytes of GSP firmware into RAM.\n", (long long)st.st_size);

    /* 2. Map BARs 
     * Map BAR0 and BAR1 Landing Pad */
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    bar0 = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);

    // We only need to map the portion of BAR1 where we are landing the firmware
    bar1_pad = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
            mem_fd, NV_BAR1_PHYS + GSP_VRAM_OFFSET );

    if (bar0 == MAP_FAILED || bar1_pad == MAP_FAILED) {
        perror("map failed"); return 1;
    }

    /* 3. Inject Firmware into VRAM */
    printf("Step 2: Injecting firmware into VRAM at offset 0x%x...\n", GSP_VRAM_OFFSET);
    memcpy(bar1_pad, fw_buffer, st.st_size);

    // Verify a few bytes to ensure the transfer wasn't "munted".
    if (memcmp(bar1_pad, fw_buffer, 64) == 0) {
        printf("Step 3: Transfer verified. Silicon has the binary.\n");
    } else {
        printf("CRITICAL ERROR: VRAM transfer corruption!\n");
        return 1;

    }

    
    /* 4. The Handshake: Point and Shoot */
    printf("Step 4: Preparing the Falcon...\n");


    volatile uint32_t *regs = (volatile uint32_t *)bar0;
    
    // A. Force Halt/Reset first (Ensure a clean state).
    regs[NV_PGSP_FALCON_CPUCTL / 4] = 0x2;
    usleep(1000);
    
    // B. Calculate the Internal shifted address.
    // THE GSP see VRAM  starting at 0, so the pointer is just the OFFSET.
    // Falcon requires this to be right-shifted by 8.
    uint32_t internal_shifted_address = GSP_VRAM_OFFSET >> 8;

    printf("Step 5: Setting the Internal Boot Pointer to 0x%08x (Shifted: 0x%x)\n",
            GSP_VRAM_OFFSET, internal_shifted_address);


    regs[NV_PGSP_FALCON_BOOTADDR / 4] = internal_shifted_address;
    
    // C. Kick the Falcon into Start mode

    printf("Step 6: Releasing the Falcon (START)...\n");
    regs[NV_PGSP_FALCON_CPUCTL / 4] = 0x1;

    printf("----------------------------------------------------------\n");
    printf("Injection complete. Run 'nv_gsp_status' to check for a pulse.\n");

    /* Cleanup */
    munmap(bar0, 0x200000);
    munmap(bar1_pad, st.st_size);
    close(mem_fd);
    free(fw_buffer);

    return 0;
}
