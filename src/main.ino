//Example from: https://github.com/VolosR/PCBDesignClock
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include "time.h"
#include "background.h"
#include "pin_config.h"
#include <ArduinoJson.h>

#include <HTTPClient.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

String lastSummaryUpdate = ":";

const char *ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600*2;         //time zone * 3600 , my time zone is  +1 GTM
const int   daylightOffset_sec = 3600;

int frame = 0;

char timeHour[3];
char timeMin[3];
char timeSec[3];
char day[3];
char month[10];
char year[5];
char timeWeekDay[10];
int dayInWeek;

int brightnesses[8] = {0, 4, 8, 16, 32, 64, 128, 255};
int bright = 3;
int deb = 0;
int deb2 = 0;
int rotation = MONEYBOX_ROTATION;
int LAST_LINE = rotation % 2 ? 20 : 38;

unsigned long currTime = 0;
int period = 900;

int statusCode = 0;
JsonDocument summary;

void setRotation(){
    tft.setRotation(rotation);
    if(rotation%2){
        sprite.createSprite(320, 170);
    } else {
        sprite.createSprite(170, 320);
    }
}

void setup(){
    Serial.begin(115200);
    // Turn on display power
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);

    tft.init();
    tft.invertDisplay(1);
    setRotation();
    sprite.setSwapBytes(true);

    delay(50);
    ledcSetup(0, 10000, 8);
    ledcAttachPin(38, 0);
    ledcWrite(0, brightnesses[bright]);

    pinMode(0, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);

    WiFi.begin(SSID_NAME, SSID_PASSWORD);
}

int configured = 0;
void draw()
{
    if (rotation % 2)
    {
        horizontalDraw();
    }
    else
    {
        verticalDraw();
    }
}

int16_t drawString(String input, uint8_t line, int32_t x = 0, uint8_t size = 1, uint16_t fgcolor = TFT_WHITE)
{
    if (line > ((rotation % 2) ? LAST_LINE * 2 : LAST_LINE))
    {
        Serial.println("return: " + input);
        return 0;
    }

    if ((rotation % 2) && (line > LAST_LINE))
    { // Wrap line to second column for horizontal orientation
        line -= LAST_LINE+1;
        x += 150;
    }
    sprite.setFreeFont(0);
    sprite.setTextSize(size);
    sprite.setTextColor(fgcolor, TFT_BLACK);
    int32_t dx = 2 + x;
    int32_t dy = 2 + line*8;
    return sprite.drawString(input, dx, dy);
}

int16_t drawSummary(uint8_t line = 0, int32_t x = 0){
    if (!summary.isNull()){
        int current = (int) summary["current"];
        int in = (int) summary["in"];
        int diff24 = (int)summary["diff24"];
        const char *currency = summary["currency"];
        int thousands = current / 1000;
        int units = current % 1000;

        int dxIn = drawString(String(in), line, x, 2, TFT_ORANGE);
        int dxAll = drawString(String(current - in >= 0 ? "+" : "") + String(current - in) + "=", line, x + dxIn, 2, current - in > 0 ? TFT_GREEN : TFT_RED);

        int dxThousands = drawString(String(thousands), line + 2, x, 7, TFT_GOLD);
        int dxUnits = drawString(String(units), line + 2, x + dxThousands, 3, TFT_GOLD);
        int dxCurrency = drawString(currency, line + 5, x + dxThousands, 3);
        drawString("24h: " + String(diff24), 9, 0, 2, diff24 > 0 ? TFT_GREEN : TFT_RED);
        return dxThousands + max(dxUnits, dxCurrency);
    }
    return 0;
}

int16_t drawRates(uint8_t line = 0, int32_t x = 0)
{
    if (!summary.isNull())
    {
        JsonObject rates = summary["rates"];
        const char *currency = summary["currency"];
        int i = 0;
        int16_t dxKeys = 0;
        for (JsonObject::iterator it = rates.begin(); it != rates.end(); ++it)
        {
            const char *key = it->key().c_str();

            const double value = it->value();
            if (value && key != currency)
            {
                dxKeys = max(drawString(String(key) + ":", line + 2 * i++, x, 2), dxKeys);
            }
        }
        i = 0;
        for (JsonObject::iterator it = rates.begin(); it != rates.end(); ++it)
        {
            const char *key = it->key().c_str();

            const double value = it->value();
            if (value && key != currency)
            {
                drawString(String(value), line + 2 * i++, x + dxKeys, 2);
            }
        }
    }
    return 0;
}

int16_t drawStatus(uint8_t line = 0, int32_t x = 0)
{
    int dxW = drawString(String(configured ? "W" : "?") + ";", line, x);
    int dxStatus = drawString("s:" + String(statusCode) + ";", line, x + dxW);

    int dxUpdate = drawString("u:" + lastSummaryUpdate + ";", line, x + dxW + dxStatus);
    int dxBrightness = drawString("b:" + String(int(bright * 100 / 7)) + "%;", line, x + dxW + dxStatus + dxUpdate);
    return dxW + dxStatus + dxBrightness + dxUpdate;
}

