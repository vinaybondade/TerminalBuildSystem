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
#include "cJSON.h"

//#define X86
#define FLASH_BLK_SIZE 0x10000

#define CONFIG_MTD_DEV_0	"/dev/mtd0"
#define CONFIG_MTD_DEV_1	"/dev/mtd1"
#define CONFIG_MTD_DEV_2	"/dev/mtd2"

typedef struct param_tag
{
    char *paramName;
    char *beginTag;
    char *endTag;
}param_tag_t;

typedef struct config_param_info config_param_info_t;
struct config_param_info
{
    char        *paramName;
    char        *partitionName;
    char        *flashAddr;
    char        *ramAddr;
    int         (*set_func)(config_param_info_t *paramInfo, const char *val);
    int         (*del_func)(config_param_info_t *paramInfo);
    param_tag_t *paramTag;
    unsigned long    size;
};

#define INTERNAL_BUFFER_SIZE 8192

// RAM Location to load the flash partition data.
//#define RAM_ADDR  		(char *)(0x4000000)
char RAM_ADDR[INTERNAL_BUFFER_SIZE];

char BLOBA[INTERNAL_BUFFER_SIZE];
char BLOBB[INTERNAL_BUFFER_SIZE];
char BLOBC[INTERNAL_BUFFER_SIZE];

int set_hiSkyId(config_param_info_t *infoPtr, const char *id);
int set_macAddr(config_param_info_t *infoPtr, const char *id);
int set_param(config_param_info_t *infoPtr, const char *id);
int set_serviceID(config_param_info_t *infoPtr, const char *id);
int delete_param (config_param_info_t *infoPtr);
int set_rfSettings(config_param_info_t *infoPtr, const char *settings);
int load_data(config_param_info_t *infoPtr);
int set_wfSettings(config_param_info_t *infoPtr, const char *settings);

static param_tag_t hiSkyIdTag = { "hiskyid", "<hiSkyId>", "</hiSkyId>"};
static param_tag_t WiFiPasswdTag = { "wifip", "<p>", "</p>"};
static param_tag_t ssidTag = { "ssid", "<s>", "</s>"};
static param_tag_t satIdxTag = { "sati", "<i>", "</i>"};
static param_tag_t bleAddrTag = { "ble", "<b>", "</b>"};
static param_tag_t servIdTag = { "servid", "<v>", "</v>"};
static param_tag_t gpsLatTag = { "gpslat", "<gpslat>", "</gpslat>"};
static param_tag_t gpsLongTag = { "gpslong", "<gpslong>", "</gpslong>"};
static param_tag_t gpsAltTag = { "gpsalt", "<gpsalt>", "</gpsalt>"};
static param_tag_t fltMgmtTimingTag = { "fms", "<fms>", "</fms>"};
static param_tag_t rfSettingsTag = { "rfsettings", "<rfsettings>", "</rfsettings>"};
//static param_tag_t hiSkyIdTag = { "hiskyid", "<hiSkyId>", "</hiSkyId>"};
//static param_tag_t macAddrTag = { "macaddr", "<macaddr>", "</macaddr>"};
//static param_tag_t wfSettingsTag = { "wfsettings", "<wfsettings>", "</wfsettings>"};

#ifndef X86
static config_param_info_t paramTable[] = {

    { "hiskyid",      CONFIG_MTD_DEV_0,         BLOBA,       (char*)RAM_ADDR,    NULL,            NULL,                 &hiSkyIdTag,          INTERNAL_BUFFER_SIZE },
    { "wifip",        CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &WiFiPasswdTag,       INTERNAL_BUFFER_SIZE},
    { "ssid",         CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &ssidTag,             INTERNAL_BUFFER_SIZE},
    { "sati",         CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &satIdxTag,           INTERNAL_BUFFER_SIZE},
    { "ble",          CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &bleAddrTag,          INTERNAL_BUFFER_SIZE},
    { "gpslat",       CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &gpsLatTag,           INTERNAL_BUFFER_SIZE},
    { "gpslong",      CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &gpsLongTag,          INTERNAL_BUFFER_SIZE},
    { "gpsalt",       CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &gpsAltTag,           INTERNAL_BUFFER_SIZE},
    { "servid",       CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &servIdTag,           INTERNAL_BUFFER_SIZE},
    { "fms",          CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       delete_param,         &fltMgmtTimingTag,    INTERNAL_BUFFER_SIZE},
    { "rfsettings",   CONFIG_MTD_DEV_2,         BLOBC,       (char*)RAM_ADDR,    set_rfSettings,  NULL,                 &rfSettingsTag,       INTERNAL_BUFFER_SIZE },
};

