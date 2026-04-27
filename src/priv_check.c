/* /src/priv_check.c  */
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/* ANSI Escape Codes for UI Feedback */
#define COLOR_RED    "\033[1;31m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RESET  "\033[0m"
#define CLEAR_SCREEN "\033[H\033[2J"


int main() {
    int sysctl_mib[2];
    int current_secure_level;
    size_t variable_length;

    printf(CLEAR_SCREEN);
    printf("====================================================\n");
    printf("   SALIX BSD-PRIME: SYSTEM PRIVILEGE AUDITOR        \n");
    printf("====================================================\n\n");


    /* Step 1: Configure Management Information Base (MIB) for securelevel */
    sysctl_mib[0] = CTL_KERN;
    sysctl_mib[1] = KERN_SECURELVL;
    variable_length = sizeof(current_secure_level);

    
    /* Step 2: Query the kernel */
    if (sysctl(sysctl_mib, 2, &current_secure_level, &variable_length, NULL, 0) == -1) {
        perror(COLOR_RED "Error: Failed to query kern.sercurelevel" COLOR_RESET);
        return 1;
    }

    
   printf("Current Security Clearance: %s%d%s\n\n", 
           (current_secure_level <= 0) ? COLOR_GREEN : COLOR_RED,
           current_secure_level, 
           COLOR_RESET);    

    /* Step 3: Evaluate environmental readienss */
    if (current_secure_level > 0) {
        printf(COLOR_RED "[!] DIAGNOSTIC BLOCKED\n" COLOR_RESET);
        printf("----------------------------------------------------\n");
        printf("REASON: OpenBSD is in 'Secure Mode'.\n");
        printf("In this state, writes to /dev/mem (VRAM) are strictly\n");
        printf("prohibited by the kernel to protect system integrity.\n\n");
        
        printf(COLOR_YELLOW "REQUIRED OPERATOR ACTION:\n" COLOR_RESET);
        printf("1. Open /etc/sysctl.conf as root.\n");
        printf("2. Set 'kern.securelevel=-1' or '0'.\n");
        printf("3. Reboot the machine to apply the hardware bypass.\n\n");
        
        printf("The nv_memtest64 utility cannot run in this environment.\n");
        printf("====================================================\n");
        return 1;
    } else {
        printf(COLOR_GREEN "[+] CLEARANCE GRANTED\n" COLOR_RESET);
        printf("----------------------------------------------------\n");
        printf("Environment is in 'Insecure/Dev Mode'.\n");
        printf("Direct physical mapping of GPU BARs is PERMITTED.\n\n");
        
        printf("STATUS: Ready to execute nv_memtest64.\n");
        printf("NOTE: Ensure you run the test suite with root privileges.\n");
        printf("====================================================\n");
        return 0;
    }
    
    
}