int16_t drawTime(uint8_t line = 0, int32_t x = 0)
{
    if (currTime)
    {
        int dxTime = drawString(String(timeHour) + ":" + String(timeMin), line, x, 5);
        int dxSec = drawString(timeSec, line + 5, x, 3);
        int dxDay = drawString(String(month) + " " + String(day), line + 5, x + dxSec, 2);
        int dxWeekday = drawString(String(timeWeekDay), line + 7, x + dxSec, 2);
        return max(dxTime, dxSec + max(dxDay, dxWeekday));
    }
    return 0;
}

void verticalDraw()
{
    sprite.fillSprite(TFT_BLACK);
    
    //sprite.pushImage(0, 0, 320, 170, frames[frame]);
    
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);

    drawStatus(LAST_LINE);

    drawTime(27);
    if (!summary.isNull())
    {
        drawSummary();
        drawRates(14);
    }

    sprite.pushSprite(0, 0);
}

void horizontalDraw()
{
        sprite.fillSprite(TFT_BLACK);
        sprite.setTextColor(TFT_WHITE, TFT_BLACK);
        

        drawTime(12);

        sprite.setTextSize(1);

        drawStatus(LAST_LINE + LAST_LINE + 1);

        if (!summary.isNull())
        {
            drawSummary();
            drawRates(LAST_LINE+1);
        }

        // for (int i = 0; i < 40;i++)
        //     drawString(String(i % 10), i, 150);

        sprite.pushSprite(0, 0);
    }

    struct tm timeinfo;

    void getTime() // get time from server
    {
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

    void configure()
    {

        configured = 1;
    }

    void oneMinuteRefresh()
    {
        Serial.println("oneMinuteRefresh");
    }

    JsonDocument getSummary()
    {
        HTTPClient https;
        https.setFollowRedirects(followRedirects_t::HTTPC_STRICT_FOLLOW_REDIRECTS);
        https.begin(MONEYBOX_SUMMARY);

        statusCode = https.GET();
        JsonDocument doc;
        if (statusCode != -1)
        {
            String payload = https.getString();
            DeserializationError error = deserializeJson(doc, payload.c_str());
            if (error)
            {
                doc = summary;
            }
            else
            {
                lastSummaryUpdate = String(timeHour) + ":" + String(timeMin);
            }
        }
        else
        {
            doc = summary;
        }
        https.end();
        return doc;
    }

    void oneHourRefresh()
    {
        summary = getSummary();
    }

    void oneDayRefresh()
    {
        Serial.println("oneDayRefresh");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }

    void tenSecondsRefresh()
    {
    }

    void tenMinutesRefresh()
    {
    }

    int previousTenSeconds = -1;
    int previousMinute = -1;
    int previousHour = -1;
    int previousTenMinutes = -1;
    int previousDay = -1;
    void refresh()
    {
        if (timeinfo.tm_min != previousMinute)
        {
            previousMinute = timeinfo.tm_min;
            oneMinuteRefresh();
        }
        if (timeinfo.tm_sec / 10 != previousTenSeconds)
        {
            previousTenSeconds = timeinfo.tm_sec / 10;
            tenSecondsRefresh();
        }
        if (timeinfo.tm_min / 10 != previousTenMinutes)
        {
            previousTenMinutes = timeinfo.tm_min / 10;
            tenMinutesRefresh();
        }
        if (timeinfo.tm_hour != previousHour)
        {
            previousHour = timeinfo.tm_hour;
            oneHourRefresh();
        }
        if (timeinfo.tm_mday != previousDay)
        {
            previousDay = timeinfo.tm_mday;
            oneDayRefresh();
        }
    }

    void loop()
    {
        if (!configured)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                configure();
                delay(500);
            }
        }
        else
        {
            refresh();
        }

        if (digitalRead(0) == 0)
        { // if left button pressed , change brightness
            if (deb == 0)
            {
                deb = 1;
                bright++;
                if (bright == 8)
                    bright = 0;
                ledcWrite(0, brightnesses[bright]);
            }
        }
        else
            deb = 0;

        if (digitalRead(14) == 0)
        { // if left button pressed , change color
            if (deb2 == 0)
            {
                deb2 = 1;
            }
        }
        else
            deb2 = 0;

        if (millis() > currTime + period)
        {
            currTime = millis();
            getTime();
        }
        draw();
    }

// TFT Pin check
#if PIN_LCD_WR  != TFT_WR || \
    PIN_LCD_RD  != TFT_RD || \
    PIN_LCD_CS    != TFT_CS   || \
    PIN_LCD_DC    != TFT_DC   || \
    PIN_LCD_RES   != TFT_RST  || \
    PIN_LCD_D0   != TFT_D0  || \
    PIN_LCD_D1   != TFT_D1  || \
    PIN_LCD_D2   != TFT_D2  || \
    PIN_LCD_D3   != TFT_D3  || \
    PIN_LCD_D4   != TFT_D4  || \
    PIN_LCD_D5   != TFT_D5  || \
    PIN_LCD_D6   != TFT_D6  || \
    PIN_LCD_D7   != TFT_D7  || \
    PIN_LCD_BL   != TFT_BL  || \
    TFT_BACKLIGHT_ON   != HIGH  || \
    170   != TFT_WIDTH  || \
    320   != TFT_HEIGHT
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif

