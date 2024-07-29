#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include "clock.h"
#ifndef RENDER_H
#define RENDER_H
class Render
{                        
public:               
    TFT_eSprite &sprite;
    TFT_eSPI &tft;
    int rotation;
    int LAST_LINE;
    int brightnesses[8] = {0, 4, 8, 16, 32, 64, 128, 255};
    int bright = 4;
    Render(TFT_eSPI &tft, TFT_eSprite &sprite);
    void setRotation();
    void init();
    void switchBright();
    int16_t drawStatus(
        bool configured,
        int statusCode,
        String lastSummaryUpdate,
        float voltage,
        uint8_t line = 0,
        int32_t x = 0);
    int16_t drawString(String input, uint8_t line, int32_t x = 0, uint8_t size = 1, uint16_t fgcolor = TFT_WHITE);
    int16_t drawSummary(JsonDocument summary, uint8_t line = 0, int32_t x = 0);
    int16_t drawMain(JsonDocument summary, uint8_t line = 0, int32_t x = 0);
    int16_t drawStats(JsonDocument summary, uint8_t line = 0, int32_t x = 0);
    int16_t drawRates(JsonDocument summary, uint8_t line = 0, int32_t x = 0);
    int16_t drawQRCode(String input, uint8_t dx = 20, uint8_t dy = 20);
    int16_t drawTime(Clock clockHelper, uint8_t line = 0, int32_t x = 0);
};
#endif