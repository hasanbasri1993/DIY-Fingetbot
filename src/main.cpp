
#include <Arduino.h>
#include <conf.h>

#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProSwitch.h"
#include <ArduinoOTA.h>

#define WIFI_SSID CONF_WIFI_SSID
#define WIFI_PASS CONF_WIFI_PASS
#define APP_KEY CONF_APP_KEY
#define APP_SECRET CONF_APP_SECRET
#define SWITCH_ID CONF_SWITCH_ID
#define SWITCH_GPIO D4
String newHostname = "Switch PC";

#define BAUD_RATE 115200 // Change baudrate to your need

#include <Servo.h>

Servo myservo; // create servo object to control a servo

bool myPowerState = false;
unsigned long previousMillis = 0;
const long intervalCheck = 1000;

bool onPowerState(const String &deviceId, bool &state)
{
    Serial.printf("Device %s turned %s (via SinricPro) \r\n", deviceId.c_str(), state ? "on" : "off");
    myPowerState = state;
    return true;
}

void setupArduinoOta()
{
    ArduinoOTA.onStart([]()
                       {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";
                Serial.println("Start updating " + type); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.setHostname(newHostname.c_str());
    ArduinoOTA.setPassword(PASS_UPLOAD);
    ArduinoOTA.onError([](ota_error_t error)
                       {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

    ArduinoOTA.begin();
}

void handleButtonPress()
{
    unsigned long actualMillis = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    if (myPowerState && actualMillis - previousMillis > intervalCheck)
    {
        digitalWrite(LED_BUILTIN, LOW);
        myservo.write(170);
        SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
        delay(2500);
        Serial.printf("Device %s turned %s (manually via flashbutton)\r\n", mySwitch.getDeviceId().c_str(), myPowerState ? "on" : "off");
        myservo.write(0);
        myPowerState = false;
        mySwitch.sendPowerStateEvent(myPowerState);
        previousMillis = actualMillis;
    }
}

void setupWiFi()
{
    Serial.printf("Default hostname: %s\n", WiFi.hostname().c_str());
    WiFi.mode(WIFI_STA);
    WiFi.hostname(newHostname.c_str());
    Serial.printf("New hostname: %s\n", WiFi.hostname().c_str());
    Serial.printf("\r\n[Wifi]: Connecting");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.printf(".");
        delay(250);
    }
    Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
    setupArduinoOta();
}

void setupSinricPro()
{
    SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];

    mySwitch.onPowerState(onPowerState);

    SinricPro.onConnected([]()
                          {
        SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
        mySwitch.sendPowerStateEvent(false);
        Serial.printf("Connected to SinricPro\r\n"); });
    SinricPro.onDisconnected([]()
                             { Serial.printf("Disconnected from SinricPro\r\n"); });

    SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    myservo.attach(SWITCH_GPIO);
    myservo.write(0);
    Serial.begin(BAUD_RATE);
    Serial.printf("\r\n\r\n");
    setupWiFi();
    setupSinricPro();
}

void loop()
{
    handleButtonPress();
    ArduinoOTA.handle();
    SinricPro.handle();
}