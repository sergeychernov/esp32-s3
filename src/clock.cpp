
#include <Arduino.h>
#include "clock.h"
void Clock::getTime() // get time from server
{
    if (millis() > currTime + period)
    {
        currTime = millis();
        if (!getLocalTime(&timeinfo))
        {
            return;
        }

        strftime(timeHour, 3, "%H", &timeinfo);
        strftime(timeMin, 3, "%M", &timeinfo);
        strftime(timeSec, 3, "%S", &timeinfo);
        strftime(timeWeekDay, 10, "%A", &timeinfo);

        strftime(day, 3, "%d", &timeinfo);
        strftime(month, 10, "%B", &timeinfo);
        strftime(year, 5, "%Y", &timeinfo);
    }
    
}

void Clock::tick(void)
{
}

bool Clock::getLocalTime(struct tm *info, uint32_t ms)
{
    uint32_t start = millis();
    time_t now;
    while ((millis() - start) <= ms)
    {
        time(&now);
        localtime_r(&now, info);
        if (info->tm_year > (2016 - 1900))
        {
            return true;
        }
        delay(10);
    }
    return false;
}

void Clock::syncNtp(){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#error "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif