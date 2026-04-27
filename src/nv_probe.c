/* /src/nv_probe.c */
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* Physical address from pcidump (BAR 0) */
#define NV_BAR0_PHYS 0xac000000
/* Size to map - 4KB is enough for basic registers */
#define MAP_SIZE 4096
/* NVIDIA PMC_BOOT_0 register offset  */
#define NV_PMC_BOOT_0 0x00000000

int main() {
    int mem_fd;
    void *map_base;
    uint32_t boot0;

    /* 1. Open the physical memory device */
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        perror("Error opening /dev/mem (Check securelevel/doas)");
        return EXIT_FAILURE;
    }

    /* 2. Map the GPU Register Space (BAR 0) into our virtual memory */
    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, NV_BAR0_PHYS);

    if (map_base == MAP_FAILED) {
        perror("mmap failed (Is the bridge closed or address misaligned?)");
        close(mem_fd);
        return EXIT_FAILURE;
    }

    printf("Successfully mapped BAR0 at 0x%08x\n", NV_BAR0_PHYS);

    /* 3. The Handshake: Read the PMC_BOOT_0 register */
    /* We treat map_base as a volatile pointer to prevent compiler optimization */
    boot0 = *(volatile uint32_t *)((uint8_t *)map_base + NV_PMC_BOOT_0);

    printf("---------------------------------------\n");
    printf("Silicon Signature: 0x%08x\n", boot0);
    printf("---------------------------------------\n");

    /* 4. Basic interpretation */
    if ((boot0 >> 20) == 0x16) {
        printf("Architecture: Turing (TU1xx) Detected.\n");
    } else if (boot0 == 0xFFFFFFFF || boot0 == 0) {
        printf("Warning: Received a null/dead response. The silicon is still comatose.\n");
    }

    /* 5. Cleanup */
    munmap(map_base, MAP_SIZE);
    close(mem_fd);

    return EXIT_SUCCESS;

}