#else
static config_param_info_t paramTable[] = {
    //{ "hiskyid",   "CONFIG_MTD_DEV_0",           BLOBA,       (char*)RAM_ADDR,      set_hiSkyId,     &hiSkyIdTag,        4096 },
    { "wifip",       "CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_param,       &WiFiPasswdTag,     4096 },
    { "ssid",        "CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_param,       &ssidTag,           4096 },
    //{ "rfsettings","CONFIG_MTD_DEV_2",           BLOBC,       (char*)RAM_ADDR,      set_rfSettings,  &rfSettingsTag,     4096 },
    //{ "wfsettings","CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_wfSettings,  &wfSettingsTag,     4096 },
    { "sati",        "CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_param,       &satIdxTag,         4096 },
    //{ "servid",    "CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_serviceID,   &serviceIdTag,      4096 },
    { "ble",         "CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_param,       &bleAddrTag,        4096 },
    { "gpslat",      "CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_param,       &gpsLatTag,         4096 },
    { "gpslong",     "CONFIG_MTD_DEV_1",           BLOBB,       (char*)RAM_ADDR,      set_param,       &gpsLongTag,        4096 },
    //{ "macaddr",   "CONFIG_MTD_DEV_0",           BLOBA,       (char*)RAM_ADDR,      set_macAddr,     &macAddrTag,        4096 }
};
#endif

int read_config(const char *dev, char *buff, uint32_t buff_size)
{
#ifdef X86
	memcpy(buff, dev, buff_size);
#else
	int fd;

	memset(buff, 0, buff_size);

	if ((fd = open(dev, O_RDWR)) < 0) {
		printf("%s: open config failed. err:%s\n", __func__, strerror(errno));

		return -1;
	}

	if (read(fd, buff, buff_size) < 0) {
		printf("%s: read config failed. err:%s\n", __func__, strerror(errno));
		close(fd);

		return -1;
	}

	close(fd);
#endif	
	return 0;
}

#ifdef X86
int write_config(const char *dev, const char *usr_buff, uint32_t buff_size)
{
    memcpy((void *)dev, usr_buff, buff_size);
#else
int write_config(const char *dev, const char *usr_buff, uint32_t buff_size)
{
	int fd;
	char *buff = NULL;

	erase_info_t ei;

	ei.length = FLASH_BLK_SIZE;   //set the erase block size
	ei.start = 0;

	if ((fd = open(dev, O_RDWR)) < 0) {
		printf("%s: open config failed. err:%s\n", __func__, strerror(errno));
		return -1;
	}

        if (ioctl(fd, MEMERASE, &ei) == -1) {
		printf("%s: err on memerase: %s\n", __func__, strerror(errno));
		close(fd);
		return -1;
	}
	
        if (lseek(fd, 0, SEEK_SET) == -1) {
		printf("%s: err on lseek: %s\n", __func__, strerror(errno));
		close(fd);
		return -1;
	}
	
	buff = calloc(1, FLASH_BLK_SIZE);
	if (NULL == buff) {
		printf("%s: err on write: %s\n", __func__, strerror(errno));
		close(fd);
		return -1;
	}

	memcpy(buff, usr_buff, buff_size);
	
	if (write(fd, buff, FLASH_BLK_SIZE) == -1) {
		printf("%s: err on write: %s\n", __func__, strerror(errno));
		free(buff);
		close(fd);
		return -1;
	}

	close(fd);
#endif
	return 0;
}

int load_data(config_param_info_t *infoPtr)
{
    #ifdef X86
    read_config(infoPtr->flashAddr, infoPtr->ramAddr, infoPtr->size);
    #else
    read_config(infoPtr->partitionName, infoPtr->ramAddr, infoPtr->size);
    #endif
    return 0;
}

int print_param(config_param_info_t *infoPtr)
{
    char *iter = NULL;
    char *dataPtr = NULL;

    memset(infoPtr->ramAddr, 0, infoPtr->size);

    load_data(infoPtr);

    if(strcmp(infoPtr->paramName, rfSettingsTag.paramName)){
        iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->beginTag);

        if(iter == NULL){
            // Parameter not found.
            printf("%s not found\n", infoPtr->paramName);
            return 0;
        }

        dataPtr = iter + strlen(infoPtr->paramTag->beginTag);
        iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->endTag);
        *iter = '\0';
    }
    else{
        dataPtr = infoPtr->ramAddr;
    }
    printf("%s\n", dataPtr);
    return 0;
}

int print_param_file(config_param_info_t *infoPtr, const char *outfile)
{
    char *iter = NULL;
    char *dataPtr = NULL;

    memset(infoPtr->ramAddr, 0, infoPtr->size);

    load_data(infoPtr);
    if(strcmp(infoPtr->paramName, rfSettingsTag.paramName)){
        iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->beginTag);

        if(iter == NULL){
            // Parameter not found.
            printf("%s not found\n", infoPtr->paramName);
            return 0;
        }

        dataPtr = iter + strlen(infoPtr->paramTag->beginTag);
        iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->endTag);
        *iter = '\0';
    }
    else{
        dataPtr = infoPtr->ramAddr;
    }

    // Open outfile
    FILE *outFP = fopen(outfile, "w");
    if(!outFP){
        printf("Cannot write to file. Error - %s.\n", strerror(errno));
        return 0;
    }

    fprintf(outFP, "%s\n", dataPtr);
    fclose(outFP);
    return 0;
}

int write_blob(config_param_info_t *infoPtr)
{    
    #ifdef X86
    write_config(infoPtr->flashAddr, infoPtr->ramAddr, infoPtr->size);
    #else
    write_config(infoPtr->partitionName, infoPtr->ramAddr, infoPtr->size);
    #endif
    return 0;
}

int delete_param (config_param_info_t *infoPtr)
{
    char *iter1 = NULL;
    char *iter2 = NULL;

    char StrTemp[2048] = {0};
    char *p_data = StrTemp;

    // Load the partition of this parameter in to memory.
    load_data(infoPtr);

    iter1 = strstr(infoPtr->ramAddr, infoPtr->paramTag->beginTag);

    if(!iter1){
        printf("%s not found\n", infoPtr->paramName);
        return -1;
    }

    iter2 = strstr(infoPtr->ramAddr, infoPtr->paramTag->endTag);
    iter2 += strlen(infoPtr->paramTag->endTag);

    ///////////////////////////////////////

    memcpy (p_data, infoPtr->ramAddr, iter1 - infoPtr->ramAddr);
    p_data+= (iter1 - infoPtr->ramAddr);
    strcpy(p_data, iter2);

    memset(infoPtr->ramAddr, 0, infoPtr->size);
    strcpy(infoPtr->ramAddr, StrTemp);

    if(write_blob(infoPtr) != 0){
        printf("Failed to commit to non volatile memory.\n");
        return -1;
    }

    return 0;
}

void test_partition(config_param_info_t *infoPtr)
{
    int index = 0;
    const int paramTableSize = sizeof(paramTable)/sizeof(paramTable[0]);

    for(index = 0;index < paramTableSize; index++){
        if(strstr(infoPtr->ramAddr, paramTable[index].paramTag->beginTag))
            return; // Found a valid parameter, partition has been init.
    }

    // Partition hasn't been init. init here.
    memset(infoPtr->ramAddr, 0, infoPtr->size);
}

int set_param(config_param_info_t *infoPtr, const char *val)
{
    char *iter = NULL;

    unsigned char params_array[2048] = {0};
    unsigned char *p_data = NULL;

    memset(params_array, 0, sizeof(params_array));
    p_data = params_array;

    // Load the flash partition to ram address
    load_data(infoPtr);

    // check if partition has been init otherwise init here.
    test_partition(infoPtr);

    // Find the parameter
    iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->beginTag);

    if(iter){
        // Paramter found
        // copy data from BLOB start address till the parameter
        memcpy(p_data, infoPtr->ramAddr, (int)(iter - infoPtr->ramAddr));
        p_data += (int)(iter - infoPtr->ramAddr);

        // Copy the parameter's begin tag.
        memcpy(p_data, infoPtr->paramTag->beginTag, strlen(infoPtr->paramTag->beginTag));
        p_data += strlen(infoPtr->paramTag->beginTag);

        // copy the new value
        memcpy(p_data, val, strlen(val));
        p_data += strlen(val);

        // Now copy the remainig data from the partition
        iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->endTag);
        memcpy(p_data, iter, strlen(iter));
    }
    else{
        // Parameter not found. Add the parameter at the end.
        memcpy(p_data, infoPtr->ramAddr, strlen(infoPtr->ramAddr));
        p_data += strlen(infoPtr->ramAddr);

        // Copy the parameter's begin tag.
        memcpy(p_data, infoPtr->paramTag->beginTag, strlen(infoPtr->paramTag->beginTag));
        p_data += strlen(infoPtr->paramTag->beginTag);

        // copy the new value
        memcpy(p_data, val, strlen(val));
        p_data += strlen(val);

        // Copy parameter's end tag.
        memcpy(p_data, infoPtr->paramTag->endTag, strlen(infoPtr->paramTag->endTag));
    }

    memset(infoPtr->ramAddr, 0, infoPtr->size);
    memcpy (infoPtr->ramAddr, params_array, strlen((char *)params_array));

    if(write_blob(infoPtr) != 0){
        printf("Failed to commit to non volatile memory.\n");
        return -1;
    }

    return 0;
} 

