<h1 align=center>
  📡 Micro Radar
</h1>
<h6 align=center>
  a tiny open-source flight radar for your desk
</h6>
<p align=center>
  <img src="https://github.com/user-attachments/assets/2ccb2063-d15c-4180-8e3c-ae3a81c814ff" alt="drawing" width="400"/>
</p>


## About This Fork

This project is based on [AnthonySturdy/micro-radar](https://github.com/AnthonySturdy/micro-radar), a tiny open-source desk flight radar. It has been modified for different hardware and includes several additional features.

### Hardware Changes

The original project uses an ESP32-C3 module with a round 1.28" GC9A01 display. This fork targets the **Waveshare ESP32-S3 Touch LCD 1.54"** board, which brings:

- **ESP32-S3** instead of ESP32-C3
- **1.54" ST7789 240x240 square IPS display** with capacitive touch (CST816 controller)
- **ES8311 audio codec** for speaker output
- **Physical buttons** — power, zoom +, and zoom −
- **Battery voltage monitoring** via ADC
- **Power latch circuit** for proper on/off control and deep sleep

### Additional Features

- **Touch interaction** — tap on aircraft to view flight details (departure/arrival airports, times)
- **AirLabs API support** — richer flight data including airline, registration, and route information alongside OpenSky
- **Built-in airport database** — offline IATA code lookup for airport names
- **Flight detail overlay** — departure/arrival airports with full names, scheduled times
- **Battery indicator** — on-screen battery level with ADC voltage reading
- **Physical button controls** — power on/off, display sleep/wake, zoom in/out, deep sleep on long press
- **WiFi reset** — hold BOOT + PLUS buttons for 3 seconds to clear credentials
- **Metric and imperial units** — configurable speed (m/s, knots) and altitude (m, feet)
- **Zoom HUD** — on-screen radius indicator when adjusting zoom

-