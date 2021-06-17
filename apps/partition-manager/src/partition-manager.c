#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define CMD_CREATE_FIRMWARE_PARTITION       "--create-firmware-partition"
#define CMD_CREATE_PARMETERS_PARTITION      "--create-parameters-partition"
#define CMD_QUICKFORMAT_EMMC                "--quick-format-emmc"
#define CMD_DELETE_PARTITION                "--delete-partition"
#define CMD_SAVE_MBR                        "--save-mbr"
#define CMD_RESTORE_MBR                     "--restore-mbr"
#define CMD_SHOW_PARTITIONS                 "--show-partitions"
#define FIRMWARE_PARTITION                  "/dev/mmcblk0p1"
#define RCPFILE_PARTITION                   "/dev/mmcblk0p2"
#define EMMC_BLK_DEVICE                     "/dev/mmcblk0"

#define EMMC_SIZE                           3850371072 // 3672 MB
#define SECTOR_SIZE                         512
#define EMMC_END                            3670 // We leave some space (2MB) at the end of emmc unused, we keep MBR backup here


int cmd_create_firmware_partition(int argc, char*argv[])
{
    char cmdParams[10][256] = { "parted", EMMC_BLK_DEVICE, "-s", "-a", "none", "mkpart", "primary", "ext4", "8192B", "0" };
    char cmd[256];
    const uint32_t DEFAULT_PARTITION_SIZE = 300; //300 Mb
    const uint32_t MIN_PARTITION_SIZE = 260; //300 Mb

    memset(cmdParams[9], 0, sizeof(cmdParams[9]));
    if(argc >= 3){
        // We have size given
        // Check for valid size
        if(strtol(argv[2], NULL, 10) < MIN_PARTITION_SIZE){
            printf("Minimum Partition size should be %d.\n", MIN_PARTITION_SIZE);
            return -1;
        }

        strncpy(cmdParams[9], argv[2], strlen(argv[2]));
    }
    else
        sprintf(cmdParams[9], "%d", DEFAULT_PARTITION_SIZE); 

    // Check if the size goes beyong the allowed boundary   
    unsigned int endAddr = strtol(cmdParams[10], NULL, 10);
    if(!endAddr){
        printf("Invalid size.\n");
        return -1;
    }

    if(endAddr >= EMMC_END){
        printf("Given size is beyond allowed size. Check the partitions' size.\n");
        return -1;
    }

    sprintf(cmd, "%s %s %s %s %s %s %s %s %s %s", cmdParams[0], cmdParams[1], cmdParams[2], cmdParams[3],
     cmdParams[4], cmdParams[5], cmdParams[6], cmdParams[7], cmdParams[8], cmdParams[9]);

    if(system(cmd) != 0){
        printf("Creating Firmware partition failed.\n");
        return -1;
    }

    // Initialise the file system
    sprintf(cmd, "mkfs.ext4 %s", FIRMWARE_PARTITION);

    if(system(cmd) != 0){
        printf("Initialising file system on Firmware partition failed.\n");
        return -1;
    }

    return 0;   
}

int get_firmware_partition_boundary(char *boundary, size_t len)
{
    FILE *fp;
    char cmd[256];

    sprintf(cmd, "parted %s print | awk \'$1+0\' | awk \'{ print $3 }\'", FIRMWARE_PARTITION);
    printf("popen cmd - %s\n", cmd);

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        return -1;
    }

    /* Read the output a line at a time - output it. */
    // Firmware partition is the first partition
    if(fgets(boundary, len, fp) != NULL) {
        printf("%s", boundary);
    }

    /* close */
    pclose(fp);
    return 0;
}

