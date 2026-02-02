#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WIFI
const char* ssid = "wifi";
const char* password = "pass";

// SINRIC
#define APP_KEY     "APP_KEY" // https://sinric.pro
#define APP_SECRET  "APP_SECRET"
#define device1 "DEV1"   // Monitor
#define device2 "DEV2"   // Plug
#define device3 "DEV3"   // Xbox
#define device4 "DEV4"   // Spotify rutina

// TOUCH
#define BTN1 2
#define BTN2 3
#define BTN3 4
#define BTN4 5
#define TOUCH_THRESHOLD 30

bool state1, state2, state3;
bool nightMode = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

String message = "";
unsigned long messageTime = 0;

void showMessage(String msg) {
  message = msg;
  messageTime = millis();

  for (int x = 128; x > 0; x -= 8) {
    display.clearDisplay();
    display.setRotation(1);
    display.setTextSize(2);
    display.setCursor(x, 8);
    display.print(msg);
    display.display();
    delay(10);
  }
}

// -displayyyy-
void drawDisplay() {
  if (millis() - messageTime < 2500 && message != "") return;

  display.clearDisplay();
  display.setRotation(1);

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Time ");
  display.println(timeClient.getFormattedTime());

  display.setCursor(0,16);
  display.print("M:");
  display.print(state1);
  display.print(" P:");
  display.print(state2);
  display.print(" X:");
  display.print(state3);

  display.display();
}

// sinric.pro
bool onPowerState(const String &deviceId, bool &state) {
  if (deviceId == device1) state1 = state;
  if (deviceId == device2) state2 = state;
  if (deviceId == device3) state3 = state;
  return true;
}

void setupSinric() {
  SinricProSwitch& sw1 = SinricPro[device1];
  SinricProSwitch& sw2 = SinricPro[device2];
  SinricProSwitch& sw3 = SinricPro[device3];

  sw1.onPowerState(onPowerState);
  sw2.onPowerState(onPowerState);
  sw3.onPowerState(onPowerState);

  SinricPro.begin(APP_KEY, APP_SECRET);
}

// touch sensor
bool touchPressed(int pin) {
  return touchRead(pin) < TOUCH_THRESHOLD;
}

// toggle jako eagle
void handleToggle(int pin, bool &state, String devId, String name) {
  static bool last = false;
  bool pressed = touchPressed(pin);

  if (pressed && !last) {
    state = !state;
    SinricProSwitch &sw = SinricPro[devId];
    sw.sendPowerStateEvent(state);
    showMessage(name + (state ? " ON" : " OFF"));
    delay(300);
  }
  last = pressed;
}

// impuls
void handleImpulse(int pin, String devId, String name) {
  static bool last = false;
  bool pressed = touchPressed(pin);

  if (pressed && !last) {
    SinricProSwitch &sw = SinricPro[devId];
    sw.sendPowerStateEvent(true);
    delay(200);
    sw.sendPowerStateEvent(false);
    showMessage(name);
  }
  last = pressed;
}

// setuip
void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  timeClient.begin();
  setupSinric();
}

// looop
void loop() {
  SinricPro.handle();
  timeClient.update();

  handleToggle(BTN1, state1, device1, "Monitor");
  handleToggle(BTN2, state2, device2, "Plug");
  handleToggle(BTN3, state3, device3, "Xbox");
  handleImpulse(BTN4, device4, "Spotify");

  drawDisplay();
}
