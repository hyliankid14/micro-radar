#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFi.h>
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
#include "ThemePalette.h"

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
#else
constexpr int CROWPANEL_BL_PIN = 2;
bool crowpanelBacklightOn = true;
unsigned long lastWakeInteractionMs = 0;
#endif

float zoomRadius = 0.5;
constexpr float ZOOM_STEP = 0.15;
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

#ifdef CROWPANEL_7
static void DrawBootSplash(LGFX_Sprite& buf, const String& title, const String& subtitle, const String& statusLine)
{
  buf.fillScreen(lgfx::color888(11, 17, 28));
  buf.drawRoundRect(36, 28, 728, 424, 12, lgfx::color888(0, 195, 255));
  buf.fillRoundRect(38, 30, 724, 72, 10, lgfx::color888(24, 38, 64));

  buf.setTextFont(4);
  buf.setTextSize(1);
  buf.setTextColor(lgfx::color888(235, 245, 255));
  buf.drawCentreString(title.isEmpty() ? "SKY FRAME" : title, 400, 42);

  buf.setTextFont(2);
  buf.setTextColor(lgfx::color888(0, 229, 255));
  buf.drawCentreString(subtitle.isEmpty() ? "Live Aviation Display" : subtitle, 400, 74);

  ThemePalette bootTheme = GetThemePalette(THEME_COCKPIT);
  VectorArt::DrawAircraftSilhouette(buf, 180, 124, 440, 210, SIL_WIDEBODY_TWIN, bootTheme, "A35K", "AVIOFRAME EDITION", "LIVE ADS-B", true);

  buf.setTextFont(4);
  buf.setTextColor(lgfx::color888(255, 206, 72));
  buf.drawCentreString(statusLine, 400, 366);

  buf.setTextFont(2);
  buf.setTextColor(lgfx::color888(110, 145, 180));
  buf.drawCentreString("http://microradar.local", 400, 404);
  buf.setTextFont(1);
  buf.pushSprite(0, 0);
}

