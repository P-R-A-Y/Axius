#include <AxiusSSD.h>
#include "OTA.h"

void OTA::setup() {}
void OTA::firsttick() {state = 0;}

#ifdef ESP32

#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

void OTA::tick() {
  if (state == 0) {
    axius->drawText("up - continue", 0);
    axius->drawText("ok - exit", 1);
    if (axius->clickZ()) axius->tomenu();
    else if (axius->clickX()) state = 1;
  } else if (state == 1) {
    axius->drawText("preparing", 0);
    axius->disableWIFI();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    state = 2;
  } else if (state == 2) {
    if (WiFi.waitForConnectResult(300) != WL_CONNECTED) {
      axius->drawText("waiting for wifi", 0);
      axius->drawText("with name \""+ssid+"\"", 1);
      axius->drawText("and password \""+password+"\"", 1);
    } else {
      state = 3;
      ArduinoOTA.setHostname(axius->deviceName.c_str());
      ArduinoOTA.setPassword("AXIUS");
    }
  } else if (state == 3) {
    ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else {  // U_SPIFFS
          type = "filesystem";
        }

        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
          Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
          Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
          Serial.println("End Failed");
        }
      });

    ArduinoOTA.begin();
    state = 4;
  } else if (state == 4) {
    
  }
}

#else
void OTA::tick() {
  axius->drawText("unsupported", 0);
  axius->drawText("ok - exit", 1);
  if (axius->clickZ()) axius->tomenu();
}
#endif