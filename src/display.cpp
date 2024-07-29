#include "display.h"

Display::Display(TFT_eSPI &tft, TFT_eSprite &sprite, Render &render, Clock &clockHelper) : tft(tft), sprite(sprite), render(render), clockHelper(clockHelper)
{
}
void Display::setWiFiConnected(bool value)
{
    wifiConnected = value;
}
void Display::setVoltage(float value)
{
    voltage = value;
}
void Display::switchMode(){
    mode = !mode;
    frameIndex = 0;
}
void Display::switchFrame(){
    frameIndex++;
    int length = mode == 0 ? (sizeof(frames) / sizeof(*frames)) : (sizeof(settingsFrames) / sizeof(*settingsFrames));
    if (frameIndex == length)
        frameIndex = 0;
}
void Display::draw(){
    sprite.fillSprite(TFT_BLACK);
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    int frame = mode == 0 ? frames[frameIndex] : settingsFrames[frameIndex];
    if (render.rotation % 2)
    {
        switch (frame)
        {
        default:
        case Frames::Default:
            horizontalDrawDefault();
            break;
        case Frames::Rates:
            break;
        }
    }
    else
    {
        switch (frame)
        {
        default:
        case Frames::Main:
            verticalDrawMain();
            break;
        case Frames::Stats:
            verticaldrawStats();
            break;
        }
    }
    sprite.pushSprite(0, 0);
}
void Display::setSummaryCode(int value){
    summaryCode = value;
}
void Display::setSummary(JsonDocument value)
{
    summary = value;
    lastSummaryUpdate = " " + String(clockHelper.timeHour) + ":" + String(clockHelper.timeMin);
    setSummaryCode(200);
}

void Display::drawStatus(){

    int brightnessLength = (sizeof(render.brightnesses) / sizeof(*render.brightnesses));
    render.drawString(String(wifiConnected ? "W" : "!") + String(voltage > 0 ? (" " + String(voltage) + "v") : "") + lastSummaryUpdate + String(summaryCode == 200 ? "" : (" " + String(summaryCode))) + String(" " + String(int((render.bright+1) * 100 / brightnessLength)) + "%"), render.LAST_LINE);
}

void Display::verticalDrawRates()
{

    drawStatus();

    render.drawTime(clockHelper, 27);
    if (!summary.isNull())
    {
        render.drawRates(summary);
    }
    
}

void Display::verticalDrawDefault()
{

    drawStatus();

    render.drawTime(clockHelper, 27);
    if (!summary.isNull())
    {
        render.drawSummary(summary);
    }

}

void Display::verticalDrawMain()
{

    drawStatus();

    if (!summary.isNull())
    {
        render.drawMain(summary);
    }

}

void Display::verticaldrawStats()
{

    drawStatus();
    render.drawTime(clockHelper, 27);
    
    if (!summary.isNull())
    {
        render.drawStats(summary);
    }

}

void Display::verticalDrawSettings()
{
    render.drawQRCode((String("WIFI:S:") + String(APP_NAME) + String(";T:nopass;;;")));
}

void Display::horizontalDrawDefault()
{


    // render.drawStatus(
    //     configured,
    //     statusCode,
    //     lastSummaryUpdate,
    //     LAST_LINE + LAST_LINE + 1);

    if (!summary.isNull())
    {
        render.drawSummary(summary);
        render.drawRates(summary, render.LAST_LINE + 1);
    }

    // for (int i = 0; i < 40;i++)
    //     drawString(String(i % 10), i, 150);

}
