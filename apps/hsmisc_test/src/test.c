
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "power-interface.h"

int main(int argc, char * argv[])
{
    int res = 0;
    //unsigned int ui_value = 0;

    int fd = open("/dev/hsmisc", O_RDWR|O_CLOEXEC);

    if(fd < 0){
        printf("Failed to open hisky-misc device.\n");
        return -1;
    }

    /* Restart*/

    res = ioctl(fd, POWER_INTERFACE_REBOOT, 1);

    if(res < 0){
        printf("ioctl failed.\n");
        return -1;
    }
    

    //Write

    /*
    ui_value = 0x55667788;
    res = ioctl(fd, POWER_INTERFACE_STARTUP_WRITE, &ui_value);
    printf ("write ui_value: %x\n", ui_value);

    if(res < 0){
        printf("ioctl failed.\n");
        return -1;
    }

    //Read
	
    ui_value = 0;
    res = ioctl(fd, POWER_INTERFACE_STARTUP_READ, &ui_value);
    printf ("read ui_value: %x\n", ui_value);

    if(res < 0){
        printf("ioctl failed.\n");
        return -1;
    }
    
    ui_value = 0;
    res = ioctl(fd, POWER_INTERFACE_STARTUP_READ_R1, &ui_value);
    printf ("read ui_value: %x\n", ui_value);

    if(res < 0){
        printf("ioctl failed.\n");
        return -1;
    }
    */

    close(fd);

    return 0;
}
