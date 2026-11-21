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

## Elecrow CrowPanel 7-inch (AvioFrame Edition)

The `crowpanel-7` PlatformIO environment supports the Elecrow CrowPanel Basic 7-inch ESP32-S3 (800×480, 4 MB flash, 8 MB PSRAM) with a full suite of living-room aviation display features inspired by [AvioFrame](https://avioframe.com/):

- **3 Switchable Display Views**:
  - **Radar + Spotter Card (`RADAR + CARD`)**: Split view with circular radar scope, range rings, compass markers, breadcrumb flight trails, and a detailed Spotter Card on the right.
  - **Split-Flap Airport Departure Board (`DEPARTURE BOARD`)**: Full-screen retro mechanical split-flap / LED terminal board displaying up to 7 live flights sorted by distance (tap any row to open its full poster).
  - **AvioFrame Portrait Poster (`SPOTTER POSTER`)**: Full-screen gallery poster featuring large aircraft photography/schematics, carrier branding, origin/destination route progress bar, 6 avionics telemetry cards, and a mini compass locator radar.
- **5 Color Themes**: **Midnight Cockpit** (EFIS cyan/magenta), **AvioFrame Gallery Light** (warm paper & vermilion ink), **Split-Flap Solari Amber**, **Technical Blueprint**, and **Classic Radar Green**.
- **Real Aircraft Photos & Carrier Logos (PSRAM Cache + Vector Fallbacks)**:
  - Automatically downloads real aircraft thumbnail photos via the free **Planespotters.net API** and carrier logos by IATA code into the 8 MB PSRAM, decoded into 16-bit RGB565 bitmaps.
  - Instant procedural **vector aircraft silhouettes** (8 airframe categories: 4-engine superjumbo, twin-aisle widebody, narrowbody jet, regional turboprop, business jet, light GA prop, helicopter, military jet) and **stylized airline livery tail-fin badges** (140+ airlines mapped by 3-letter ICAO & 2-letter IATA codes) when loading or offline.
- **"Track a Flight" / Partner Flight Pinning**:
  - Pin any flight worldwide by Flight Number / Callsign (e.g. `BA117`, `BAW117`, `U28412`), Tail Registration (`G-XWBA`), or ICAO Hex from your phone — tracks the pinned flight live via `adsb.lol` even when thousands of miles outside your local radar radius.
- **Easy Phone Setup & Live Remote Control (`http://microradar.local`)**:
  - On-screen **scannable QR codes** for both first-time Wi-Fi hotspot connection and opening the phone remote control.
  - Mobile web app with **Live Remote Control** (switch views, themes, zoom, pin flights, or tap a flight from the live list to display it on screen with zero reboot) and **1-Tap Location Setup** (phone GPS or 24 major airport/city presets).
- **Gift Personalization, Night Schedule & Spotter Log**:
  - Customizable frame title & gift greeting message on the boot and welcome screens.
  - Configurable night quiet-hours schedule that automatically sleeps the backlight overnight and wakes on touch.
  - Session **Sky Spotter Log & Records** modal tracking Total Spotted, Closest Flyover, Highest Altitude, Fastest Ground Speed, and **Rare Catches** (A380/B747 Superjumbos, Helicopters, Military, Emergency Squawks).

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