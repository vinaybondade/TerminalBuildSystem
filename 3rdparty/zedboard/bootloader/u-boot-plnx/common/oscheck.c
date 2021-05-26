#include <common.h>
#include <command.h>
#include <part.h>
#include <fat.h>
#include <fs.h>
#include <linux/string.h>
#include <u-boot/md5.h>

#define MD5SUM_SIZE     16

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the size of a file.
// Returns ulong value of file size or 0 on errors.
///////////////////////////////////////////////////////////////////////////////////////////////////

static ulong file_size(char *filename)
{
    // Find the command pointer for 'fatsize' command.
    cmd_tbl_t *cmdPtr = find_cmd("fatsize");
    char * const fatsize_cmd[6] = { "fatsize", "mmc", "0:1", filename , NULL };
    ulong size = 0;

    // command pointer sanity check
    if(!cmdPtr){
        printf("Failed to find fatsize command.\n");
        return 0;
    }

    // Execute 'fatsize' command
    if (cmdPtr->cmd(cmdPtr, 0, 4, fatsize_cmd) < 0) {
        printf ("Failed to get size of %s file.\n", filename);
        return 0;
    }

    // Convert from Ascii to hex value. 'fatsize' command stores the filesize as env variable.
    size = getenv_hex("filesize", 0);

    return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function to convert Ascii MD5 value to hex
///////////////////////////////////////////////////////////////////////////////////////////////////

static int hex_to_val(const char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';  // Simple ASCII arithmetic
    else if (ch >= 'a' && ch <= 'f')
        return 10 + ch - 'a';  // Because hex-digit a is ten 
    else if (ch >= 'A' && ch <= 'F')
        return 10 + ch - 'A';  // Because hex-digit A is ten
    else
        return -1;  // Not a valid hexadecimal digit
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function to compare md5sum value of a given file with expected md5sum file
///////////////////////////////////////////////////////////////////////////////////////////////////

int md5_check(char *filename, char* md5_filename, char *load_addr)
{
    char * const fatload_image_cmd[6] = { "fatload", "mmc", "0:1", load_addr, filename, NULL };
    char * const fatload_md5_cmd[6] = { "fatload", "mmc", "0:1", "0x2040000", md5_filename, NULL };
    unsigned char md5_actual[MD5SUM_SIZE];
    unsigned char md5_expected[MD5SUM_SIZE];
    cmd_tbl_t *cmdPtr = NULL;
    ulong  fileSize = file_size(filename);
    unsigned char *loadAddr = NULL;//0x2080000;
    unsigned char *md5_loadAddr = (unsigned char *)0x2040000;
    int i = 0;
    int j = 0;

    // Get command pointer for 'fatload' command.
    cmdPtr = find_cmd("fatload");
    if(!cmdPtr){
        printf("Failed to find fatload command.\n");
        return -1;
    }

    // Sanity check on file size.
    if(!fileSize){
        printf("Invalid filesize.\n");
        return -1;
    }

    // Load the desired file to memory to compute md5sum
    if (cmdPtr->cmd(cmdPtr, 0, 5, fatload_image_cmd) < 0) {
        printf ("Failed to load %d.\n", filename);
        return -1;
    }

    memset(md5_actual, 0, MD5SUM_SIZE);
    memset(md5_expected, 0, MD5SUM_SIZE);

    // Find the md5 value of Kernel image.
    loadAddr = (unsigned char *)simple_strtol(load_addr + 2, NULL, 16);
    md5(loadAddr, fileSize, md5_actual);

    // verify the calculated md5value.
    // Load md5 file to RAM
    if (cmdPtr->cmd(cmdPtr, 0, 5, fatload_md5_cmd) < 0) {
        printf ("Failed to load md5 file.\n", md5_filename);
        return -1;
    }

    // Get the expected md5sum value
    for(i = 0, j = 0;i < MD5SUM_SIZE*2; i += 2, j++){
		
        int digit1 = hex_to_val(md5_loadAddr[i]);      
        int digit2 = hex_to_val(md5_loadAddr[i + 1]);  

        if (digit1 == -1 || digit2 == -1){
            printf("invalid md5 file - %s.\n", md5_filename);
            return -1;
        }

        md5_expected[j] = (char) (digit1 * 16 + digit2);
    }

    if(memcmp(md5_expected, md5_actual, MD5SUM_SIZE) != 0){
        printf("%s failed md5 check.\n", filename);
        return -1;
    }

    printf("MD5 Check successful for %s\n", filename);
    return 0;
}

int do_oscheck(void)
{
    const char *kernelImage = "zImage";
    const char *kernelImage_md5 = "zImage.md5";
    const char *kernelImage_loadAddr = "0x2080000";
    const char *dtbImage = "zynq-zed.dtb";
    const char *dtbImage_md5 = "zynq-zed.md5";
    const char *dtbImage_loadAddr = "0x2000000";
    const char *rootfsImage = "zedboard-ramdisk.img";
    const char *rootfsImage_md5 = "zedboard-ramdisk.md5";
    const char *rootfsImage_loadAddr = "0x4000000";

    if(md5_check(kernelImage, kernelImage_md5, kernelImage_loadAddr) == 0){
        // zImage passed md5 check, check device tree
        if(md5_check(dtbImage, dtbImage_md5, dtbImage_loadAddr) == 0){
            // device tree passed md5 check, check rootfs
            if(md5_check(rootfsImage, rootfsImage_md5, rootfsImage_loadAddr) == 0){
                // OS passed md5 check
                return 0;
            }

            printf("md5 check failed for rootfs.\n Starting secure bootloader.\n");
            return -1;
        }

        printf("md5 check failed for device tree.\n Starting secure bootloader.\n");
        return -1;
    }

    printf("md5 check failed for kernel image.\n Starting secure bootloader.\n");
    return -1;
}