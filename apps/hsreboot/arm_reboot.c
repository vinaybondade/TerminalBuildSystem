#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>


int main(int argc, char** argv)
{
    reboot(LINUX_REBOOT_CMD_RESTART);
    return 0;
}

