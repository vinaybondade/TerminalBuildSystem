
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

    int fd = open("/dev/hisky-misc", O_RDWR|O_CLOEXEC);
    if(fd < 0){
        printf("Failed to open hisky-misc device.\n");
        return -1;
    }

    res = ioctl(fd, POWER_INTERFACE_REBOOT, 1);
    if(res < 0){
        printf("ioctl failed.\n");
        return -1;
    }

    close(fd);

    return 0;
}
