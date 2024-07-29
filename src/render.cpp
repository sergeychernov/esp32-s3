#include <Arduino.h>
#include <TFT_eSPI.h>
#include "render.h"
#include "qrcode.h"
#include "pin_config.h"

Render::Render(TFT_eSPI &tft, TFT_eSprite &sprite) : tft(tft), sprite(sprite)
{
    rotation = MONEYBOX_ROTATION;
    LAST_LINE = rotation % 2 ? 20 : 38;
}

void Render::init(){
    // Turn on display power
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    tft.init();
    tft.invertDisplay(1);
    setRotation();
    sprite.setSwapBytes(true);
    ledcSetup(0, 10000, 8);
    ledcAttachPin(38, 0);
    ledcWrite(0, brightnesses[bright]);
}

void Render::switchBright()
{
    bright++;
    if (bright == (sizeof(brightnesses) / sizeof(*brightnesses)))
        bright = 0;
    ledcWrite(0, brightnesses[bright]);
}

int16_t Render::drawStatus(
    bool configured,
    int statusCode,
    String lastSummaryUpdate,
    float voltage,
    uint8_t line,
    int32_t x)
{
    int dxW = drawString(String(configured ? "W" : "?") + ";", line, x);
    int dxStatus = drawString("s:" + String(statusCode) + ";", line, x + dxW);

    int dxUpdate = drawString("u:" + lastSummaryUpdate + ";", line, x + dxW + dxStatus);
    int dxBrightness = drawString("b:" + String(int(bright * 100 / 7)) + "%;", line, x + dxW + dxStatus + dxUpdate);

    int dxVoltage = drawString("v:" + String(voltage), line, x + dxW + dxStatus + dxUpdate + dxBrightness);
    return dxW + dxStatus + dxBrightness + dxUpdate + dxVoltage;
}

void Render::setRotation(){
    tft.setRotation(rotation);
    if(rotation%2){
        sprite.createSprite(320, 170);
    } else {
        sprite.createSprite(170, 320);
    }
}
int16_t Render::drawString(String input, uint8_t line, int32_t x, uint8_t size, uint16_t fgcolor)
{
    if (line > ((rotation % 2) ? LAST_LINE * 2 : LAST_LINE))
    {
        Serial.println("return: " + input);
        return 0;
    }

    if ((rotation % 2) && (line > LAST_LINE))
    { // Wrap line to second column for horizontal orientation
        line -= LAST_LINE + 1;
        x += 150;
    }
    sprite.setFreeFont(0);
    sprite.setTextSize(size);
    sprite.setTextColor(fgcolor, TFT_BLACK);
    int32_t dx = 2 + x;
    int32_t dy = 2 + line * 8;
    return sprite.drawString(input, dx, dy);
}

