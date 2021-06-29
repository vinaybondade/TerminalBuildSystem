/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <version.h>

#define PS_DDR_SW_UPDATE_ADDR	(unsigned int *)0x10000000
#define PS_DDR_RECOVERY_ADDR	(unsigned int *)0x10000004

#define PS_DDR_STM_NO_APP	(unsigned int *)0x10000008
#define PS_DDR_MB_FAIL		(unsigned int *)0x1000000c
#define PS_DDR_MB_ERROR		(unsigned int *)0x10000010
#define PS_DDR_FPGA_ERROR	(unsigned int *)0x10000014
#define PS_DDR_STM_ERROR	(unsigned int *)0x10000018

#define SW_UPDATE_NEEDED			0xfeedabba
#define SW_RECOVERY_NEEDED			0xdeadbeef
#define SW_UPDATE_R0				0xfaacabba
#define SW_UPDATE_R1				0xd00efaac

#define SW_UPDATE_ST_MEMORY_ADDR		0x18000000
#define SW_MEMORY_SIZE 				   0x20000
#define SW_MEMORY_SIZE_HALF 			   0x10000
#define SW_UPDATE_MEMORY_OFFSET1		       0x4

#define LED_SERVICE_ADDRESS			0x43c30110 /*0x43c3010c* st led */ 
#define LED_SERVICE_COLOR_ADDRESS		0x43c300f4 /*middle led - arm   */

#define LED_POWER_LEFT_ADDRESS			0x43c30110 /*0x43c3010c* st led */ 
#define LED_SERVICE_RIGHT_ADDRESS		0x43c300f4 /*right led - arm   */

DECLARE_GLOBAL_DATA_PTR;

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = getenv("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

		run_command_list(p, -1, 0);

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}

/*
void blink_led () {

	unsigned int *boot_type_fpga_error = PS_DDR_FPGA_ERROR;
	unsigned int *led_service_error = LED_SERVICE_ADDRESS;
	unsigned int *led_service_color = LED_SERVICE_COLOR_ADDRESS;

	if (boot_type_fpga_error[0] != SW_UPDATE_NEEDED) {

		led_service_error[0] = 1;
		udelay(20000);
		led_service_color[0] = 2;
	}
}
*/

void blink_led_ex () {

	unsigned int *boot_type_fpga_error = PS_DDR_FPGA_ERROR;
	unsigned int *led_service_addr = LED_SERVICE_RIGHT_ADDRESS;
	unsigned int *led_st_addr = LED_POWER_LEFT_ADDRESS;

	if (boot_type_fpga_error[0] != SW_UPDATE_NEEDED) {

		// blink the service led with blue color
		led_service_addr[0] = 9;

		//set the st led with red color
		led_st_addr[0] = 4;
	}

}


/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;
 	unsigned int *boot_type = PS_DDR_SW_UPDATE_ADDR;
	unsigned int *boot_type_b_press = PS_DDR_RECOVERY_ADDR;
	unsigned int *boot_type_stm_no_app = PS_DDR_STM_NO_APP;
	unsigned int *boot_type_mb_fail = PS_DDR_MB_FAIL;
	unsigned int *boot_type_mb_error = PS_DDR_MB_ERROR;
	unsigned int *boot_type_fpga_error = PS_DDR_FPGA_ERROR;
	unsigned int *boot_type_stm32_error = PS_DDR_STM_ERROR;

	unsigned int *mem_add_r0 = SW_UPDATE_ST_MEMORY_ADDR + SW_MEMORY_SIZE_HALF;
	unsigned int *mem_add_r1 = SW_UPDATE_ST_MEMORY_ADDR + SW_MEMORY_SIZE_HALF + SW_UPDATE_MEMORY_OFFSET1;
	char * const argv_hsconf[4] = { "hsconf", "set", "wfsettings", NULL };

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

#ifdef CONFIG_VERSION_VARIABLE
	setenv("ver", version_string);  /* set version variable */
