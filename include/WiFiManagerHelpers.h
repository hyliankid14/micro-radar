#pragma once

#include <WiFiManager.h>
#include "LGFX.h"

namespace WiFiManagerHelpers
{
    constexpr const char* WiFiManagerName = "MicroRadar-Setup";

    static void ConfigureWiFiManager(WiFiManager& wm, LGFX& tft)
    {
        wm.setTitle("Sky Frame — Setup Wi-Fi");
        wm.setCustomHeadElement(
            "<style>"
            "body{background:#0b111c;color:#ebf5ff;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;}"
            "button,input[type='submit']{background:#00e5ff !important;color:#06101c !important;border-radius:8px !important;font-weight:700 !important;border:none !important;}"
            "input{background:#121c2e !important;color:#fff !important;border:1px solid #253f63 !important;border-radius:6px !important;}"
            "</style>"
        );

        wm.setAPCallback([&tft](WiFiManager* wifiManager) {
            tft.fillScreen(lgfx::color888(11, 17, 28));
            tft.setTextColor(lgfx::color888(0, 229, 255));

#ifdef CROWPANEL_7
            tft.drawRoundRect(40, 28, 720, 424, 12, lgfx::color888(0, 195, 255));
            tft.fillRoundRect(42, 30, 716, 62, 10, lgfx::color888(24, 38, 64));

            tft.setTextFont(4);
            tft.setTextSize(1);
            tft.setTextColor(lgfx::color888(235, 245, 255));
            tft.drawCentreString("SKY FRAME — FIRST-TIME WI-FI SETUP", 400, 48);

            // Standard Wi-Fi auto-join QR code for iOS & Android cameras
            const int qrSize = 180;
            const int qrX = 96;
            const int qrY = 145;
            tft.fillRoundRect(qrX - 10, qrY - 10, qrSize + 20, qrSize + 20, 8, lgfx::color888(255, 255, 255));
            String wifiQr = String("WIFI:S:") + WiFiManagerName + ";T:nopass;;";
            tft.qrcode(wifiQr, qrX, qrY, qrSize, 3);

            int tx = 320;
            tft.setTextFont(4);
            tft.setTextColor(lgfx::color888(0, 229, 255));
            tft.drawString("Step 1: Scan QR Code", tx, 140);

            tft.setTextFont(2);
            tft.setTextColor(lgfx::color888(210, 230, 250));
            tft.drawString("Point your phone camera at the QR code", tx, 174);
            tft.drawString("to join the setup hotspot automatically:", tx, 194);

            tft.fillRoundRect(tx, 222, 310, 38, 6, lgfx::color888(18, 28, 46));
            tft.drawRoundRect(tx, 222, 310, 38, 6, lgfx::color888(0, 195, 255));
            tft.setTextFont(4);
            tft.setTextColor(lgfx::color888(255, 206, 72));
            tft.drawCentreString(WiFiManagerName, tx + 155, 230);

            tft.setTextFont(4);
            tft.setTextColor(lgfx::color888(0, 229, 255));
            tft.drawString("Step 2: Choose Home Wi-Fi", tx, 282);

            tft.setTextFont(2);
            tft.setTextColor(lgfx::color888(210, 230, 250));
            tft.drawString("A setup page will pop up on your phone", tx, 316);
            tft.drawString("(or open http://192.168.4.1 in your browser).", tx, 336);
            tft.setTextFont(1);
#else
            const int lineHeight = tft.fontHeight() + 10;
            tft.drawCenterString("- SETUP -", tft.width() / 2, tft.height() / 2 - lineHeight);
            tft.drawCentreString("Connect to this WiFi hotspot:", tft.width() / 2, tft.height() / 2);
            tft.drawCenterString(WiFiManagerName, tft.width() / 2, tft.height() / 2 + lineHeight);
#endif
        });
    }
}
