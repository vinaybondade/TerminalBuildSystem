#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "../include/armRegFile.h"
#include "../include/arm_ublaze_icd.h"

int send_tpc_to_uBlaze()
{
       char buf[MAX_ARM_MB_DATA] = {0};
       int fd = open("/dev/mtd2", O_RDONLY);

       if(fd == -1) {
              printf ("ERROR: Could not open /dev/mtd2");
              return -1;
       }

       buf[0] = TPC_JSON_FILE;

       if (read(fd, buf+1, sizeof(buf) -1) == -1) {
              printf("ERROR: Read mtd2 failed: %s\n", strerror(errno));
              close(fd);
              return -1;
       }

       int ret = writeMsg2MB(buf, sizeof(buf), SEND_FILE);

       if(ret < 0) {
              printf("ERROR: writeMsg2MB return: %d", ret);
              close(fd);
              return -1;
       }

       close(fd);
       return 0;
}

int main()
{
	printf ("App Initialized ...\n");
	armRegFileInit(0x43c30000, 0x10000);
	send_tpc_to_uBlaze();
	armRegFileCleanup(0x10000);
	printf ("mtd2 content was written successfully\n");
	printf ("App Exit\n");
	return 0;
}


