#ifndef REC_LOGGER
#define REC_LOGGER

#include <syslog.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>       /* time_t, struct tm, time, localtime, asctime */

#define testmax(x,y)	(x>y) ? x : y;
#define testmin(x,y)	(x<y) ? x : y;


//#define THREAD_SAFE

#ifdef THREAD_SAFE111
 extern pthread_mutex_t	g_logLock;
 #define LOG_LOCK		pthread_mutex_lock(&g_logLock)
 #define LOG_UNLOCK	pthread_mutex_unlock(&g_logLock)
#else
 #define LOG_LOCK
 #define LOG_UNLOCK
#endif


#define MAX_BUF 1000			//whole message, including headers
#define MAX_HEADERS 300         //headers only
#define SIZE_OF_PRORAM_NAME (124)

extern unsigned int gLogLevel;
extern char gMsg[MAX_BUF];
extern const char* gLogLevels[8];

void setLogLevel(unsigned int level);
unsigned int getLogLevel();
void logStart(char* fileName, unsigned int level);
void logStop();
void buildLogMsg(unsigned int level, int offset, const char *fmt,  ...);

#define msgInfo(...) {  \
	LOG_LOCK; \
	if(gLogLevel >= LOG_INFO) { \
		int offset = snprintf(gMsg,MAX_HEADERS, "[%s] [%s:%d] ", gLogLevels[LOG_INFO], \
			__PRETTY_FUNCTION__, __LINE__ ); \
			offset = testmax(0,offset); \
			offset = testmin(MAX_HEADERS,offset); \
			buildLogMsg(LOG_INFO, offset, __VA_ARGS__); \
	} \
	LOG_UNLOCK; \
 }

#define msgErr(...) { \
	LOG_LOCK; \
	if(gLogLevel >= LOG_ERR) { \
		int offset = snprintf(gMsg,MAX_HEADERS, "[%s] [%s:%d] ", gLogLevels[LOG_INFO], \
			__PRETTY_FUNCTION__, __LINE__ ); \
			offset = testmax(0,offset); \
			offset = testmin(MAX_HEADERS,offset); \
			buildLogMsg(LOG_ERR, offset, __VA_ARGS__); \
	} \
	LOG_UNLOCK; \
 }

#define msgDbg(...) { \
	LOG_LOCK; \
	if(gLogLevel >= LOG_DEBUG) { \
		int offset = snprintf(gMsg,MAX_HEADERS, "[%s] [%s:%d] ", gLogLevels[LOG_INFO], \
			__PRETTY_FUNCTION__, __LINE__ ); \
			offset = testmax(0,offset); \
			offset = testmin(MAX_HEADERS,offset); \
			buildLogMsg(LOG_DEBUG, offset, __VA_ARGS__); \
	} \
	LOG_UNLOCK; \
 }

#define msgWarn(...) { \
	LOG_LOCK; \
	if(gLogLevel >= LOG_WARNING) {\
		int offset = snprintf(gMsg,MAX_HEADERS, "[%s] [%s:%d] ", gLogLevels[LOG_INFO], \
			__PRETTY_FUNCTION__, __LINE__ ); \
			offset = testmax(0,offset); \
			offset = testmin(MAX_HEADERS,offset); \
			buildLogMsg(LOG_WARNING, offset, __VA_ARGS__);\
	} \
	LOG_UNLOCK; \
 }

#endif