#endif /* CONFIG_VERSION_VARIABLE */

	cli_init();

	run_preboot_environment_command();

	/////////////////////////////////////////////

	*mem_add_r1 = 0x0;

	setenv("mmcboot", "ext4load mmc 0:1 0x2000000 zynq-zed.dtb && ext4load mmc 0:1 0x2080000 zImage && ext4load mmc 0:1 0x4000000 zedboard-ramdisk.img && bootz 0x2080000 0x4000000 0x2000000");

	setenv("qspiboot", "sf probe && sf read 0x2000000 0xE00000 0x20000 && sf read 0x2080000 0x100000 0x400000 && sf read 0x4000000 0x500000 0x900000 && bootz 0x2080000 0x4000000 0x2000000");

	setenv("memboot", "bootz 0x2080000 0x4000000 0x2000000");
	setenv("bootdelay", "1");

	/////////////////////////////////////////////////////////////////////////////////

	//printf ("boot_type: %x\n", SW_UPDATE_NEEDED);

	if (boot_type[0] == SW_UPDATE_NEEDED ||
	    boot_type_b_press[0] == SW_UPDATE_NEEDED ||
	    boot_type_fpga_error[0] == SW_UPDATE_NEEDED) {

		////////////////////////////////////
		//QSPI BOOT
		////////////////////////////////////

		printf ("Sta:\tFlash Boot - BUTTON|FPGA|VALID|UBOOT\n");

	        // find hsconf command    

    		cmd_tbl_t *cmdPtr = find_cmd("hsconf");

		////////////////////////////////////
		//blink service led
		////////////////////////////////////

		blink_led_ex();
		
		////////////////////////////////////

		if(!cmdPtr) {
        		printf("Could not find flash read/write function.\n");
        		return 0;
    		}
		
	        if (cmdPtr->cmd(cmdPtr, 0, 3, argv_hsconf) < 0) {
			printf ("hsconf failed - set wfsettings\n");
			return 0;
	        }

		///////////////////////////////////

		*mem_add_r1 = SW_UPDATE_R1;
		setenv("bootcmd", "run qspiboot");
	}
	else if (boot_type_stm_no_app[0] == SW_UPDATE_NEEDED || 
		 boot_type_mb_fail[0] == SW_UPDATE_NEEDED    ||
		 boot_type_mb_error[0] == SW_UPDATE_NEEDED   ||
		 boot_type_stm32_error[0] == SW_UPDATE_NEEDED) {

		/////////////////////////////////////////
		//MMC or QSPI
		/////////////////////////////////////////

		*mem_add_r1 = SW_RECOVERY_NEEDED;

		printf ("Sta:\tFlash\MMC Boot - STM|MB|MB|STM\n");
		setenv("bootcmd", "run qspiboot");

		////////////////////////////////////
		//blink service led
		////////////////////////////////////

		blink_led_ex();

		////////////////////////////////////

		if (do_oscheck () >= 0) {

			setenv("bootcmd", "run memboot");
			printf ("Sta:\tEMMC Boot With MD5 OK\n");
		}
		else {
			setenv("bootcmd", "run qspiboot");
			printf ("Sta:\tFlash Boot - MD5 Error\n");
			
		}

	}
	else {

		if (do_oscheck () >= 0) {

			if (mem_add_r0[0] == SW_UPDATE_R0) {
				blink_led_ex();
			}

			setenv("bootcmd", "run memboot");
			printf ("Sta:\tEMMC Boot With MD5 OK\n");
		}
		else {

			////////////////////////////////////
			//blink service led
			////////////////////////////////////

			blink_led_ex();
			
			////////////////////////////////////

			setenv("bootcmd", "run qspiboot");
			printf ("Sta:\tFlash Boot - MD5 Error\n");
			
		}

	}

	/////////////////////////////////////////////

#if defined(CONFIG_UPDATE_TFTP)
	//update_tftp(0UL, NULL, NULL);
#endif /* CONFIG_UPDATE_TFTP */

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