static char * get_hiSkyID(void)
{
    config_param_info_t *hiSkyId_param = NULL;
    int index = 0;
    char *iter1 = NULL;
    char *iter2 = NULL;
    int paramTableSize = sizeof(paramTable)/sizeof(paramTable[0]);

    for(index = 0;index < paramTableSize;index++){
        if(!strcmp(paramTable[index].paramName, hiSkyIdTag.paramName)){
            // Found the paramTable netry for HiSKyID .
            hiSkyId_param = &paramTable[index];
            break;
        }
    }

    if(hiSkyId_param != NULL){
        load_data(hiSkyId_param);
        iter1 = strstr(hiSkyId_param->ramAddr, hiSkyId_param->paramTag->beginTag);

        if(iter1 == NULL){
            // Parameter not found.
            printf("%s not found\n", hiSkyId_param->paramName);
            return NULL;
        }

        iter2 = iter1 + strlen(hiSkyId_param->paramTag->beginTag);
        iter1 = strstr(hiSkyId_param->ramAddr, hiSkyId_param->paramTag->endTag);

        if(iter1 == NULL){
            // End tag not found. something snot right. Retuen failure.
            printf("Invalid hiskyId format present.\n");
            return NULL;
        }
        *iter1 = '\0';

        return iter2;
    }

    return NULL;
}

