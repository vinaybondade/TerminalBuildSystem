
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/string.h>

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

// RAM Location to load the flash partition data.
#define RAM_ADDR  		0x4000000
#define RF_SETTING_LOAD_ADDR    0x2080000

int set_hiSkyId(config_param_info_t *infoPtr, const char *id);
int set_param(config_param_info_t *infoPtr, const char *id);
int set_rfSettings(config_param_info_t *infoPtr, const char *settings);
int load_data(config_param_info_t *infoPtr);
int set_wfSettings(config_param_info_t *infoPtr, const char *settings);
int set_macAddr(config_param_info_t *infoPtr, const char *val);

static param_tag_t WiFiPasswdTag = { "wifip", "<p>", "</p>"};
static param_tag_t ssidTag = { "ssid", "<s>", "</s>"};
static param_tag_t hiSkyIdTag = { "hiskyid", "<hiSkyId>", "</hiSkyId>"};
static param_tag_t macAddrTag = { "macaddr", "<macaddr>", "</macaddr>"};
static param_tag_t rfSettingsTag = { "rfsettings", "<rfsettings>", "</rfsettings>"};
static param_tag_t wfSettingsTag = { "wfsettings", "<wfsettings>", "</wfsettings>"};

static config_param_info_t paramTable[] = {
    { "hiskyid",     "BLOB A",           (char*)0xE20000,       (char*)RAM_ADDR,      set_hiSkyId,     &hiSkyIdTag,        0x20000 },
    { "wifip",       "BLOB B",           (char*)0xE40000,       (char*)RAM_ADDR,      set_param,       &WiFiPasswdTag,     0x40000 },
    { "ssid",        "BLOB B",           (char*)0xE40000,       (char*)RAM_ADDR,      set_param,       &ssidTag,           0x40000 },
    { "rfsettings",  "BLOB C",           (char*)0xE80000,       (char*)RAM_ADDR,      set_rfSettings,  &rfSettingsTag,     0x40000 },
    { "wfsettings",  "BLOB B",           (char*)0xE40000,       (char*)RAM_ADDR,      set_wfSettings,  &wfSettingsTag,     0x40000 },
    { "macaddr",     "BLOB A",           (char*)0xE20000,       (char*)RAM_ADDR,      set_macAddr,     &macAddrTag,        0x20000 }
};

