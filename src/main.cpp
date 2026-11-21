#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <Wire.h>

#include "LGFX.h"
#include "WiFiManagerHelpers.h"
#include "ConfigurationWebServer.h"
#include "AirportDatabase.h"
#include "AirLabsManager.h"
#include "HttpRequestManager.h"
#include "OpenSkyAuthTokenHandler.h"
#include "AircraftManager.h"
#include "DrawHelpers.h"
#include "TouchManager.h"
#include "DisplayLayout.h"

#ifndef CROWPANEL_7
constexpr int SCREEN_SIZE_DIV_2 = DISPLAY_WIDTH / 2;
constexpr int BACKLIGHT_PIN = 46;
constexpr int PWR_LATCH_PIN = 2;
constexpr int I2C_SDA_PIN = 42;
constexpr int I2C_SCL_PIN = 41;

constexpr int BTN_BOOT = 0;
constexpr int BTN_PLUS = 4;
constexpr int BTN_PWR = 5;

constexpr unsigned long DEBOUNCE_MS = 30;
constexpr unsigned long LONG_PRESS_MS = 2000;
constexpr unsigned long RESET_HOLD_MS = 3000;

enum DeviceState {
  STATE_ON,
  STATE_DISPLAY_OFF,
};

DeviceState deviceState = STATE_ON;
#endif
float zoomRadius = 0.5;
constexpr float ZOOM_STEP = 0.1;
constexpr float ZOOM_MIN = 0.05;
constexpr float ZOOM_MAX = 180.0;

constexpr unsigned long ZOOM_DISPLAY_MS = 2000;
unsigned long zoomDisplayUntil = 0;

#ifndef CROWPANEL_7
constexpr int BATTERY_PIN = 1;
constexpr float BATTERY_MULTIPLIER = 3.0f;

struct ButtonState {
  int pin;
  bool lastRaw;
  bool lastStable;
  unsigned long lastChangeMs;
  unsigned long pressStartMs;
  bool longPressTriggered;
};

ButtonState btnBoot  = { BTN_BOOT, HIGH, HIGH, 0, 0, false };
ButtonState btnPlus  = { BTN_PLUS, HIGH, HIGH, 0, 0, false };
ButtonState btnPwr   = { BTN_PWR,  HIGH, HIGH, 0, 0, false };
#endif

LGFX tft;
LGFX_Sprite backbuffer(&tft);

WiFiManager wm;
ConfigurationWebServer configServer;
AirportDatabase airportDb;
HttpRequestManager httpManager;
AirLabsManager airlabsManager(configServer, httpManager);
OpenSkyAuthTokenHandler authHandler(httpManager);

AircraftManager aircraftManager(configServer, airlabsManager, authHandler, httpManager, &airportDb, tft);

#ifndef CROWPANEL_7
bool debounceButton(ButtonState& b)
{
  bool raw = digitalRead(b.pin);
  unsigned long now = millis();

  if (raw != b.lastRaw) {
    b.lastChangeMs = now;
    b.lastRaw = raw;
  }

  if (now - b.lastChangeMs < DEBOUNCE_MS) return false;
  if (raw == b.lastStable) return false;

  b.lastStable = raw;
  return true;
}