int load_rfsettings_file(config_param_info_t *infoPtr, const char *settingsFile, size_t *rfFileSize)
{
    FILE *settingsFP = fopen(settingsFile, "rb");
    if(!settingsFP){
        printf("Failed to open RF settings file. Error - %s\n", strerror(errno));
        return -1;
    }

    fseek(settingsFP, 0, SEEK_END);
    *rfFileSize = ftell(settingsFP);
    fseek(settingsFP, 0, SEEK_SET);

    if(fread(infoPtr->ramAddr, 1, *rfFileSize, settingsFP) != *rfFileSize){
        printf("Failed to read full RF Settings File.\n");
        fclose(settingsFP);
        return -1;
    }

    fclose(settingsFP);
    return 0;
}

static size_t get_mtd_size(const char *mtdName)
{
    mtd_info_t mtd_info;
	int fd;

	fd = open(mtdName, O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtdName);
		return -1;
	}

	if(ioctl(fd, MEMGETINFO, &mtd_info)){
		fprintf(stderr, "Could not get MTD device info from %s. Error - %s.\n", mtdName, strerror(errno));
		close(fd);
		return -1;
	}

	return mtd_info.size;
}

static const char *get_id_from_input(const char *fileBuffer)
{
    if(!fileBuffer){
        printf("Invalid buffer.\n");
        return NULL;
    }

    // Check if file is in correct JSON format.
    cJSON *jsonObj = cJSON_Parse(fileBuffer);
    if(!jsonObj){
        printf("RF Settings File not in correct format.\n");
        return NULL;
    }

    // Check if serial number given in the rf settings file matches the terminal's serial number
    cJSON *cellsObj = cJSON_GetObjectItem(jsonObj, "cells");
    if(!cellsObj){
        printf("Failed to find cells.\n");
        return NULL;
    }

    cJSON *cellsIndex0Obj = cJSON_GetArrayItem(cellsObj, 0);
    if(!cellsIndex0Obj){
        printf("Failed to find 0th cell.\n");
        return NULL;
    }

    cJSON *systemInfoObj = cJSON_GetObjectItem(cellsIndex0Obj, "system info");
    if(!systemInfoObj){
        printf("Failed to find systemInfoObj.\n");
        return NULL;
    }

    cJSON *serialNumberObj = cJSON_GetObjectItem(systemInfoObj, "system_serial_number");
    if(!serialNumberObj){
        printf("Serial number not found in given rf settinsg file.\n");
        return NULL;
    }

    const char *serialNumberStr = cJSON_Print(serialNumberObj);
    if(!serialNumberStr){
        printf("Failed to find serial number value in given rf settinsg file.\n");
        return NULL;
    }
    
    // Clean up JSON objects
    cJSON_free(serialNumberObj);
    cJSON_free(systemInfoObj);
    cJSON_free(cellsIndex0Obj);
    cJSON_free(cellsObj);
    cJSON_free(jsonObj);
    
    return serialNumberStr;
}

