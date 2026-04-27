/* /src/nv_memtest.c  */
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <inttypes.h>
#include <ctype.h>
#include <strings.h>

#define BAR0_BASE 0xac000000
#define BAR1_BASE 0x80000000
#define APERTURE_SIZE (256ULL * 1024 * 1024)
#define TOTAL_VRAM_SIZE (8ULL * 1024 * 1024 * 1024) 
#define TOTAL_BLOCKS 32 

#define NV_PBI_BAR1_WINDOW_CONFIG 0x00001700

// ANSI Macros 
#define CLEAR_SCREEN() printf("\033[H\033[2J")
#define CURSOR_UP(n)   printf("\033[%dA", n)

struct test_telemetry {
    char current_phase[64];
    uint64_t current_offset;
    uint64_t session_bytes_total;
    uint64_t session_bytes_done;
    uint64_t block_bytes_total;
    uint64_t block_bytes_done;
    uint32_t error_count;
    uint32_t current_pass;
    int current_block;
    int is_active;
    int stop_requested;
    time_t start_time;
};

volatile uint32_t *bar0_registers;
volatile uint32_t *bar1_aperture;
struct termios original_tty;

/* --- UI Utilities --- */

void get_input(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin)) {
        buffer[strcspn(buffer, "\n")] = 0; // Strip newline
    }
}

void print_progress_bar(const char *label, uint64_t done, uint64_t total, int width) {
    const char *stars = "****************************************";
    double progress = (total > 0) ? (double)done / total : 0;
    int percentage = (int)(progress * 100);
    if (percentage > 100) percentage = 100;

    int filled_width = (percentage * width) / 100;
    printf("%-10s [%.*s%*s] %3d%%", label, filled_width, stars, width - filled_width, "", percentage);
}

void *watchdog_display_loop(void *argument) {
    struct test_telemetry *t = (struct test_telemetry *)argument;
    while (t->is_active) {
        time_t now = time(NULL);
        int elapsed = (int)(now - t->start_time);

        printf("\r\033[K"); // Clear line
        printf("PHASE: %-16s | PASS: %u | BLOCK: %02d/31 | DLA: 0x%09" PRIx64 "\n", 
               t->current_phase, t->current_pass, t->current_block, t->current_offset);
        
        print_progress_bar("BLOCK ", t->block_bytes_done, t->block_bytes_total, 20);
        printf(" | ERRORS: %u\n", t->error_count);
        
        print_progress_bar("SESSION", t->session_bytes_done, t->session_bytes_total, 20);
        printf(" | TIME: %02d:%02d | 'q' to stop ", elapsed / 60, elapsed % 60);
        
        fflush(stdout);
        usleep(100000);
        if (t->is_active) CURSOR_UP(2); // Jump back up for next refresh
    }
    return NULL;
}

/* --- Core Engine Logic --- */

void check_for_interrupt(struct test_telemetry *t) {
    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
    if (poll(&pfd, 1, 0) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0 && (c == 'q' || c == 'Q')) {
            t->stop_requested = 1;
        }
    }
}

uint64_t parse_size_suffix(const char *input) { 
    if (!input || strlen(input) == 0) return APERTURE_SIZE;
    char *end;
    uint64_t val = strtoull(input, &end, 10);
    if (val == 0) return APERTURE_SIZE;
    while (isspace((unsigned char)*end)) end++;
    if (*end == '\0') return val;
    if (strncasecmp(end, "k", 1) == 0) return val * 1024ULL;
    if (strncasecmp(end, "m", 1) == 0) return val * 1048576ULL;
    return val;
}

void run_block_test(uint64_t size, struct test_telemetry *t) {
    uint32_t elements = (uint32_t)(size / 4);
    t->block_bytes_total = size * 4; // 4 stages
    t->block_bytes_done = 0;

    char *phases[] = {"Fill: Address", "Verify: Address", "Fill: Pattern", "Verify: Pattern"};
    for (int p = 0; p < 4 && !t->stop_requested; p++) {
        strncpy(t->current_phase, phases[p], 63);
        for (uint32_t i = 0; i < elements && !t->stop_requested; i++) {
            if (p == 0) bar1_aperture[i] = (uint32_t)(t->current_offset + (i * 4));
            else if (p == 1) { if (bar1_aperture[i] != (uint32_t)(t->current_offset + (i * 4))) t->error_count++; }
            else if (p == 2) bar1_aperture[i] = 0x55555555;
            else if (p == 3) { if (bar1_aperture[i] != 0x55555555) t->error_count++; }
            
            t->block_bytes_done += 4;
            t->session_bytes_done += 4;
            if (i % 4096 == 0) check_for_interrupt(t); 
        }
    }
}

