#include "clib_syslog.h"

static char sysCategory[256] = {0};
static int sysFacility = 0;

#ifdef LOG_LEVEL
static int logLevel = LOG_DEBUG;
#else
static int logLevel = LOG_INFO;
#endif

void syslog_init(const char *category, int loglevel, int facility)
{
    if (NULL == category) {
        return;
    }

    if (loglevel >= 0)
        logLevel = loglevel;

    memset(sysCategory, 0, sizeof sysCategory);
    strncpy(sysCategory, category, sizeof sysCategory - 1);
    sysFacility = facility;
}

void syslog_info(int mlogLevel, const char *fileName, const char *functionName, int line, const char* fmt, ...)
{
    if (mlogLevel > logLevel) {
        return;
    }

    va_list         para;
    unsigned long   tagLen = 0;
    char            buf[2048] = {0};
    char*           currentProj = NULL;
    char*           logLevelstr = NULL;

    va_start(para, fmt);

    currentProj = strstr(fileName, "filesystem/");
    if (NULL != currentProj) {
        fileName = currentProj + 11;
    }

    memset(buf, 0, sizeof buf);

    openlog("", LOG_NDELAY, sysFacility);
    switch (mlogLevel) {
    case LOG_EMERG:
        logLevelstr = "EMERG";
        break;
    case LOG_ALERT:
        logLevelstr = "ALERT";
        break;
    case LOG_CRIT:
        logLevelstr = "CRIT";
        break;
    case LOG_ERR:
        logLevelstr = "ERROR";
        break;
    case LOG_WARNING:
        logLevelstr = "WARNING";
        break;
    case LOG_NOTICE:
        logLevelstr = "NOTICE";
        break;
    case LOG_INFO:
        logLevelstr = "INFO";
        break;
    case LOG_DEBUG:
        logLevelstr = "DEBUG";
        break;
    default:
        logLevelstr = "UNKNOWN";

    }
    snprintf(buf, sizeof buf - 1, "%s [%s] %s %s line:%-5d ", logLevelstr, sysCategory, fileName, functionName, line);
    tagLen = strlen(buf);
    vsnprintf(buf + tagLen, sizeof buf - 1 - tagLen, (const char*)fmt, para);
    syslog(mlogLevel, "%s", buf);
    closelog();
    va_end(para);
}