int16_t Render::drawSummary(JsonDocument summary, uint8_t line, int32_t x)
{
    if (!summary.isNull())
    {
        int current = (int)summary["current"];
        int in = (int)summary["in"];
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

int16_t Render::drawMain(JsonDocument summary, uint8_t line, int32_t x)
{
    if (!summary.isNull())
    {
        uint8_t stringHeight = 2;
        uint8_t row = line;

        JsonObject rates = summary["rates"];
        JsonObject assets = summary["assets"];

        int current = (int)summary["current"];
        int in = (int)summary["in"];
        const char *currency = summary["currency"];
        JsonObject currencyAmounts = summary["currencyAmounts"];
        int thousands = current / 1000;
        int units = current % 1000;
        int unitsLenght = String(units).length();
        String zeroText = "";

        if(unitsLenght == 1){
            zeroText = "00";
        }else if(unitsLenght == 2){
            zeroText = "0";
        }

        int16_t maxdx = 0;

        int dxThousands = drawString(String(thousands), row, x, 4, TFT_GOLD);
        int dxUnits = drawString(String(units) + zeroText, row, x + dxThousands, 2, TFT_GOLD);
        row += 2;
        int dxCurrency = drawString(currency,row, x + dxThousands, 2);
        row += 2;

        maxdx = dxCurrency;

        for (JsonObject::iterator it = rates.begin(); it != rates.end(); ++it)
        {
            const char *key = it->key().c_str();
            const double value = it->value();
            int16_t dx = 0;

            if (value && assets[key])
            {
                double amount = value * double(assets[key]);

                row += stringHeight;
                dx += drawString(String(key), row, x, stringHeight, TFT_WHITE);
                dx += drawString(String(double(assets[key])), row, x + dx + 2, 2, TFT_GREEN);
                row += stringHeight;
                drawString(String(amount) + String(currency), row, x, 1, TFT_GREEN);
                row += 1;

                maxdx = max(dx, maxdx);
            }
        }
       
        return maxdx;
    }
    return 0;
}

int16_t Render::drawStats(JsonDocument summary, uint8_t line, int32_t x)
{
    if (!summary.isNull())
    {
        uint8_t stringHeight = 4;
        uint8_t row = line;
        int16_t dx1 = 0, dx2 = 0, dx3 = 0;

        int current = (int)summary["current"];

        int diff24h = current - (int)summary["diff24"];
        int diff7d = current -(int)summary["diff7d"];
        int diff30d = current -(int)summary["diff30d"];

        int16_t maxdx = 0;

        dx1 += drawString("24h", row, x, stringHeight, TFT_WHITE);
        row += stringHeight;
        dx1 += drawString((diff24h > 0 ? "+" : "") + String(diff24h), row, x, 3, diff24h >= 0 ? TFT_GREEN : TFT_RED );
        row += stringHeight;
        
        dx2 += drawString("7d", row, x, stringHeight, TFT_WHITE);
        row += stringHeight;
        dx2 += drawString((diff7d > 0 ? "+" : "") + String(diff7d), row, x, 3, diff7d >= 0 ? TFT_GREEN : TFT_RED );
        row += stringHeight;

        dx3 += drawString("30d", row, x, stringHeight, TFT_WHITE);
        row += stringHeight;
        dx3 += drawString((diff30d > 0 ? "+" : "") + String(diff30d), row, x, 3, diff30d >= 0 ? TFT_GREEN : TFT_RED );
        row += stringHeight;
      
        maxdx = max(max(dx1, dx2), max(dx3, maxdx));

        return maxdx;
    }
    return 0;
}

int16_t Render::drawRates(JsonDocument summary, uint8_t line, int32_t x)
{
    int16_t maxdx = 0;
    if (!summary.isNull())
    {
        JsonObject rates = summary["rates"];
        const char *currency = summary["currency"];
        JsonObject assets = summary["assets"];
        int i = 0;
        double sum = 0;

        uint8_t stringHeight = 4;
        for (JsonObject::iterator it = rates.begin(); it != rates.end(); ++it)
        {
            const char *key = it->key().c_str();

            const double value = it->value();
            if (value && assets[key])
            {
                double amount = value * double(assets[key]);
                sum += amount;
                int16_t dx1 = drawString(String(value), line + stringHeight * i, x, 1, TFT_GREEN);
                dx1 += drawString("*", line + stringHeight * i, x + dx1);
                dx1 += drawString(String(double(assets[key])), line + stringHeight * i, x + dx1, 1, TFT_ORANGE);
                dx1 += drawString(String(key)+"=", line + stringHeight * i, x + dx1);
                int16_t dx2 = drawString(it == rates.begin()? "":"+", line + stringHeight * i + 1, x, 2);
                dx2 += drawString(String(amount), line + stringHeight * i + 1, x + dx2, 2, TFT_GOLD);
                dx2 += drawString(String(currency), line + stringHeight * i + 1, x + dx2, 2);
                i++;
                maxdx = max(max(dx1, dx2), maxdx);
            }
        }
        int16_t dx3 = drawString("=", line + stringHeight * i + 1, x, 2);
        dx3 += drawString(String(sum), line + stringHeight * i + 1, x + dx3, 2, TFT_GOLD);
        dx3 += drawString(String(currency), line + stringHeight * i + 1, x + dx3, 2);

        maxdx = max(dx3, maxdx);
    }
    return maxdx;
}

int16_t Render::drawQRCode(String input, uint8_t dx, uint8_t dy)
{

    // Create the QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, input.c_str());
    int rectSize = 4;
    for (uint8_t y = 0; y < qrcode.size; y++)
    {
        for (uint8_t x = 0; x < qrcode.size; x++)
        {
            sprite.fillRect(dx + x * rectSize, dy + y * rectSize, rectSize, rectSize, qrcode_getModule(&qrcode, x, y) ? 0xFFFFFF : 0x0);
        }
    }
    return rectSize * qrcode.size;
}

int16_t Render::drawTime(Clock clockHelper, uint8_t line, int32_t x)
{
    if (clockHelper.currTime)
    {
        int dxTime = drawString(String(clockHelper.timeHour) + ":" + String(clockHelper.timeMin), line, x, 5);
        int dxSec = drawString(clockHelper.timeSec, line + 5, x, 3);
        int dxDay = drawString(String(clockHelper.month) + " " + String(clockHelper.day), line + 5, x + dxSec, 2);
        int dxWeekday = drawString(String(clockHelper.timeWeekDay), line + 7, x + dxSec, 2);
        return max(dxTime, dxSec + max(dxDay, dxWeekday));
    }
    return 0;
}

// TFT Pin check
// #if PIN_LCD_WR  != TFT_WR || \
//     PIN_LCD_RD  != TFT_RD || \
//     PIN_LCD_CS    != TFT_CS   || \
//     PIN_LCD_DC    != TFT_DC   || \
//     PIN_LCD_RES   != TFT_RST  || \
//     PIN_LCD_D0   != TFT_D0  || \
//     PIN_LCD_D1   != TFT_D1  || \
//     PIN_LCD_D2   != TFT_D2  || \
//     PIN_LCD_D3   != TFT_D3  || \
//     PIN_LCD_D4   != TFT_D4  || \
//     PIN_LCD_D5   != TFT_D5  || \
//     PIN_LCD_D6   != TFT_D6  || \
//     PIN_LCD_D7   != TFT_D7  || \
//     PIN_LCD_BL   != TFT_BL  || \
//     TFT_BACKLIGHT_ON   != HIGH  || \
//     170   != TFT_WIDTH  || \
//     320   != TFT_HEIGHT
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
// #endif