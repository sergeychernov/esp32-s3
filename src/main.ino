#include <FS.h> //this needs to be first, or it all crashes and burns…
#include <Arduino.h>
#include <OneButton.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiManager.h> 
#include "time.h"
#include "background.h"
#include "pin_config.h"
#include <ArduinoJson.h>

#include <HTTPClient.h>

#include <SPIFFS.h>

#include "display.h"
#include "render.h"

#include "clock.h"
#include "CronAlarms.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
Render render(tft, sprite);

Clock clockHelper;
Display display(tft, sprite, render, clockHelper);

WiFiManager wifiManager;
OneButton displayButton = OneButton(
    PIN_BUTTON_2, // Input pin for the button
    true,       // Button is active LOW
    true        // Enable internal pull-up resistor
);
OneButton selectButton = OneButton(
    PIN_BUTTON_1, // Input pin for the button
    true,         // Button is active LOW
    true          // Enable internal pull-up resistor
);


int frame = 0;

int deb = 0;
int deb2 = 0;
int rotation = MONEYBOX_ROTATION;
int LAST_LINE = rotation % 2 ? 20 : 38;

void setup(){
    Serial.begin(115200);
    Cron.delay(500);

    render.init();
    displayButton.attachClick([](){
        Serial.println("Display One Pressed!");
        display.switchFrame();
    });
    displayButton.attachLongPressStop([](){
        Serial.println("Display Long Pressed!");
        display.switchMode();
    });

    selectButton.attachDoubleClick([]()
                                   { 
        Serial.println("Display Double Pressed!");
        render.switchBright(); });
    selectButton.attachClick([]()
                              {
        Serial.println("Display One Pressed!");
        render.switchBright(); });
    selectButton.attachLongPressStop([]()
                                     {
        Serial.println("Display Long Pressed!");
        render.switchBright(); });

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    File file = SPIFFS.open("/wifi2.json");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }
    if (file.available()){
        while (file.available())
        {
            Serial.write(file.read());
        }
    }else{
        file.close();
        file = SPIFFS.open("/wifi2.json", "w");
        JsonDocument doc;
        doc["ssid"] = "ssid";
        doc["password"] = "password";
        String output;
        serializeJson(doc, output);
        int bytesWritten = file.print(output);
        Serial.println("bytesWritten: " + String(bytesWritten));
        file.close();
    }

    Serial.println("File Content:" + String(file.available()));

        
        //wifiManager.startConfigPortal(APP_NAME);
    WiFi.begin(SSID_NAME, SSID_PASSWORD);
}

int configured = 0;

    void updateSummary()
    {
        HTTPClient https;
        https.setFollowRedirects(followRedirects_t::HTTPC_STRICT_FOLLOW_REDIRECTS);
        https.begin(MONEYBOX_SUMMARY);

        int statusCode = https.GET();
        JsonDocument doc;
        if (statusCode == 200)
        {
            String payload = https.getString();
            DeserializationError error = deserializeJson(doc, payload.c_str());
            if (error)
            {
                display.setSummaryCode(-1);
            }
            else
            {
                display.setSummary(doc);
            }
        }
        else
        {
            display.setSummaryCode(statusCode);
        }
        https.end();
    }

    void configure()
    {
        configured = 1;
        clockHelper.syncNtp();
        clockHelper.getTime();
        updateSummary();
        

        Cron.create("*/10 * * * * *", []()
                    { display.setVoltage( float(analogReadMilliVolts(4)) * 2 / 1000); }, false);
        Cron.create("* */10 * * * *", []()
                    { 
                    updateSummary();
                     }, false);
        Cron.create("* * * */1 * *", []()
                    { clockHelper.syncNtp(); }, false);
    }




    void loop()
    {
        Cron.delay();
        displayButton.tick();
        selectButton.tick();
        if (!configured)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                display.setWiFiConnected();
                configure();
            }
        }
            
        clockHelper.getTime();
        display.draw();
    }
