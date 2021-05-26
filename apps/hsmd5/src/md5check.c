#include <assert.h>
#include <ctype.h>  // isalpha(), isdigit()
#include <errno.h>  // required by log.h
#include <stddef.h> // offsetof()
#include <stdint.h> // required by udp_sock.h
#include <stdio.h>  // required by log.h
#include <stdlib.h> // exit() - required by libsys - error.h
#include <stdbool.h>
#include <string.h> // memcpy()

#include <fcntl.h>	// open()
#include <unistd.h> // getopt()

#include <ini.h>


/************************************* constants ********************************************/
/* Return codes */
#define ALL_FILES_OK 0
/* 1..MAX_FILES - MD% mismatch in the file number N */
#define FILE_IO_ERROR -1
#define SWU_VERSION_FILE_CORRUPTED -2


#define BUFF_SIZE 1480
#define MAX_FILES 30
#define MD5_LEN 32
#define PATH_MAX_LEN 256

/* Opened package directory */
#define SWU_PACKAGE_DIR "./"

/* Version file location */
#define SWU_VERSION_FILE_NAME "pkg_ver.txt"

#define SWU_VERSION_FILE SWU_PACKAGE_DIR SWU_VERSION_FILE_NAME


/************************************* types ***********************************************/

typedef struct callback_data {
    char const * swu_package_dir;
    unsigned int last_file_processed;
} callback_data_t;

/************************************* globals ********************************************/

//static unsigned int g_last_file_processed = 0;

/**************************************  macros  *********************************************/

/************************************* functions declarations *********************************/


/************************************* functions definitions ***********************************/

static int calc_file_md5(char *file_name, char *md5sum, size_t md5_size)
{
        const char MD5SUM_CMD_FMT[] = "md5sum %s 2>/dev/null";
        char cmd[PATH_MAX_LEN + sizeof(MD5SUM_CMD_FMT)];

        assert(md5_size > MD5_LEN);

        memset(md5sum, 0, md5_size);
        sprintf(cmd, MD5SUM_CMD_FMT, file_name);
        FILE *p = popen(cmd, "r");
        if (p == NULL) {
/* DEBUG \/
            printf("%s: popen %s fail! \n", __FUNCTION__, cmd);
/\~DEBUG */            
            return 0;
        }
        int i;
        for (i = 0; i < MD5_LEN; i++) {
                *md5sum = (char)fgetc(p);
                md5sum++;
        }

        *md5sum = '\0';
        int status = pclose(p);
        if(status) {
/* DEBUG \/
            printf("%s: pclose %s fail! Error:%d\n", __FUNCTION__, cmd, status);
/\~DEBUG */
            return 0;
        }

        return i == MD5_LEN;
}



static int ini_parser_callback (void* arg ,  //Data object, provided by the ini_parse caller 
                                            //.ini file item attributes:
                                const char* section, 
                                const char* name, 
                                const char* value)
{
callback_data_t* callback_data = (callback_data_t*) arg;
    char path[PATH_MAX_LEN+1];  //path is an ASCIIZ string
    char md5sum [MD5_LEN+1];    //MD5 sum is an ASCIIZ string

    /* Calcutale the file full path */
    strncpy(path, callback_data->swu_package_dir, PATH_MAX_LEN);
    path[PATH_MAX_LEN] = '\0';
    size_t remainder = PATH_MAX_LEN - strlen(path);
    strncat(path, name, remainder);
    path[PATH_MAX_LEN] = '\0';

    if(strcmp("files-md5", section) == 0) {
        /* Strip the framing "" from the value string */
        if(value[0] == '"') {
            value++;
        }
/* DEBUG \/
        printf ("%s: Item:%d, path:%s, value:%s\n",
                __FUNCTION__, callback_data->last_file_processed, path, value);
/\~DEBUG */

        int md5_len = calc_file_md5(path, md5sum, sizeof(md5sum));
        if(md5_len <= 0) {
            printf ("Artifact file %s read error\n", path);
            return 0;  /* Artifact File I/O error */
        }

        if(memcmp(value, md5sum, MD5_LEN) != 0) {
            printf ("Artifact file %s MD5 mismatch!\n Should be: %s calculated: %s\n",
                    path, value, md5sum);
            return 0;  /* Artifact File MD5 mismatch */
        }

        callback_data->last_file_processed ++;
/* DEBUG *\/
        printf ("Artifact file :%s OK\n Should be: %s calculated: %s\n",
                path, value, md5sum);
/\~DEBUG  */
        printf (" %s OK\n", path);
    }
	return 1; //Done OK
}


void usage (char *argv_0)
{
	printf("Usage: %s -d \n"
	"\td<dir_name>:\tThe package directory\n",
	argv_0);
}


#ifdef BANNER
const char MD5CHECK_BANNER_MSG[] = \
"/////////////////////////////////////////////////////////\n" \
"// hsMd5 check Version 1.0.0  " __TIMESTAMP__ "\n"\
"/////////////////////////////////////////////////////////\n";
#endif

int main(int argc, char **argv)
{

int status;
int rc;

//static char* dir_name = SWU_PACKAGE_DIR;
static char dir_name   [PATH_MAX_LEN+1];
char version_file_name [PATH_MAX_LEN+1];

callback_data_t callback_data ; //ToDo allocate space if data transfer back from callback is needed


#ifdef BANNER
	puts(DBG_HELLO_MSG);
#endif
	int opt;

	while ((opt = getopt(argc, argv, "d:")) != -1) {
		switch (opt) {
			case 'd':
				strncpy(dir_name, optarg, PATH_MAX_LEN-sizeof(SWU_VERSION_FILE_NAME)-1);//Reserve 1 byte for the separating "/"
                strcat(dir_name, "/"); // Add the separating "/"
/* DEBUG *\
				printf("Working dir: %s\n", dir_name);
/\~DEBUG */
				break;
			default:
				usage (argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}
    /* Calculate the version file name */
    strcpy(version_file_name, dir_name);
    strcat(version_file_name, SWU_VERSION_FILE_NAME);
    callback_data.swu_package_dir = dir_name;
    callback_data.last_file_processed = 0;

/* DEBUG *\
        printf ("Version file :%s \n", version_file_name);
/\~DEBUG */
    rc = ini_parse(version_file_name, ini_parser_callback, &callback_data);
    if(rc == 0) {
        /* All files OK */
        status = ALL_FILES_OK;
    } else if(rc == -1) {
        status = FILE_IO_ERROR;
    } else {
        status = SWU_VERSION_FILE_CORRUPTED;
    }
        return status;
}

