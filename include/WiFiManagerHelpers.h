#pragma once

#include <WiFiManager.h>

namespace WiFiManagerHelpers
{
    constexpr const char* WiFiManagerName = "MicroRadar-Setup";

    static void ConfigureWiFiManager(WiFiManager& wm, LGFX& tft)
    {
        wm.setTitle("Micro Radar - Setup WiFi");
        wm.setCustomHeadElement("<style>body{background:#111;color:#00ff00;font-family:monospace;} div:has(> a){background:#00ff00;} a:hover{color:#111;}</style>");

        wm.setAPCallback([&tft](WiFiManager* wifiManager) {
            tft.fillScreen(lgfx::color888(0, 0, 0));
            tft.setTextColor(lgfx::color888(0, 255, 0));
#ifdef CROWPANEL_7
            tft.setTextSize(2);
#endif

            const int lineHeight = tft.fontHeight() + 10;
            tft.drawCenterString("- SETUP -", tft.width() / 2, tft.height() / 2 - lineHeight);
            tft.drawCentreString("Connect to this WiFi hotspot:", tft.width() / 2, tft.height() / 2);
            tft.drawCenterString(WiFiManagerName, tft.width() / 2, tft.height() / 2 + lineHeight);
            }
        );
    }
}