void enterDeepSleep()
{
  Serial.println("[PWR] Entering deep sleep...");
  Serial.flush();

  digitalWrite(BACKLIGHT_PIN, LOW);
  tft.sleep();

  rtc_gpio_init((gpio_num_t)PWR_LATCH_PIN);
  rtc_gpio_set_direction((gpio_num_t)PWR_LATCH_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_level((gpio_num_t)PWR_LATCH_PIN, 1);
  gpio_hold_en((gpio_num_t)PWR_LATCH_PIN);

  rtc_gpio_init((gpio_num_t)BACKLIGHT_PIN);
  rtc_gpio_set_direction((gpio_num_t)BACKLIGHT_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_level((gpio_num_t)BACKLIGHT_PIN, 0);
  gpio_hold_en((gpio_num_t)BACKLIGHT_PIN);

  uint64_t wakeupMask = (1ULL << BTN_PWR);
  esp_sleep_enable_ext1_wakeup(wakeupMask, ESP_EXT1_WAKEUP_ANY_LOW);

  esp_deep_sleep_start();
}

void checkLongPress()
{
  if (btnPwr.pressStartMs == 0) return;
  bool currentState = digitalRead(BTN_PWR);
  if (currentState == LOW) {
    unsigned long now = millis();
    if (!btnPwr.longPressTriggered && (now - btnPwr.pressStartMs) >= LONG_PRESS_MS) {
      btnPwr.longPressTriggered = true;
      Serial.println("[PWR] Holding 2s - powering off");

      backbuffer.fillScreen(lgfx::color888(0, 0, 0));
      backbuffer.setTextColor(lgfx::color888(0, 255, 0));
      backbuffer.setTextSize(2);
      backbuffer.drawCentreString("Powering Off", SCREEN_SIZE_DIV_2, SCREEN_SIZE_DIV_2);
      backbuffer.pushSprite(0, 0);

      delay(800);
      enterDeepSleep();
    }
  }
}

void handlePwrButton()
{
  if (!debounceButton(btnPwr)) return;

  if (btnPwr.lastStable == LOW) {
    btnPwr.pressStartMs = millis();
    btnPwr.longPressTriggered = false;
    Serial.println("[PWR] Button pressed");
  } else {
    unsigned long pressDuration = millis() - btnPwr.pressStartMs;
    Serial.print("[PWR] Button released, duration="); Serial.print(pressDuration); Serial.println("ms");

    if (pressDuration < 500 && !btnPwr.longPressTriggered) {
      if (deviceState == STATE_ON) {
        deviceState = STATE_DISPLAY_OFF;
        digitalWrite(BACKLIGHT_PIN, LOW);
        tft.sleep();
        Serial.println("[PWR] Display OFF");
      } else {
        deviceState = STATE_ON;
        tft.wakeup();
        digitalWrite(BACKLIGHT_PIN, HIGH);
        Serial.println("[PWR] Display ON");
      }
    }
    btnPwr.pressStartMs = 0;
  }
}

unsigned long resetHoldStartMs = 0;

void checkResetHold()
{
  bool plusHeld = (btnPlus.lastStable == LOW);
  bool bootHeld = (btnBoot.lastStable == LOW);

  if (plusHeld && bootHeld) {
    unsigned long now = millis();
    if (resetHoldStartMs == 0) {
      resetHoldStartMs = now;
    } else if (now - resetHoldStartMs >= RESET_HOLD_MS) {
      Serial.println("[RESET] + and - held 3s - clearing WiFi and restarting");

      backbuffer.fillScreen(lgfx::color888(0, 0, 0));
      backbuffer.setTextColor(lgfx::color888(0, 255, 0));
      backbuffer.setTextSize(2);
      backbuffer.drawCentreString("Resetting WiFi...", SCREEN_SIZE_DIV_2, SCREEN_SIZE_DIV_2);
      backbuffer.pushSprite(0, 0);

      delay(1000);
      wm.resetSettings();
      ESP.restart();
    }
  } else {
    resetHoldStartMs = 0;
  }
}

void handleZoomButtons()
{
  if (debounceButton(btnPlus) && btnPlus.lastStable == LOW) {
    aircraftManager.adjustRadius(-ZOOM_STEP);
    zoomRadius = aircraftManager.getRadius();
    zoomDisplayUntil = millis() + ZOOM_DISPLAY_MS;
    Serial.print("[ZOOM+] Radius: "); Serial.println(zoomRadius, 2);
  }

  if (debounceButton(btnBoot) && btnBoot.lastStable == LOW) {
    aircraftManager.adjustRadius(+ZOOM_STEP);
    zoomRadius = aircraftManager.getRadius();
    zoomDisplayUntil = millis() + ZOOM_DISPLAY_MS;
    Serial.print("[ZOOM-] Radius: "); Serial.println(zoomRadius, 2);
  }
}

float readBatteryVoltage()
{
  long sum = 0;
  const int SAMPLES = 32;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogReadMilliVolts(BATTERY_PIN);
  }
  float mv = (float)sum / SAMPLES;
  return (mv / 1000.0f) * BATTERY_MULTIPLIER;
}

int getBatteryLevel(float voltage)
{
  if (voltage < 3.52f) return 1;
  if (voltage < 3.64f) return 20;
  if (voltage < 3.76f) return 40;
  if (voltage < 3.88f) return 60;
  if (voltage < 4.00f) return 80;
  return 100;
}

void drawBatteryIndicator(LGFX_Sprite& buf, int level)
{
  constexpr int X = 212;
  constexpr int Y = 4;
  constexpr int W = 22;
  constexpr int H = 10;
  constexpr int TIP_W = 3;
  constexpr int TIP_H = 4;
  constexpr int PAD = 2;
  constexpr int SEGMENTS = 4;

  buf.drawRect(X, Y, W, H, lgfx::color888(0, 200, 0));
  buf.fillRect(X + W, Y + (H - TIP_H) / 2, TIP_W, TIP_H, lgfx::color888(0, 200, 0));

  int innerW = W - PAD * 2;
  int innerH = H - PAD * 2;

  buf.fillRect(X + PAD, Y + PAD, innerW, innerH, lgfx::color888(10, 10, 0));

  int filledSegments = (level * SEGMENTS + 50) / 100;
  if (filledSegments < 0) filledSegments = 0;
  if (filledSegments > SEGMENTS) filledSegments = SEGMENTS;

  int segGap = 1;
  int segW = (innerW - (SEGMENTS - 1) * segGap) / SEGMENTS;

  uint32_t color = lgfx::color888(0, 220, 0);
  if (level < 20) color = lgfx::color888(220, 50, 0);
  else if (level < 40) color = lgfx::color888(220, 180, 0);

  for (int i = 0; i < filledSegments; i++) {
    int sx = X + PAD + i * (segW + segGap);
    buf.fillRect(sx, Y + PAD, segW, innerH, color);
  }
}

void drawZoomLevel(LGFX_Sprite& buf)
{
  if (millis() < zoomDisplayUntil) {
    buf.setTextSize(1);
    buf.setTextColor(lgfx::color888(0, 255, 0));
    char text[24];
    snprintf(text, sizeof(text), "%.1f%c", zoomRadius, (char)247);
    buf.drawString(text, 4, 4);
  }
}
#endif

void setup()
{
  esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();

  Serial.begin(115200);
  delay(300);

  if (wakeupCause != ESP_SLEEP_WAKEUP_UNDEFINED) {
    Serial.println("[BOOT] Woke from deep sleep");
  } else {
    Serial.println("[BOOT] Micro Radar starting...");
  }

#ifndef CROWPANEL_7
  pinMode(PWR_LATCH_PIN, OUTPUT);
  digitalWrite(PWR_LATCH_PIN, HIGH);
  gpio_hold_dis((gpio_num_t)PWR_LATCH_PIN);

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  gpio_hold_dis((gpio_num_t)BACKLIGHT_PIN);

  pinMode(BTN_BOOT, INPUT_PULLUP);
  pinMode(BTN_PLUS, INPUT_PULLUP);
  pinMode(BTN_PWR, INPUT_PULLUP);
  delay(50);
  btnBoot.lastRaw = btnBoot.lastStable = digitalRead(BTN_BOOT);
  btnPlus.lastRaw = btnPlus.lastStable = digitalRead(BTN_PLUS);
  btnPwr.lastRaw  = btnPwr.lastStable  = digitalRead(BTN_PWR);
  Serial.print("[BOOT] Button states BOOT=");
  Serial.print(btnBoot.lastStable);
  Serial.print(" PLUS=");
  Serial.print(btnPlus.lastStable);
  Serial.print(" PWR=");
  Serial.println(btnPwr.lastStable);

  deviceState = STATE_ON;
#else
  TouchManager::Initialise();
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  Serial.printf("[BOOT] PSRAM: %lu bytes\n", static_cast<unsigned long>(ESP.getPsramSize()));
#endif

  if (tft.init()) {
    Serial.println("[BOOT] Display initialised");
#ifdef CROWPANEL_7
    Serial.printf("[BOOT] Display color depth: %d bits\n",
                  static_cast<int>(tft.getColorDepth()) & lgfx::color_depth_t::bit_mask);
#endif
  } else {
    Serial.println("[BOOT] Display init FAILED");
    while (true) delay(1000);
  }
#ifndef CROWPANEL_7
  tft.invertDisplay(true);
  Serial.printf("[BATT] Startup ADC voltage: %.3fV\n", readBatteryVoltage());
#endif

  backbuffer.setColorDepth(8);
  backbuffer.setPsram(true);
  if (!backbuffer.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
    Serial.println("[BOOT] Framebuffer allocation FAILED");
    while (true) delay(1000);
  }
  backbuffer.fillScreen(lgfx::color888(0, 0, 0));
  backbuffer.setTextColor(lgfx::color888(0, 255, 0));
  backbuffer.drawCentreString("Connecting to WiFi...", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
  backbuffer.pushSprite(0, 0);

  WiFiManagerHelpers::ConfigureWiFiManager(wm, tft);
  wm.autoConnect(WiFiManagerHelpers::WiFiManagerName);
  Serial.println("[BOOT] WiFi connected");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("[BOOT] NTP sync requested (UTC)");

#ifndef CROWPANEL_7
  TouchManager::Initialise();
#endif
  configServer.Initialise();
  airlabsManager.Initialise();
  
  // Initialize airport database
  if (airportDb.begin()) {
    Serial.printf("[BOOT] Airport database ready: %d airports loaded\n", airportDb.getAirportCount());
  } else {
    Serial.println("[BOOT] Airport database not available");
  }
  
  Serial.println("[BOOT] Config web server ready - http://microradar.local");
  aircraftManager.Initialise();
  zoomRadius = aircraftManager.getRadius();
  Serial.print("[BOOT] Aircraft manager ready, initial radius: ");
  Serial.println(zoomRadius, 2);

#ifndef CROWPANEL_7
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("[BOOT] I2C bus initialised");
#endif

  backbuffer.fillScreen(lgfx::color888(0, 0, 0));
  backbuffer.setTextColor(lgfx::color888(0, 255, 0));
  backbuffer.drawCentreString("Syncing UTC...", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
  backbuffer.pushSprite(0, 0);
  Serial.print("[NTP] UTC time: ");
  Serial.println((long)time(nullptr));

  unsigned long ntpStart = millis();
  while ((long)time(nullptr) < 1700000000 && millis() - ntpStart < 5000) {
    delay(200);
  }
  Serial.print("[NTP] UTC synced to: ");
  Serial.println((long)time(nullptr));
}

void loop()
{
#ifndef CROWPANEL_7
  handlePwrButton();
  handleZoomButtons();
  checkLongPress();
  checkResetHold();

  if (deviceState != STATE_ON) {
    delay(20);
    return;
  }
#endif

  aircraftManager.Update();

  {
    static unsigned long touchDownMs = 0;
    static int lastTouchX = 0, lastTouchY = 0;
    static bool wasTouching = false;
    static const unsigned long TAP_MAX_MS = 500;

    auto touch = TouchManager::ConsumeTouch();
    unsigned long now = millis();

    if (touch.valid) {
#ifdef CROWPANEL_7
      aircraftManager.NoteInteraction(now);
#endif
      if (!wasTouching) {
        touchDownMs = now;
        Serial.printf("[TOUCH] Press at %d,%d\n", touch.x, touch.y);
      }
      wasTouching = true;
      lastTouchX = touch.x;
      lastTouchY = touch.y;
#ifdef CROWPANEL_7
      if (touch.x >= 650 && touch.y >= 444 && now - touchDownMs >= 3000) {
        Serial.println("[RESET] Clearing WiFi credentials");
        wm.resetSettings();
        ESP.restart();
      }
#endif
    } else if (wasTouching) {
      unsigned long duration = now - touchDownMs;
      wasTouching = false;
      if (duration < TAP_MAX_MS) {
        Serial.printf("[TOUCH] Tap at %d,%d (hold=%lums)\n", lastTouchX, lastTouchY, duration);
#ifdef CROWPANEL_7
        if (lastTouchY >= 444 && lastTouchX < 200) {
          aircraftManager.adjustRadius(lastTouchX < 100 ? -ZOOM_STEP : ZOOM_STEP);
          zoomRadius = aircraftManager.getRadius();
        } else if (lastTouchX >= 452 && aircraftManager.IsDetailOpen()) {
          aircraftManager.DismissDetail();
        } else if (lastTouchX < RADAR_X + RADAR_SIZE && lastTouchY < RADAR_Y + RADAR_SIZE) {
          String hit = aircraftManager.HitTestAircraft(lastTouchX, lastTouchY);
          if (!hit.isEmpty()) aircraftManager.SelectFlight(hit);
        }
#else
        if (aircraftManager.IsDetailOpen()) {
          aircraftManager.DismissDetail();
        } else {
          String hit = aircraftManager.HitTestAircraft(lastTouchX, lastTouchY);
          if (!hit.isEmpty()) {
            aircraftManager.SelectFlight(hit);
          }
        }
#endif
      }
#ifdef CROWPANEL_7
      aircraftManager.NoteInteraction(millis());
#endif
    }
  }

#ifdef CROWPANEL_7
  aircraftManager.UpdateIdleSelection(millis());
#endif

  backbuffer.fillScreen(lgfx::color888(0, 0, 0));
  String renderScanlines = configServer.GetStoredString("scanline");
  if (renderScanlines.isEmpty() || renderScanlines == "true") {
    DrawScanLines(backbuffer,
      RADAR_CENTRE_X,
      RADAR_CENTRE_Y,
      RADAR_CENTRE_X + (std::cos(millis() / 3000.0f) * RADAR_RADIUS),
      RADAR_CENTRE_Y + (std::sin(millis() / 3000.0f) * RADAR_RADIUS),
      20, 128, 5, RADAR_RADIUS);
  }
  aircraftManager.Draw(backbuffer);
  if (aircraftManager.IsDetailOpen()) {
    aircraftManager.DrawFlightDetail(backbuffer);
  }
#ifdef CROWPANEL_7
  if (!aircraftManager.IsDetailOpen()) {
    backbuffer.drawRect(452, 8, 340, 432, lgfx::color888(0, 100, 0));
    backbuffer.setTextFont(2);
    backbuffer.setTextColor(lgfx::color888(0, 220, 0));
    backbuffer.drawString("MICRO RADAR", 468, 24);
    backbuffer.drawString("Tap an aircraft for details", 468, 60);
    backbuffer.setTextFont(1);
  }
  backbuffer.setTextFont(2);
  backbuffer.setTextColor(lgfx::color888(0, 220, 0));
  backbuffer.drawRect(8, 444, 88, 32, lgfx::color888(0, 180, 0));
  backbuffer.drawRect(104, 444, 88, 32, lgfx::color888(0, 180, 0));
  backbuffer.drawCentreString("ZOOM +", 52, 451);
  backbuffer.drawCentreString("ZOOM -", 148, 451);
  backbuffer.drawString("Radius: " + String(zoomRadius, 2) + " deg", 208, 451);
  backbuffer.drawRightString("Hold to reset WiFi", 792, 451);
  backbuffer.setTextFont(1);
#else
  drawZoomLevel(backbuffer);
  {
    float v = readBatteryVoltage();
    int level = getBatteryLevel(v);
    static unsigned long lastBatLog = 0;
    unsigned long now = millis();
    if (now - lastBatLog > 30000) {
      lastBatLog = now;
      Serial.printf("[BATT] %.2fV level=%d%%\n", v, level);
    }
    drawBatteryIndicator(backbuffer, level);
  }
#endif
  backbuffer.pushSprite(0, 0);
#ifdef CROWPANEL_7
  delay(20);
#endif
}
