#include <TFT_eSPI.h>
#include "render.h"
#include <ArduinoJson.h>
#ifndef DISPLAY_H
#define DISPLAY_H

enum Frames
{
    Default,
    Rates,
    Settings
};
class Display
{                      
public:                 
    void setSummary(JsonDocument value);
    void setSummaryCode(int value);
    Display(TFT_eSPI &tft, TFT_eSprite &sprite, Render &render, Clock &clockHelper);
    void switchFrame();
    void switchMode();
    void draw();
    void setWiFiConnected(bool value = true);
    void setVoltage(float value);

private:
    TFT_eSprite &sprite;
    TFT_eSPI &tft;
    bool wifiConnected = false;
    float voltage = 0;
    String lastSummaryUpdate = "";
    int frames[2] = {Frames::Default, Frames::Rates};
    int settingsFrames[1] = {Frames::Settings};
    int frameIndex = 0;
    int mode = 0;
    int summaryCode = -1;
    Render &render;
    JsonDocument summary;
    Clock &clockHelper;
    void horizontalDrawDefault();
    void verticalDrawDefault();
    void verticalDrawRates();
    void verticalDrawSettings();
    void drawStatus();
};
#endif