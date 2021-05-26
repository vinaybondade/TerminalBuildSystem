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

#define RF_SETTING_LOAD_ADDR    (char *)(0x2080000)

int set_hiSkyId(config_param_info_t *infoPtr, const char *id);
int set_macAddr(config_param_info_t *infoPtr, const char *id);
int set_param(config_param_info_t *infoPtr, const char *id);
int set_serviceID(config_param_info_t *infoPtr, const char *id);
int set_rfSettings(config_param_info_t *infoPtr, const char *settings);
int load_data(config_param_info_t *infoPtr);
int set_wfSettings(config_param_info_t *infoPtr, const char *settings);

static param_tag_t WiFiPasswdTag = { "wifip", "<p>", "</p>"};
static param_tag_t ssidTag = { "ssid", "<s>", "</s>"};
static param_tag_t satIdxTag = { "sati", "<sati>", "</sati>"};
//static param_tag_t serviceIdTag = { "servid", "<servid>", "</servid>"};
static param_tag_t bleAddrTag = { "ble", "<ble>", "</ble>"};
static param_tag_t gpsLatTag = { "gpslat", "<gpslat>", "</gpslat>"};
static param_tag_t gpsLongTag = { "gpslong", "<gpslong>", "</gpslong>"};
static param_tag_t gpsAltTag = { "gpsalt", "<gpsalt>", "</gpsalt>"};
//static param_tag_t hiSkyIdTag = { "hiskyid", "<hiSkyId>", "</hiSkyId>"};
//static param_tag_t macAddrTag = { "macaddr", "<macaddr>", "</macaddr>"};
//static param_tag_t rfSettingsTag = { "rfsettings", "<rfsettings>", "</rfsettings>"};
//static param_tag_t wfSettingsTag = { "wfsettings", "<wfsettings>", "</wfsettings>"};

#ifndef X86
static config_param_info_t paramTable[] = {

    { "wifip",        CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       &WiFiPasswdTag, INTERNAL_BUFFER_SIZE},
    { "ssid",         CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       &ssidTag,       INTERNAL_BUFFER_SIZE},
    { "sati",         CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       &satIdxTag,     INTERNAL_BUFFER_SIZE},
    { "ble",          CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       &bleAddrTag,    INTERNAL_BUFFER_SIZE},
    { "gpslat",       CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       &gpsLatTag,     INTERNAL_BUFFER_SIZE},
    { "gpslong",      CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       &gpsLongTag,    INTERNAL_BUFFER_SIZE},
    { "gpsalt",       CONFIG_MTD_DEV_1,         BLOBB,       (char*)RAM_ADDR,    set_param,       &gpsAltTag,     INTERNAL_BUFFER_SIZE},

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
    iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->beginTag);

    if(iter == NULL){
        // Parameter not found.
        printf("%s not found\n", infoPtr->paramName);
        return 0;
    }

    dataPtr = iter + strlen(infoPtr->paramTag->beginTag);
    iter = strstr(infoPtr->ramAddr, infoPtr->paramTag->endTag);
    *iter = '\0';
    printf("%s\n", dataPtr);
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

static int delete_param (config_param_info_t *infoPtr)
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
        return 0;
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
        return 0;
    }

    return 0;
} 

int main(int argc, char *const argv[])
{
    int index = 0;
    int paramCount = 0;
    char *keyValPairPtr = NULL;
    char *keyPtr = NULL;
    char *valPtr = NULL;
    const int keyValPairIdx = 2;

    if (argc <= 1) {
	    return -1;
    }

    if(argv[keyValPairIdx] == NULL){
        // printf needs replacement using correct print function of uboot.
        printf("Invalid arguments\n");
        return -1;
    }

    keyValPairPtr = (char *)calloc(strlen(argv[keyValPairIdx]), sizeof(char));
    if(keyValPairPtr == NULL){
        // printf needs replacement using correct print function of uboot.
        printf("Memory allocation failed\n");
        return 0;
    }

    strcpy(keyValPairPtr, argv[keyValPairIdx]);
    keyPtr = keyValPairPtr;

    paramCount = sizeof(paramTable)/sizeof(paramTable[0]);

    if(!strcmp(argv[1], "set")){

        // Get a pointer to "=" location in the key Value pair. Format is Key=Value.
        valPtr = strstr(keyValPairPtr, "=");
        valPtr = (valPtr != 0) ? (valPtr+1) : NULL; // Make the pointer to point after "="

        // Truncate the Key string
        keyPtr[strcspn(keyPtr, "=")] = '\0';

        for(index = 0; index < paramCount; index++){
            if(!strncmp(paramTable[index].paramName, keyPtr, strlen(paramTable[index].paramName))){
                return paramTable[index].set_func(&paramTable[index], valPtr);
            }
        }
    }
    else if(!strcmp(argv[1], "print")){ 
        for(index = 0; index < paramCount; index++){
            if(!strncmp(paramTable[index].paramName, keyPtr, strlen(paramTable[index].paramName))){
                return print_param(&paramTable[index]);
            }
        }
    }
    else if(!strcmp(argv[1], "delete")){
        for(index = 0; index < paramCount; index++){
            if(!strncmp(paramTable[index].paramName, keyPtr, strlen(paramTable[index].paramName))){
                return delete_param(&paramTable[index]);
            }
        }
    }

    printf("Invalid arguments\n");

    return 0;
}
