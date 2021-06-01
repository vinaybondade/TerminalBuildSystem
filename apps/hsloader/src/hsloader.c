
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include "hsloader.h"

#define SW_UPDATE_R1	0xd00efaac
#define SW_UPDATE_R2	0xdeadbeef
#define SW_UPDATE_APP	0xfaacabba

#if defined (sauboard)
#define EXE_PATH    "/opt/sauboard/bin/"
#else
#define EXE_PATH    "/opt/zedboard/bin/"
#endif

int main(int argc, char * argv[])
{
    //int res = 0;
    unsigned int fsbl_value = 0;
    unsigned int app_value = 0;
    int fd= 0;

    printf ("/////////////////////////////////////////////////////////\n");
    printf ("// HS Loader App Version 1.0.0\n");
    printf ("/////////////////////////////////////////////////////////\n");

    fd = open("/dev/hsmisc", O_RDWR|O_CLOEXEC);

    if(fd < 0){
        printf("Failed to open hisky-misc device.\n");
        return -1;
    }

    /////////////////////////////////////////////////////////
    // Read the Status
    /////////////////////////////////////////////////////////

    ioctl(fd, POWER_INTERFACE_STARTUP_READ, &app_value);
    //printf ("read app_value: %x\n", app_value);

    ioctl(fd, POWER_INTERFACE_STARTUP_READ_R1, &fsbl_value);
    //printf ("read uboot_value: %x\n", fsbl_value);

    close(fd);

    printf("EXE Path - %s.\n", EXE_PATH);

    if (app_value == SW_UPDATE_APP ||
	fsbl_value == SW_UPDATE_R1 ||
	fsbl_value == SW_UPDATE_R2) {

	system (EXE_PATH"swu.elf &");
    }
    else {

	system (EXE_PATH"hs_udp_service &");
	//system ("/opt/zedboard/bin/ble-service -i 127.0.0.1 -t 10000000 &");
	system ("/etc/hs-ble-service.sh");
	system (EXE_PATH"terminal_app &");
	usleep(1000000);

	system (EXE_PATH"hsModem.elf -i -a -f -c 7 &"); //-i -m -a &
    }

    /////////////////////////////////////////////////////////

    return 0;
}
