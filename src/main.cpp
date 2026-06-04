#include <Arduino.h>
#include <WiFiManager.h>

#include "LGFX.h"

#define SCREEN_SIZE 240

LGFX tft;
WiFiManager wm;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.invertDisplay(true);
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);

  tft.fillScreen(lgfx::color888(0, 0, 0));

  tft.setTextColor(lgfx::color888(0, 255, 0));
  tft.drawCentreString("Connecting to WiFi...", SCREEN_SIZE / 2, SCREEN_SIZE / 2);

  wm.autoConnect("FlightRadar-Setup");
}

void loop()
{
  tft.fillScreen(lgfx::color888(0, 0, 0));
  tft.fillCircle(120, 120, 110, lgfx::color888(0, 255, 0));
  tft.fillCircle(120, 120, 60, lgfx::color888(255, 0, 0));
  tft.fillCircle(120, 120, 30, lgfx::color888(0, 0, 255));

  tft.setTextColor(lgfx::color888(255, 255, 255));
  tft.drawCentreString("Hello, world!", SCREEN_SIZE / 2, SCREEN_SIZE / 2);
}