static size_t strcspn(const char *s, const char *reject)
{
    const char *p;
    const char *r;
    size_t count = 0;

    for (p = s; *p != '\0'; ++p) {

	for (r = reject; *r != '\0'; ++r) {
		if (*p == *r)
			return count;
	}

	++count;
    }

    return count;
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

static char * get_macAddr_from_rfSettings(void)
{
    config_param_info_t *rfsetting_param = NULL;
    int index = 0;
    char *iter1 = NULL;
    char *iter2 = NULL;
    int paramTableSize = sizeof(paramTable)/sizeof(paramTable[0]);

    for(index = 0;index < paramTableSize;index++){
        if(!strcmp(paramTable[index].paramName, rfSettingsTag.paramName)){
            // mac address can be found in rfsettings parition. 
            rfsetting_param = &paramTable[index];
            break;
        }
    }

    if(rfsetting_param != NULL){
        load_data(rfsetting_param);
        iter1 = strstr(rfsetting_param->ramAddr, "mac_address");

        if(iter1 == NULL){
            // Parameter not found.
            return NULL;
        }

        iter2 = iter1 + strlen("mac_address") + 3; // advance for strlen("\":\"") bytes
        iter1 = strstr(iter1, "\"");

        if(iter1 == NULL){
            // End tag not found. something not right. Return failure.
            printf("Invalid mac_address format present\n");
            return NULL;
        }
        *iter1 = '\0';

        return iter2;
    }

    return NULL;
}

int load_data(config_param_info_t *infoPtr)
{
    const int cmd_count = 5;
    cmd_tbl_t *cmdPtr = find_cmd("sf");
    char * const argv_probe[4] = { "sf", "probe", "0", NULL };
	
    if(!cmdPtr){
        printf("Could not failed flash read/write function.\n");
        return 0;
    }

    if (cmdPtr->cmd(cmdPtr, 0, 3, argv_probe) < 0) {
	printf ("hsconf failed - probe\n");
	return 0;
    }

    if(! strcmp (infoPtr->paramName, "hiskyid")) {

        char * const argv_read[6] = { "sf", "read", "0x4000000", "0xE20000", "0x20000", NULL };
        return cmdPtr->cmd(cmdPtr, 0, cmd_count, argv_read);
    }
    else if(! strcmp (infoPtr->paramName, "macaddr")) {

        char * const argv_read[6] = { "sf", "read", "0x4000000", "0xE20000", "0x20000", NULL };
        return cmdPtr->cmd(cmdPtr, 0, cmd_count, argv_read);
    }
    else if(! strcmp (infoPtr->paramName, "ssid")) {

        char * const argv_read[6] = { "sf", "read", "0x4000000", "0xE40000", "0x40000", NULL };
        return cmdPtr->cmd(cmdPtr, 0, cmd_count, argv_read);
    }
    else if(! strcmp (infoPtr->paramName, "wifip")) {

        char * const argv_read[6] = { "sf", "read", "0x4000000", "0xE40000", "0x40000", NULL };
        return cmdPtr->cmd(cmdPtr, 0, cmd_count, argv_read);
    }
    else if(! strcmp (infoPtr->paramName, "rfsettings")) {

        char * const argv_read[6] = { "sf", "read", "0x4000000", "0xE80000", "0x40000", NULL };
        return cmdPtr->cmd(cmdPtr, 0, cmd_count, argv_read);
    }

    return 0;
}

int write_blobA(void)
{    
    cmd_tbl_t *cmdPtr = find_cmd("sf");

    char * const argv_probe[4] = { "sf", "probe", "0", NULL };
    char * const argv_erase[5] = { "sf", "erase", "0xE20000", "0x20000", NULL };
    char * const argv_write[6] = { "sf", "write", "0x4000000", "0xE20000", "0x20000", NULL };

    if(!cmdPtr) {
        printf("Could not find flash read/write function.\n");
        return -1;
    }

    //printf ("%s %s %s\n", argv_probe[0], argv_probe[1], argv_probe[2]);

    if (cmdPtr->cmd(cmdPtr, 0, 3, argv_probe) < 0) {
        printf ("hsconf failed - probe\n");
        return -1;
    }

    //printf ("%s %s %s %s\n", argv_erase[0], argv_erase[1], argv_erase[2], argv_erase[3]);

    if (cmdPtr->cmd(cmdPtr, 0, 4, argv_erase) < 0) {   
        printf ("hsconf failed - erase\n");
        return -1;
    }

    //printf ("%s %s %s %s %s\n", argv_write[0], argv_write[1], argv_write[2], argv_write[3], argv_write[4]);

    if (cmdPtr->cmd(cmdPtr, 0, 5, argv_write) < 0) {
        printf ("hsconf failed - write\n");
        return 1;
    }

    return 0;

}

int set_blobA_param(config_param_info_t *infoPtr, const char *val)
{
    char *iter = NULL;
    char *dataPtr = NULL;
    int status = 0;
    int minSize = 8;

    unsigned char params_array[2048] = {0};
    unsigned char *p_data;

    p_data = params_array;

    // Load the flash partition to ram address

    load_data(infoPtr);
    
    if (!strcmp( infoPtr->paramTag->paramName, paramTable[0].paramTag->paramName)) {
        memcpy (p_data, paramTable[0].paramTag->beginTag, strlen(paramTable[0].paramTag->beginTag));
        p_data+=strlen(paramTable[0].paramTag->beginTag);

        memcpy (p_data, val, strlen(val));
        p_data+=strlen(val);

        memcpy (p_data, paramTable[0].paramTag->endTag, strlen(paramTable[0].paramTag->endTag));
        p_data+=strlen(paramTable[0].paramTag->endTag);
    }
    else {
	    iter = strstr(infoPtr->ramAddr, paramTable[0].paramTag->beginTag);
	    if (iter != NULL) {
            dataPtr = iter + strlen(paramTable[0].paramTag->beginTag);
            iter = strstr(infoPtr->ramAddr, paramTable[0].paramTag->endTag);
            *iter = '\0';

            memcpy (p_data, paramTable[0].paramTag->beginTag, strlen(paramTable[0].paramTag->beginTag));
            p_data+=strlen(paramTable[0].paramTag->beginTag);

            memcpy (p_data, dataPtr, strlen(dataPtr));
            p_data+=strlen(dataPtr);

            memcpy (p_data, paramTable[0].paramTag->endTag, strlen(paramTable[0].paramTag->endTag));
            p_data+=strlen(paramTable[0].paramTag->endTag);
	    }
    }

    if (!strcmp( infoPtr->paramTag->paramName, paramTable[5].paramTag->paramName)) {

        memcpy (p_data, paramTable[5].paramTag->beginTag, strlen(paramTable[5].paramTag->beginTag));
        p_data+=strlen(paramTable[5].paramTag->beginTag);

        memcpy (p_data, val, strlen(val));
        p_data+=strlen(val);

        memcpy (p_data, paramTable[5].paramTag->endTag, strlen(paramTable[5].paramTag->endTag));
        p_data+=strlen(paramTable[5].paramTag->endTag);
    }
    else {

	    iter = strstr(infoPtr->ramAddr, paramTable[5].paramTag->beginTag);

	    if (iter != NULL) {

            dataPtr = iter + strlen(paramTable[5].paramTag->beginTag);
            iter = strstr(infoPtr->ramAddr, paramTable[5].paramTag->endTag);
            *iter = '\0';

            memcpy (p_data, paramTable[5].paramTag->beginTag, strlen(paramTable[5].paramTag->beginTag));
            p_data+=strlen(paramTable[5].paramTag->beginTag);

            memcpy (p_data, dataPtr, strlen(dataPtr));
            p_data+=strlen(dataPtr);

            memcpy (p_data, paramTable[5].paramTag->endTag, strlen(paramTable[5].paramTag->endTag));
            p_data+=strlen(paramTable[5].paramTag->endTag);
            }
    
    }

    *p_data = '\0';

    /////////////////////////////////////////////////////////////////////////

    memset(infoPtr->ramAddr, 0, infoPtr->size);
    memcpy (infoPtr->ramAddr, params_array, strlen(params_array));

    if(write_blobA() != 0){
        printf("Failed to commit to non volatile memory.\n");
        return -1;
    }

    return 0;
}

static int check_macaddr_format(char *val)
{
    const int TOKENS = 5; // : in XX:XX:XX:XX:XX:XX
    int index = 0;
    int len = strlen(val);
    for(index = 0;(index < TOKENS) && ((index*3)+2 < len);index++){
        if(val[(index*3)+2] != ':' )
            return -1;
    }

    return 0;
}

int set_macAddr(config_param_info_t *infoPtr, const char *val)
{
    const int LEN = 17; // 17 bytes as in mac address "AA:BB:CC:DD:EE:FF"
   
    if(strlen(val) != LEN){
        printf("Invalid format\n");
        return 0;
    }

    // Check for valid hiSky Mac address.
    char* macAddr = get_macAddr_from_rfSettings();
    if(!macAddr){
        printf("Mac Address not found\n");
        return 0;
    }

    if(strncmp(macAddr, val, LEN)){
        printf("Invalid mac address\n");
        return 0;
    }


    // check mac address format XX:XX:XX:XX:XX:XX
    if(check_macaddr_format(val) != 0){
        printf("Invalid format.\n");
        return 0;
    }

    if(set_blobA_param(infoPtr, val) != 0){
        printf("Failed to set %s\n", infoPtr->paramName);
        return 0;
    }

    return 0;
}

int set_hiSkyId(config_param_info_t *infoPtr, const char *id)
{
    const int idLen = 14;
    unsigned char hiskyidStr[256];

    if(strlen(id) > idLen){
        printf("Invalid length\n");
        return 0;
    }

    if(set_blobA_param(infoPtr, id) != 0){
        printf("Failed to set %s.\n", infoPtr->paramName);
        return 0;
    }
    
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

static int delete_param_from_memory (config_param_info_t *infoPtr)
{
    char *iter1 = NULL;
    char *iter2 = NULL;

    char StrTemp[2048] = {0};
    char *p_data = StrTemp;

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
    return 0;
}

static int delete_param(config_param_info_t *infoPtr)
{
    cmd_tbl_t *cmdPtr = find_cmd("sf");
    char * const argv_probe[4] = { "sf", "probe", "0", NULL };

    if(!cmdPtr) {
        printf("Could not find flash read/write function.\n");
        return 0;
    }

    if (cmdPtr->cmd(cmdPtr, 0, 3, argv_probe) < 0) {
	printf ("hsconf failed - probe\n");
	return 0;
    }

    if(!strcmp (infoPtr->paramName, "hiskyid")  ||
       !strcmp (infoPtr->paramName, "macaddr")) {  

	load_data (infoPtr);

        if (delete_param_from_memory (infoPtr) >=0 ) {  
            
            char * const argv_probe[4] = { "sf", "probe", "0", NULL };
            char * const argv_erase[5] = { "sf", "erase", "0xE20000", "0x20000", NULL };
            char * const argv_write[6] = { "sf", "write", "0x4000000", "0xE20000", "0x20000", NULL };

	    if(!cmdPtr) {
		printf("Could not find flash read/write function.\n");
		return 0;
	    }

            //printf ("%s %s %s\n", argv_probe[0], argv_probe[1], argv_probe[2]);

            if (cmdPtr->cmd(cmdPtr, 0, 3, argv_probe) < 0) {
                printf ("hsconf failed - probe\n");
                return 0;
            }

            printf ("%s %s %s %s\n", argv_erase[0], argv_erase[1], argv_erase[2], argv_erase[3]);

            if (cmdPtr->cmd(cmdPtr, 0, 4, argv_erase) < 0) {
                printf ("hsconf failed - erase\n");
                return 0;
            }

            //printf ("%s %s %s %s %s\n", argv_write[0], argv_write[1], argv_write[2], argv_write[3], argv_write[4]);

            if (cmdPtr->cmd(cmdPtr, 0, 5, argv_write) < 0) {
                printf ("hsconf failed - write\n");
                return 0;
            }

        }
    }
    else if((! strcmp (infoPtr->paramName, "ssid"))  ||
	    (! strcmp (infoPtr->paramName, "wifip"))) {

	load_data (infoPtr);

	if (delete_param_from_memory (infoPtr) >=0 ) {
	
	    cmd_tbl_t *cmdPtr = find_cmd("sf");

	    char * const argv_probe[4] = { "sf", "probe", "0", NULL };
	    char * const argv_erase[5] = { "sf", "erase", "0xE40000", "0x40000", NULL };
	    char * const argv_write[6] = { "sf", "write", "0x4000000", "0xE40000", "0x40000", NULL };

	    if(!cmdPtr) {
		printf("Could not find flash read/write function.\n");
		return 0;
	    }

	    //printf ("%s %s %s\n", argv_probe[0], argv_probe[1], argv_probe[2]);

	    if (cmdPtr->cmd(cmdPtr, 0, 3, argv_probe) < 0) {
		printf ("hsconf failed - probe\n");
		return 0;
	    }

	    printf ("%s %s %s %s\n", argv_erase[0], argv_erase[1], argv_erase[2], argv_erase[3]);

	    if (cmdPtr->cmd(cmdPtr, 0, 4, argv_erase) < 0) {

		printf ("hsconf failed - erase\n");
		return 0;
	    }

	    //printf ("%s %s %s %s %s\n", argv_write[0], argv_write[1], argv_write[2], argv_write[3], argv_write[4]);

	    if (cmdPtr->cmd(cmdPtr, 0, 5, argv_write) < 0) {

		printf ("hsconf failed - write\n");
		return 0;
	    }

	}
	else {
		printf("%s not found\n", infoPtr->paramName);
	}

    }
   
    return 0;
}

int set_param(config_param_info_t *infoPtr, const char *val)
{
    char *iter = NULL;
    char *dataPtr = NULL;
    int status = 0;
    int minSize = 8;

    unsigned char params_array[2048] = {0};
    unsigned char *p_data;

    p_data = params_array;

    // Validate the Size

    if(strlen(val) < minSize){
	printf("Invalid length - not less than %d\n", minSize);
	return 0;
    }

    // Load the flash partition to ram address

    load_data(infoPtr);
    
    if (!strcmp( infoPtr->paramTag->paramName, paramTable[1].paramTag->paramName)) {

	memcpy (p_data, paramTable[1].paramTag->beginTag, strlen(paramTable[1].paramTag->beginTag));
	p_data+=strlen(paramTable[1].paramTag->beginTag);

	memcpy (p_data, val, strlen(val));
	p_data+=strlen(val);

	memcpy (p_data, paramTable[1].paramTag->endTag, strlen(paramTable[1].paramTag->endTag));
	p_data+=strlen(paramTable[1].paramTag->endTag);

    }
    else {

	    iter = strstr(infoPtr->ramAddr, paramTable[1].paramTag->beginTag);

	    if (iter != NULL) {

		dataPtr = iter + strlen(paramTable[1].paramTag->beginTag);
		iter = strstr(infoPtr->ramAddr, paramTable[1].paramTag->endTag);
		*iter = '\0';

		memcpy (p_data, paramTable[1].paramTag->beginTag, strlen(paramTable[1].paramTag->beginTag));
		p_data+=strlen(paramTable[1].paramTag->beginTag);

		memcpy (p_data, dataPtr, strlen(dataPtr));
		p_data+=strlen(dataPtr);

		memcpy (p_data, paramTable[1].paramTag->endTag, strlen(paramTable[1].paramTag->endTag));
		p_data+=strlen(paramTable[1].paramTag->endTag);

	    }
    }

    if (!strcmp( infoPtr->paramTag->paramName, paramTable[2].paramTag->paramName)) {

	memcpy (p_data, paramTable[2].paramTag->beginTag, strlen(paramTable[2].paramTag->beginTag));
	p_data+=strlen(paramTable[2].paramTag->beginTag);

	memcpy (p_data, val, strlen(val));
	p_data+=strlen(val);

	memcpy (p_data, paramTable[2].paramTag->endTag, strlen(paramTable[2].paramTag->endTag));
	p_data+=strlen(paramTable[2].paramTag->endTag);
    }
    else {

	    iter = strstr(infoPtr->ramAddr, paramTable[2].paramTag->beginTag);

	    if (iter != NULL) {

		dataPtr = iter + strlen(paramTable[2].paramTag->beginTag);
		iter = strstr(infoPtr->ramAddr, paramTable[2].paramTag->endTag);
		*iter = '\0';

		memcpy (p_data, paramTable[2].paramTag->beginTag, strlen(paramTable[2].paramTag->beginTag));
		p_data+=strlen(paramTable[2].paramTag->beginTag);

		memcpy (p_data, dataPtr, strlen(dataPtr));
		p_data+=strlen(dataPtr);

		memcpy (p_data, paramTable[2].paramTag->endTag, strlen(paramTable[2].paramTag->endTag));
		p_data+=strlen(paramTable[2].paramTag->endTag);
	    }
    
    }

    *p_data = '\0';

    /////////////////////////////////////////////////////////////////////////

    memset(infoPtr->ramAddr, 0, infoPtr->size);
    memcpy (infoPtr->ramAddr, params_array, strlen(params_array));

    //////////////////////////////////////////////////////////////
    // We have the data updated, need to burn it to flash.
    //////////////////////////////////////////////////////////////
     
    cmd_tbl_t *cmdPtr = find_cmd("sf");

    char * const argv_probe[4] = { "sf", "probe", "0", NULL };
    char * const argv_erase[5] = { "sf", "erase", "0xE40000", "0x40000", NULL };
    char * const argv_write[6] = { "sf", "write", "0x4000000", "0xE40000", "0x40000", NULL };

    if(!cmdPtr) {
	printf("Could not find flash read/write function.\n");
	return 0;
    }

    //printf ("%s %s %s\n", argv_probe[0], argv_probe[1], argv_probe[2]);

    if (cmdPtr->cmd(cmdPtr, 0, 3, argv_probe) < 0) {
	printf ("hsconf failed - probe\n");
	return 0;
    }

    //printf ("%s %s %s %s\n", argv_erase[0], argv_erase[1], argv_erase[2], argv_erase[3]);

    if (cmdPtr->cmd(cmdPtr, 0, 4, argv_erase) < 0) {

	printf ("hsconf failed - erase\n");
	return 0;
    }

    //printf ("%s %s %s %s %s\n", argv_write[0], argv_write[1], argv_write[2], argv_write[3], argv_write[4]);

    if (cmdPtr->cmd(cmdPtr, 0, 5, argv_write) < 0) {

	printf ("hsconf failed - write\n");
	return 0;
    }

    return 0;
}

static int execute_loady(config_param_info_t *infoPtr)
{
    cmd_tbl_t *cmdPtr = find_cmd("loady");
    char * const argv_loady[4] = { "loady", "0x2080000", NULL };

    if (cmdPtr->cmd(cmdPtr, 0, 2, argv_loady) < 0) {
        printf ("hsconf failed - loady\n");
        return -1;
    }

    return 0;
}

int set_rfSettings(config_param_info_t *infoPtr, const char *settings)
{
    char *iter1 = NULL;
    char *iter2 = NULL;
    char *dataPtr = RF_SETTING_LOAD_ADDR;
    int rfSettinsgSize = 0;
    int maxRfSettingsSize = 0;
    char *hiSkyID = NULL;
    char rfSettingsID[32];
    const int MAX_LEN = 3712;

    // Rf settings file cannot be more than the partition.
    memset(RF_SETTING_LOAD_ADDR, 0, infoPtr->size);

    ///Read the hiskyid
    hiSkyID = get_hiSkyID();

    if(hiSkyID == NULL){

        printf("No hiSkyId found.\n");
        return 0;
    }

    // Load file at RF settings RAM location.
    if(execute_loady(infoPtr) != 0){
        printf("Failed to load rf settings to memory.\n");
        return 0;
    }

    // 2 NULL chars to mark the end of partition

    //maxRfSettingsSize = infoPtr->size - strlen(infoPtr->paramTag->beginTag) - strlen(infoPtr->paramTag->endTag) - 2; 
    maxRfSettingsSize = MAX_LEN;
    rfSettinsgSize = strlen(RF_SETTING_LOAD_ADDR);

    if(maxRfSettingsSize < rfSettinsgSize){
        // Given RF settings size is more than the available size.
        printf("Error rf settings size\n");
        return 0;
    }

    /////////////////////////////////////////////////////////////////////////////

    // Check for hiSKyID in the rfsettings file.

    iter1 = strstr(dataPtr, "system_serial_number");

    if(iter1 == NULL){
        // No hiSKyId found in rfsettings
        printf("No hiSkyId found in given rf settings.\n");
        return 0;
    }

    iter1 += strlen("system_serial_number") + 2; // advance for strlen("\" : \"") bytes
    iter2 = strstr(iter1, ",");

    if(iter2 == NULL){
        // hiSKyId not in correct format in rf settings.
        printf("Invalid format for hiskyId in rf settings.\n");
        return 0;
    }

    memset(rfSettingsID, 0, sizeof(rfSettingsID));
    strncpy(rfSettingsID, iter1, iter2 - iter1);

    if(strcmp(rfSettingsID, hiSkyID)){

        // hiskyId mimatch.
        printf("Invalid hiSKyId in file - Expected: %s Found: %s\n", hiSkyID, rfSettingsID);
        return 0;
    }

    /////////////////////////////////////////////////////////////////////////////

    // Write Blob C     
    cmd_tbl_t *cmdPtr = find_cmd("sf");

    char * const argv_probe[4] = { "sf", "probe", "0", NULL };
    char * const argv_erase[5] = { "sf", "erase", "0xE80000", "0x40000", NULL };
    char * const argv_write[6] = { "sf", "write", "0x2080000", "0xE80000", "0x40000", NULL };
    
    if(!cmdPtr) {
        printf("Could not find flash read/write function.\n");
        return 0;
    }

    memset(infoPtr->ramAddr, 0, infoPtr->size);
    iter1 = infoPtr->ramAddr;
    strcpy(iter1, infoPtr->paramTag->beginTag);

    iter1 += strlen(infoPtr->paramTag->beginTag);
    strcpy(iter1, RF_SETTING_LOAD_ADDR);

    iter1 += rfSettinsgSize;
    strcpy(iter1, infoPtr->paramTag->paramName);

    //printf ("%s %s %s\n", argv_probe[0], argv_probe[1], argv_probe[2]);

    if (cmdPtr->cmd(cmdPtr, 0, 3, argv_probe) < 0) {
        printf ("hsconf failed - probe\n");
        return 0;
    }

    //printf ("%s %s %s %s\n", argv_erase[0], argv_erase[1], argv_erase[2], argv_erase[3]);

    if (cmdPtr->cmd(cmdPtr, 0, 4, argv_erase) < 0) {
        printf ("hsconf failed - erase\n");
        return 0;
    }

    //printf ("%s %s %s %s %s\n", argv_write[0], argv_write[1], argv_write[2], argv_write[3], argv_write[4]);

    if (cmdPtr->cmd(cmdPtr, 0, 5, argv_write) < 0) {
        printf ("hsconf failed - write\n");
        return 0;
    }

    return 0;
}

int set_wfSettings(config_param_info_t *infoPtr, const char *settings)
{
    char *iter1 = NULL;
    char *iter2 = NULL;
    char *dataPtr = RF_SETTING_LOAD_ADDR;
    int rfSettinsgSize = 0;
    int maxRfSettingsSize = 0;
    char *hiSkyID = NULL;
    char wifi_ssid[32];

    ///Read the hiskyid
    hiSkyID = get_hiSkyID();

    if(hiSkyID == NULL){

        printf("No hiSkyId found.\n");
        return 0;
    }
    else {

	if (strlen (hiSkyID) < 13) {

		printf("Invalid hiSkyId found\n");
		return 0;
	}
    }

    sprintf (wifi_ssid, "%sWF", hiSkyID);
   
    set_param (&paramTable[1], "admin123"); // wifi password
    set_param (&paramTable[2], wifi_ssid);  // wifi ssid

    return 0;
}

static int do_hsconf(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
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

    printf("Inalid arguments\n");

    return 0;
}

U_BOOT_CMD(
	hsconf,	3,	1,	do_hsconf,
	"Non Volatile configuration settings managemnet sub-system",
	"set key=value - set `key' to `value' and store in configuration settings\n"
	"hsconf print key     - print value of `key' from configuration settings\n"
	"hsconf delete key    - Remove `key' from configuration settings\n"
);