int cmd_create_parameters_partition(int argc, char*argv[])
{
    char cmdParams[10][256] = { "parted", EMMC_BLK_DEVICE, "-s", "-a", "none", "mkpart", "primary", "ext4", "0", "0"};
    const uint32_t PARTITION_SIZE = 260; // 300 MB
    const uint32_t DEFAULT_PARTITION_SIZE = 300; // 2Gigabytes
    const uint32_t DEFAULT_PARTITION_BUFFER_BYTES = 300; // 260 MB
    char cmd[256];
    char size[32];

    // Get the end address of firmware partition
    char firmwarePartBoundary[256];
    if(get_firmware_partition_boundary(firmwarePartBoundary, sizeof(firmwarePartBoundary)) != 0){
        printf("Can't find Firmware partition boundary. Please check if Firmware partition exists.\n");
        return -1;
    }

    printf("Firmware partition boundary - %s\n", firmwarePartBoundary);

    // Form the command
    memset(size, 0, sizeof(size));

    if(argc >= 3){
        // We have size given
        // Check for valid size
        if(strtol(argv[2], NULL, 10) < PARTITION_SIZE){
            printf("Minimum Partition size should be %d.\n", PARTITION_SIZE);
            return -1;
        }
        strncpy(size, argv[2], strlen(argv[2]));
    }
    else
        sprintf(size, "%d", DEFAULT_PARTITION_SIZE);

    // Calculate the start and end.
    uint32_t startAddr = 0;
    uint32_t endAddr = 0;
    sscanf(firmwarePartBoundary, "%d", &startAddr);

    // We need to keep buffer of 260 bytes between firmware partition and ext2 partition
    startAddr += DEFAULT_PARTITION_BUFFER_BYTES;
    printf("startAddr - %d\n", startAddr);
    endAddr = startAddr + strtol(size, NULL, 10);
    printf("endAddr - %d\n", endAddr); 

    // Check if the size goes beyong the allowed boundary   
    if(endAddr >= EMMC_END){
        printf("Given size is beyond allowed size. Check the partitions' size.\n");
        return -1;
    }

    sprintf(cmd, "%s %s %s %s %s %s %s %s %d %d", cmdParams[0], cmdParams[1], cmdParams[2], cmdParams[3],
     cmdParams[4], cmdParams[5], cmdParams[6], cmdParams[7], startAddr, endAddr);

    if(system(cmd) != 0){
        printf("Creating RCP file partition failed.\n");
        return -1;
    }

    // Initialise the file system
    sprintf(cmd, "mkfs.ext4 %s -F", RCPFILE_PARTITION);

    if(system(cmd) != 0){
        printf("Initialising file system on RCP File partition failed.\n");
        return -1;
    }

    return 0;
}

int cmd_delete_partition(int argc, char*argv[])
{
    char cmdParams[6][256] = { "parted", EMMC_BLK_DEVICE, "-a", "none", "rm", "2"};

    uint8_t partitionIdx = 0;
    char cmd[256];

    if(argc != 3){
        printf("Invalid parameters.\n");
        return -1;
    }
    partitionIdx = strtol(argv[2], NULL, 10);

    sprintf(cmd, "%s %s %s %s %s %d", cmdParams[0], cmdParams[1], cmdParams[2], cmdParams[3],
     cmdParams[4], partitionIdx);

    if(system(cmd) != 0){
        printf("EMMC quick format failed.\n");
        return -1;
    }

    return 0;
}

int cmd_quick_format_emmc(int argc, char*argv[])
{
    char cmd[256];
    sprintf(cmd, "dd if=/dev/zero of=%s bs=1k count=2048", EMMC_BLK_DEVICE);

    if(system(cmd) != 0){
        printf("EMMC quick format failed.\n");
        return -1;
    }

    return 0;
}

int cmd_save_mbr(int argc, char*argv[])
{
    const char *binFile = "/tmp/mbr.bin";
    char cmd1[255];
    sprintf(cmd1, "dd if=%s of=%s bs=512 count=2", EMMC_BLK_DEVICE, binFile);

    if(system(cmd1) != 0){
        printf("Failed to read MBR.\n");
        return -1;
    }

    char cmd2[255];
    sprintf(cmd2, "dd if=%s of=%s bs=512 count=2 seek=%lld", binFile, EMMC_BLK_DEVICE, (EMMC_SIZE/SECTOR_SIZE)-2);

    if(system(cmd2) != 0){
        printf("Failed to backup MBR.\n");
        return -1;
    }

    return 0;
}

