#include "time.h"
#ifndef Clock_H
#define Clock_H
class Clock
{                       
public:                  
    struct tm timeinfo;
    char timeHour[3];
    char timeMin[3];
    char timeSec[3];
    char day[3];
    char month[10];
    char year[5];
    char timeWeekDay[10];
    int dayInWeek;
    void getTime();
    
    void syncNtp();
    const char *ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 3600 * 2; // time zone * 3600 , my time zone is  +1 GTM
    const int daylightOffset_sec = 3600;
    unsigned long currTime = 0;
    int period = 900;
    void tick(void);

private:
    bool getLocalTime(struct tm *info, uint32_t ms = 0);
};
#endif