static void DrawBottomToolbar(LGFX_Sprite& buf, const ThemePalette& theme)
{
  const int y = 442;
  const int h = 32;

  auto drawBtn = [&](int x, int w, const String& label, bool highlighted = false) {
    buf.fillRoundRect(x, y, w, h, 5, highlighted ? theme.bgHeader : theme.buttonBg);
    buf.drawRoundRect(x, y, w, h, 5, highlighted ? theme.textAccent : theme.buttonBorder);
    buf.setTextFont(2);
    buf.setTextSize(1);
    buf.setTextColor(highlighted ? theme.textAccent : theme.buttonText);
    buf.drawCentreString(label, x + w / 2, y + 8);
  };

  drawBtn(8,   78, "ZOOM +");
  drawBtn(92,  78, "ZOOM -");
  drawBtn(176, 152, String("VIEW: ") + ViewModeToDisplayName(aircraftManager.GetViewMode()));
  drawBtn(334, 154, String("THEME: ") + ThemeIdToDisplayName(aircraftManager.GetThemeId()));
  drawBtn(494, 98,  String("STATS (") + String(aircraftManager.GetSpotterStats().totalUniqueSeen) + ")", aircraftManager.IsStatsModalOpen());

  // Right status & IP info box
  buf.fillRoundRect(598, y, 194, h, 5, theme.bgSecondary);
  buf.drawRoundRect(598, y, 194, h, 5, theme.borderSecondary);

  buf.setTextFont(1);
  buf.setTextColor(theme.textPrimary);
  float rangeDist = aircraftManager.ConvertDistance(static_cast<float>(aircraftManager.getRadius() * 111.32));
  String rangeStr = "Range: " + String(rangeDist, rangeDist < 15.0f ? 1 : 0) + " " + aircraftManager.DistUnit();
  buf.drawString(rangeStr, 606, y + 6);

  buf.setTextColor(theme.textAccent);
  String hostStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "microradar.local";
  buf.drawString(hostStr, 606, y + 18);

  buf.setTextColor(theme.textMuted);
  buf.drawRightString("RST 3s", 786, y + 6);
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

  deviceState = STATE_ON;
#else
  TouchManager::Initialise();
  pinMode(CROWPANEL_BL_PIN, OUTPUT);
  digitalWrite(CROWPANEL_BL_PIN, HIGH);
  crowpanelBacklightOn = true;
  lastWakeInteractionMs = millis();
  Serial.printf("[BOOT] PSRAM: %lu bytes\n", static_cast<unsigned long>(ESP.getPsramSize()));
#endif

  if (tft.init()) {
    Serial.println("[BOOT] Display initialised");
  } else {
    Serial.println("[BOOT] Display init FAILED");
    while (true) delay(1000);
  }
#ifndef CROWPANEL_7
  tft.invertDisplay(true);
#endif

#ifdef CROWPANEL_7
  // 16-bit RGB565 backbuffer in 8 MB PSRAM for real aircraft photos, carrier logos & rich themes
  backbuffer.setColorDepth(16);
#else
  backbuffer.setColorDepth(8);
#endif
  backbuffer.setPsram(true);
  if (!backbuffer.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
    Serial.println("[BOOT] Framebuffer allocation FAILED");
    while (true) delay(1000);
  }

  String bootTitle = configServer.GetStoredString("gift-title");
  String bootSub = configServer.GetStoredString("gift-subtitle");
#ifdef CROWPANEL_7
  DrawBootSplash(backbuffer, bootTitle, bootSub, "Connecting to Wi-Fi...");
#else
  backbuffer.fillScreen(lgfx::color888(0, 0, 0));
  backbuffer.setTextColor(lgfx::color888(0, 255, 0));
  backbuffer.drawCentreString("Connecting to WiFi...", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
  backbuffer.pushSprite(0, 0);
#endif

  WiFiManagerHelpers::ConfigureWiFiManager(wm, tft);
  wm.autoConnect(WiFiManagerHelpers::WiFiManagerName);
  Serial.println("[BOOT] WiFi connected");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("[BOOT] NTP sync requested (UTC)");

#ifndef CROWPANEL_7
  TouchManager::Initialise();
#endif

  if (airportDb.begin()) {
    Serial.printf("[BOOT] Airport database ready: %d airports loaded\n", airportDb.getAirportCount());
  }

  configServer.SetAirportDatabase(&airportDb);
  configServer.SetAircraftManager(&aircraftManager);
  configServer.Initialise();
  airlabsManager.Initialise();

  Serial.println("[BOOT] Config web server ready - http://microradar.local");
  aircraftManager.Initialise();
  zoomRadius = aircraftManager.getRadius();

#ifndef CROWPANEL_7
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#endif

#ifdef CROWPANEL_7
  DrawBootSplash(backbuffer, aircraftManager.GetGiftTitle(), aircraftManager.GetGiftSubtitle(), "Syncing Live Aviation Clock...");
#else
  backbuffer.fillScreen(lgfx::color888(0, 0, 0));
  backbuffer.setTextColor(lgfx::color888(0, 255, 0));
  backbuffer.drawCentreString("Syncing UTC...", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
  backbuffer.pushSprite(0, 0);
#endif

  unsigned long ntpStart = millis();
  while ((long)time(nullptr) < 1700000000 && millis() - ntpStart < 4000) {
    delay(200);
  }
  Serial.printf("[NTP] UTC synced to: %ld\n", (long)time(nullptr));
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
  zoomRadius = aircraftManager.getRadius();

  {
    static unsigned long touchDownMs = 0;
    static int lastTouchX = 0, lastTouchY = 0;
    static bool wasTouching = false;
    static const unsigned long TAP_MAX_MS = 550;

    auto touch = TouchManager::ConsumeTouch();
    unsigned long now = millis();

    if (touch.valid) {
#ifdef CROWPANEL_7
      lastWakeInteractionMs = now;
      if (!crowpanelBacklightOn) {
        digitalWrite(CROWPANEL_BL_PIN, HIGH);
        crowpanelBacklightOn = true;
      }
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
      if (touch.x >= 680 && touch.y >= 440 && now - touchDownMs >= 3000) {
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
        if (lastTouchY >= 438) {
          // Bottom Toolbar Touch Zones
          if (lastTouchX < 90) {
            aircraftManager.adjustRadius(-ZOOM_STEP);
          } else if (lastTouchX < 174) {
            aircraftManager.adjustRadius(+ZOOM_STEP);
          } else if (lastTouchX < 332) {
            aircraftManager.CycleViewMode();
          } else if (lastTouchX < 490) {
            aircraftManager.CycleTheme();
          } else if (lastTouchX < 596) {
            aircraftManager.ToggleStatsModal();
          }
          zoomRadius = aircraftManager.getRadius();
        } else {
          aircraftManager.HandleContentTap(lastTouchX, lastTouchY);
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
  // Night quiet-hours backlight auto-sleep (wakes on touch for 60s)
  if (aircraftManager.IsNightQuietHours()) {
    if (crowpanelBacklightOn && (millis() - lastWakeInteractionMs > 60000)) {
      digitalWrite(CROWPANEL_BL_PIN, LOW);
      crowpanelBacklightOn = false;
    }
  } else if (!crowpanelBacklightOn) {
    digitalWrite(CROWPANEL_BL_PIN, HIGH);
    crowpanelBacklightOn = true;
  }

  aircraftManager.UpdateIdleSelection(millis());
#endif

  aircraftManager.Draw(backbuffer);

#ifdef CROWPANEL_7
  DrawBottomToolbar(backbuffer, aircraftManager.GetTheme());
#else
  drawZoomLevel(backbuffer);
  {
    float v = readBatteryVoltage();
    int level = getBatteryLevel(v);
    drawBatteryIndicator(backbuffer, level);
  }
#endif

  backbuffer.pushSprite(0, 0);

  // Process at most one queued aircraft photo / carrier logo download after frame is on screen
  aircraftManager.ProcessBackgroundMedia();

#ifdef CROWPANEL_7
  delay(20);
#endif
}