int cmd_restore_mbr(int argc, char*argv[])
{
    const char *binFile = "/tmp/mbr.bin";
    char cmd1[255];
    sprintf(cmd1, "dd if=%s of=%s bs=512 count=2 skip=%lld", EMMC_BLK_DEVICE, binFile, (EMMC_SIZE/SECTOR_SIZE)-2);

    if(system(cmd1) != 0){
        printf("Failed to backed MBR.\n");
        return -1;
    }

    char cmd2[255];
    sprintf(cmd2, "dd if=%s of=%s bs=512 count=2", binFile, EMMC_BLK_DEVICE);

    if(system(cmd2) != 0){
        printf("Failed to restore MBR.\n");
        return -1;
    }

    return 0;
}

int cmd_show_partitions(int argc, char*argv[])
{
    char cmd[256];
    sprintf(cmd, "parted %s print", EMMC_BLK_DEVICE);

    if(system(cmd) != 0){
        printf("EMMC print partitions command failed.\n");
        return -1;
    }

    // Perform label init
    sprintf(cmd, "parted "EMMC_BLK_DEVICE" -s -a none mklabel msdos");

    if(system(cmd) != 0){
        printf("EMMC setting label failed.\n");
        return -1;
    }
    sprintf(cmd, "parted "EMMC_BLK_DEVICE" -s -a none mklabel msdos");

    if(system(cmd) != 0){
        printf("EMMC setting label failed.\n");
        return -1;
    }

    return 0;
}

void print_usage()
{
    printf("usage: partition-manager [args] \n");
    printf("                 %s [size in MB]\n", CMD_CREATE_FIRMWARE_PARTITION);
    printf("                 %s [size in MB]\n", CMD_CREATE_PARMETERS_PARTITION);
    printf("                 %s [Partition Index 1/2]\n", CMD_DELETE_PARTITION);
    printf("                 %s\n", CMD_SAVE_MBR);
    printf("                 %s\n", CMD_RESTORE_MBR);
    printf("                 %s\n", CMD_SHOW_PARTITIONS);
    printf("                 %s\n", CMD_QUICKFORMAT_EMMC);
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        print_usage();
        return -1;
    }

    if(!strncmp(CMD_CREATE_FIRMWARE_PARTITION, argv[1], strlen(CMD_CREATE_FIRMWARE_PARTITION))){
        return cmd_create_firmware_partition(argc, argv);
    }
    else if(!strncmp(CMD_CREATE_PARMETERS_PARTITION, argv[1], strlen(CMD_CREATE_PARMETERS_PARTITION))){
        return cmd_create_parameters_partition(argc, argv);
    }
    else if(!strncmp(CMD_DELETE_PARTITION, argv[1], strlen(CMD_DELETE_PARTITION))){
        return cmd_delete_partition(argc, argv);
    }
    else if(!strncmp(CMD_QUICKFORMAT_EMMC, argv[1], strlen(CMD_QUICKFORMAT_EMMC))){
        return cmd_quick_format_emmc(argc, argv);
    }
    else if(!strncmp(CMD_SAVE_MBR, argv[1], strlen(CMD_SAVE_MBR))){
        return cmd_save_mbr(argc, argv);
    }
    else if(!strncmp(CMD_RESTORE_MBR, argv[1], strlen(CMD_RESTORE_MBR))){
        return cmd_restore_mbr(argc, argv);
    }
    else if(!strncmp(CMD_SHOW_PARTITIONS, argv[1], strlen(CMD_SHOW_PARTITIONS))){
        return cmd_show_partitions(argc, argv);
    }
    else{
        print_usage();
        return -1;
    }

    return 0;
}