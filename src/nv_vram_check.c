/* /src/nv_vram_check.c  */
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* BAR1 Physical Address from the pcidump */
#define VRAM_APERTURE_PHYSICAL_BASE 0x80000000

/* 256MB Limit for the TU104 BAR1 window */
#define VRAM_APERTURE_SIZE_LIMIT (256 * 1024 * 1024)


int main(int argc, char *argv[]) {
    int memory_device_file_descriptor;
    volatile uint32_t *vram_aperture_pointer;

    // Default values if no arguments provided.
    uint32_t block_size_in_bytes = 1024 * 1024;  // 1MB default
    uint32_t offset_from_base = 0;              // 0 default

    /* 1. Parse Command Line Arguments */
    if (argc >= 2) {
        block_size_in_bytes = (uint32_t)strtoul(argv[1], NULL, 0);
    }
    if (argc >= 3) {
        offset_from_base = (uint32_t)strtoul(argv[2], NULL, 0);
    }

    /* 2. Architectual Safety Checks */
    long system_page_size = sysconf(_SC_PAGESIZE);
    if (offset_from_base % system_page_size != 0) {
        fprintf(stderr, "Error: Offset must be page-aligned (%ld bytes).\n", system_page_size);
        return EXIT_FAILURE;
    }

    // the line below may have a missing ')' check when time to compile.
    if ((offset_from_base + block_size_in_bytes) > VRAM_APERTURE_SIZE_LIMIT) {
        fprintf(stderr, "Error: Requested block (0x%x) exceeds 256MB aperture limit.\n",
                offset_from_base + block_size_in_bytes);
        return EXIT_FAILURE;
    }


    /* 3. Open Pysical Memory */
    memory_device_file_descriptor = open("/dev/mem", O_RDWR | O_SYNC);
    if (memory_device_file_descriptor == -1) {
        perror("Error opening /dev/mem");
        return EXIT_FAILURE;
    }

    /* 4. Map the VRAM Aperture */
    vram_aperture_pointer = mmap(NULL, block_size_in_bytes,
                                 PROT_READ | PROT_WRITE, MAP_SHARED,
                                 memory_device_file_descriptor,
                                 VRAM_APERTURE_PHYSICAL_BASE + offset_from_base);

    if (vram_aperture_pointer == MAP_FAILED) {
        perror("mmap failed for VRAM segment");
        close(memory_device_file_descriptor);
        return EXIT_FAILURE;
    }

    printf("VRAM Segment Mapped:  0x%08x [Size: %u bytes, OffsetL 0x%x]\n",
            VRAM_APERTURE_PHYSICAL_BASE + offset_from_base,
            block_size_in_bytes, offset_from_base);
    
    /* 5. The Structural Test (Pseudo-Random Spray) */
    uint32_t element_count = block_size_in_bytes / sizeof(uint32_t);
    uint32_t success_count = 0;


    printf("Spraying test patterns and verifying...\n");
    
    for (uint32_t i = 0; i < element_count; i++) {
        // We use a pattern based on the index to ensure data isn't just "Stuck"
        uint32_t test_pattern = 0xABC00000 | i;
        vram_aperture_pointer[i] = test_pattern;

        if (vram_aperture_pointer[i] == test_pattern) {
            success_count++;
        }
    }

    /* 6. Reporting */
    printf("Test Results: %u %u words verified correctly.\n", success_count, element_count);
    
    if (success_count == element_count) {
        printf("VERDICT: VRAM segment is stable and writeable.\n");
    } else {
        printf("VERDICT: DATA CORRUPTION DETECTED. Check PCIe link or power state.\n" );
    }

    /* 7. Cleanup */
    munmap((void *)vram_aperture_pointer, block_size_in_bytes);
    close(memory_device_file_descriptor);
    return EXIT_SUCCESS;
}
