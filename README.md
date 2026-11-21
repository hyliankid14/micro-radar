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

## Elecrow CrowPanel 7-inch

The `crowpanel-7` PlatformIO environment supports the Elecrow CrowPanel Basic
7-inch ESP32-S3 (800x480, 4 MB flash, 8 MB PSRAM). It retains the radar on the
left and shows selected flight details on the right. Tap an aircraft to select
it, tap the detail panel to close it, and use the bottom zoom buttons to adjust
the radius. When idle, the detail panel cycles through visible aircraft every
10 seconds; touch pauses cycling for 15 seconds. Automatic cycling uses radar
data and previously looked-up details without making extra flight-detail API
requests. Hold the bottom-right reset area for three seconds to clear Wi-Fi
credentials. Waveshare-specific battery, power-latch and physical button
features are not available on the CrowPanel.

Build and upload explicitly, choosing the correct serial port:

```sh
pio run -e crowpanel-7
pio run -e crowpanel-7 -t upload --upload-port /dev/cu.usbserial-2110
```

The default environment remains the original Waveshare board. On first boot,
connect to `MicroRadar-Setup` and configure Wi-Fi via the setup portal. Then
visit `http://microradar.local` on the same network to set the radar latitude,
longitude, radius, and optional API credentials. The radius defaults to one
degree on the CrowPanel until configured.

### Experimental buffered refresh

The `crowpanel-7-buffered` environment uses ESP-IDF RGB bounce buffers and
PSRAM instruction/data XIP to reduce display drift under Wi-Fi load. It uses
one framebuffer, so it does not eliminate tearing during drawing, and its
effectiveness on the physical display still needs confirmation.

This experimental toolchain rebuilds SDK libraries and can replace PlatformIO
packages and generated `sdkconfig` files. Build a copy in a path without spaces
with a separate PlatformIO core directory; do not use a shared installation:

```sh
mkdir -p /tmp/micro-radar-buffered/project /tmp/micro-radar-buffered/pio-core
rsync -a --exclude=.git --exclude=.pio --exclude=sdkconfig.\* ./ /tmp/micro-radar-buffered/project/
PLATFORMIO_CORE_DIR=/tmp/micro-radar-buffered/pio-core pio run -d /tmp/micro-radar-buffered/project -e crowpanel-7-buffered
```

The first build downloads and rebuilds the SDK and can take several minutes.
If the nested Arduino build fails with `No module named
'SCons.Tool.FortranCommon'`, rerun the same command with the original
PlatformIO executable after the SDK libraries have been built.
The experimental target preserves the same flash partition layout as
`crowpanel-7`.