/* /src/dev/apf21008.h */
#ifndef _DEV_APF21008_H_
#define _DEV_APF21008_H_
/* PCBID: APF21008
 * Chipset: Intel HM370 (Cannon Lake-H PCH)
 * GPIO Base: 0xFD6B0000 (Correct for HM370)
 * Model: RZ09-0288 (Advanced, RTX 2080 Max-Q)
 */

/* GPIO Pins extracted from SSDT.10 System NVS */
#define PCH_GPIO_GPU_POWER    0x03070013  /* PWG0: Physically sends electricity */
#define PCH_GPIO_GPU_RESET    0x03050016  /* HRG0: Releases the "Brain Freeze" */

/* The PCH GPIO Controller base addresses from your dmesg */
#define PCH_GPIO_COM3_BASE    0xFD6B0000

#endif /* _DEV_APF21008_H_ */