/* --- Session Management --- */

void enter_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &original_tty);
    raw = original_tty;
    raw.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void restore_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tty);
}

int configure_spot_test(uint64_t* offset, uint64_t* size, int* block) {
    char buf[32];
    printf("Enter Block Number (0-31) [Default: 0]: ");
    get_input(buf, sizeof(buf));
    *block = (strlen(buf) == 0) ? 0 : atoi(buf);
    if (*block < 0 || *block >= TOTAL_BLOCKS) return 0;

    *offset = (uint64_t)(*block) * APERTURE_SIZE;
    printf("Block %d Selected: (DLA: 0x%09" PRIx64 ")\n", *block, *offset);

    printf("Enter Size to test [Default: 256MB]: ");
    get_input(buf, sizeof(buf));
    *size = parse_size_suffix(buf);
    if (*size > APERTURE_SIZE) *size = APERTURE_SIZE;

    printf("Confirm Block %d, (DLA: 0x%09" PRIx64 "): [(Y)es]/(N)o? ", *block, *offset);
    get_input(buf, sizeof(buf));
    return (buf[0] == 'n' || buf[0] == 'N') ? 0 : 1;
}

void execute_session(int choice, uint64_t off, uint64_t sz, struct test_telemetry* t) {
    pthread_t tid;
    t->is_active = 1;
    t->start_time = time(NULL);
    t->current_pass = 1;

    CLEAR_SCREEN();
    enter_raw_mode();
    pthread_create(&tid, NULL, watchdog_display_loop, t);

    if (choice == 1) {
        t->current_offset = off;
        bar0_registers[NV_PBI_BAR1_WINDOW_CONFIG / 4] = (uint32_t)(off >> 16);
        run_block_test(sz, t);
    } else {
        do {
            t->session_bytes_done = 0;
            for (int b = 0; b < TOTAL_BLOCKS && !t->stop_requested; b++) {
                t->current_block = b;
                t->current_offset = (uint64_t)b * APERTURE_SIZE;
                bar0_registers[NV_PBI_BAR1_WINDOW_CONFIG / 4] = (uint32_t)(t->current_offset >> 16);
                usleep(100);
                run_block_test(APERTURE_SIZE, t);
            }
            t->current_pass++;
        } while (choice == 3 && !t->stop_requested);
    }

    t->is_active = 0;
    pthread_join(tid, NULL);
    restore_terminal_mode();
    printf("\n\n"); // Move past the fixed UI lines
}

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) { perror("Open /dev/mem"); return 1; }
    bar0_registers = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR0_BASE);
    bar1_aperture = mmap(NULL, (size_t)APERTURE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR1_BASE);

    char m_in[16];
    while(1) {
        CLEAR_SCREEN();
        printf("Salix Prime-BSD VRAM Suite | RTX 2080mq (TU104)\n");
        printf("1. Block Spot Test\n2. Full Test (8GB)\n3. Perpetual Stress\n4. Quit\nSelection: ");
        get_input(m_in, sizeof(m_in));
        int choice = atoi(m_in);
        if (choice == 4) break;
        if (choice < 1 || choice > 3) continue;

        struct test_telemetry t = {0};
        uint64_t off = 0, sz = 0;
        int blk = 0;

        if (choice == 1) {
            if (!configure_spot_test(&off, &sz, &blk)) continue;
            t.current_block = blk;
            t.session_bytes_total = sz * 4; 
        } else {
            t.session_bytes_total = TOTAL_VRAM_SIZE * 4;
        }

        execute_session(choice, off, sz, &t);
        if (t.stop_requested) printf(">>> INTERRUPTED BY OPERATOR <<<\n");
        else printf("Test Complete. Total Errors: %u\n", t.error_count);
        
        printf("\nPress Enter to return to menu...");
        get_input(m_in, sizeof(m_in));
    }
    
    munmap((void *)bar0_registers, 0x10000);
    munmap((void *)bar1_aperture, (size_t)APERTURE_SIZE);
    close(fd);
    return 0;

}