int set_rfSettings(config_param_info_t *infoPtr, const char *settingsFile)
{
    char *dataPtr = infoPtr->ramAddr;
    int rfSettinsgSize = 0;
    int maxRfSettingsSize = 0;
    char rfSettingsID[32];
    char hiSkyID[32];
    const int MTD_SIZE = get_mtd_size(infoPtr->partitionName);
    size_t rfFileSize = 0;

    // 16K bytes will be treated as buffer bytes to leave while checking size of mtd partition that holds rf_settings
    const int BUFFER_BYTES = 16 * 1024;

    // Rf settings file cannot be more than the partition.
    memset(dataPtr, 0, infoPtr->size);

    // Read the hiskyid
    char *idPtr = get_hiSkyID();
    if(idPtr == NULL){
        printf("No hiSkyId found.\n");
        return -1;
    }
    strcpy(hiSkyID, idPtr);

    // Reset buffer for reuse.
    memset(dataPtr, 0, infoPtr->size);

    // Load file at RF settings RAM location.
    if(load_rfsettings_file(infoPtr, settingsFile, &rfFileSize) != 0){
        printf("Failed to load rf settings to memory.\n");
        return -1;
    }

    // Check that given rf_sttings file doesn't overflow the mtd partition.
    maxRfSettingsSize = MTD_SIZE - BUFFER_BYTES;

    if(maxRfSettingsSize < rfSettinsgSize){
        // Given RF settings size is more than the available size.
        printf("Error rf settings size\n");
        return -1;
    }

    /////////////////////////////////////////////////////////////////////////////

    // Check for hiSKyID in the rfsettings file.
    const char *serialNumberStr = get_id_from_input(infoPtr->ramAddr);
    if(!serialNumberStr){
        printf("Failed to get serila number from RF settings file.\n");
        return -1;
    }
    memset(rfSettingsID, 0, sizeof(rfSettingsID));
    strcpy(rfSettingsID, serialNumberStr);

    if(strcmp(rfSettingsID, hiSkyID)){

        // hiskyId mimatch.
        printf("Invalid hiSKyId in file - Expected: %s Found: %s\n", hiSkyID, rfSettingsID);
        return -1;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Write Blob C 
    if(write_blob(infoPtr) != 0){
        printf("Failed to commit to non volatile memory.\n");
        return -1;
    }       

    return 0;
}

void print_usage(void)
{
    int index = 0;
	printf("hsconf - Non Volatile configuration settings managemnet sub-system.\nUsage:\n");
    printf("hsconf set key=value - set `key' to `value' and store in configuration settings\n");
    printf("hsconf print key     - print value of `key' from configuration settings\n");
    printf("hsconf delete key    - Remove `key' from configuration settings\n");
    printf("Supported Keys - \n");
    for(index = 0;index < sizeof(paramTable)/sizeof(paramTable[0]);index++){
        printf("\t%s\n", paramTable[index].paramName);
    }
}

int main(int argc, char *const argv[])
{
    int index = 0;
    int paramCount = 0;
    char *keyValPairPtr = NULL;
    char *keyPtr = NULL;
    char *valPtr = NULL;
    const int keyValPairIdx = 2;

    if (argc < 3) {
	    print_usage();
	    return -1;
    }

    if(argv[keyValPairIdx] == NULL){
        // printf needs replacement using correct print function of uboot.
        printf("Invalid arguments\n");
	    print_usage();
        return -1;
    }

    setuid(0);
    setgid(0);

    keyValPairPtr = (char *)calloc(strlen(argv[keyValPairIdx]), sizeof(char));
    if(keyValPairPtr == NULL){
        // printf needs replacement using correct print function of uboot.
        printf("Memory allocation failed\n");
        return -1;
    }

    strcpy(keyValPairPtr, argv[keyValPairIdx]);
    keyPtr = keyValPairPtr;

    paramCount = sizeof(paramTable)/sizeof(paramTable[0]);

    if(!strcmp(argv[1], "set")){

        // Get a pointer to "=" location in the key Value pair. Format is Key=Value.
        valPtr = strstr(keyValPairPtr, "=");

        if(!valPtr){
            // No '=' found
            print_usage();
            return -1;
        }
        valPtr = (valPtr != 0) ? (valPtr+1) : NULL; // Make the pointer to point after "="

        // Truncate the Key string
        keyPtr[strcspn(keyPtr, "=")] = '\0';

        for(index = 0; index < paramCount; index++){
            if(!strncmp(paramTable[index].paramName, keyPtr, strlen(paramTable[index].paramName))){
                if(paramTable[index].set_func)
                    return paramTable[index].set_func(&paramTable[index], valPtr);
                else{
                    printf("Set function not supported.\n");
                    return -1;
                }
            }
        }
    }
    else if(!strcmp(argv[1], "print")){ 
        for(index = 0; index < paramCount; index++){
            if(!strncmp(paramTable[index].paramName, keyPtr, strlen(paramTable[index].paramName))){
                if(!argv[3])
                    // Outfile not given
                    return print_param(&paramTable[index]);
                else
                    return print_param_file(&paramTable[index], argv[3]);
            }
        }
    }
    else if(!strcmp(argv[1], "delete")){
        for(index = 0; index < paramCount; index++){
            if(!strncmp(paramTable[index].paramName, keyPtr, strlen(paramTable[index].paramName))){
                if(paramTable[index].del_func)
                    return paramTable[index].del_func(&paramTable[index]);
                else{
                    printf("Delete function not supported.\n");
                    return -1;
                }
            }
        }
    }

    printf("Invalid arguments\n");
    print_usage();

    return -1;
}
