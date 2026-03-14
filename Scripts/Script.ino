//WIFI AND SOMETHING ELSE
#define WIFI_SSID   ""           // WIFI
#define WIFI_PASS   ""     // PASS
#define TZ_OFFSET   3600             // CET=3600, CEST=7200!!

// I2C pro displej
#define I2C_SDA     8
#define I2C_SCL     9

//nesahat!!
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <Preferences.h>
#include <Wire.h>
#include <WiFiUDP.h>
#include <time.h>
#include <esp_task_wdt.h>   

#include <SinricPro.h>
#include <SinricProLight.h>
#include <SinricProSwitch.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <NTPClient.h>

struct Config {
  char  wifi_ssid[64]          = ""; //!! add this !! very scary
  char  wifi_pass[64]          = "";
  char  sinric_key[64]         = "";
  char  sinric_secret[128]     = "";
  char  sinric_light_id[64]    = "";
  char  sinric_plug_id[64]     = "";
  char  bambu_ip[32]           = "";
  char  bambu_serial[32]       = "";
  char  bambu_code[32]         = "";
  unsigned long dbl_ms[4]      = {600, 600, 600, 600}; // 1,2,3,4x
  uint8_t  disp_brightness     = 200;
  bool     disp_on             = true;
  uint8_t  time_fmt            = 1;
  uint16_t plug_timer_min      = 5;
  int32_t  tz_offset           = TZ_OFFSET;
};

struct LogEntry {
  uint32_t ts;
  char     txt[88];
};

struct BtnState {
  bool          lastRaw    = LOW;
  bool          lastStable = LOW;
  unsigned long debounceT  = 0;
  unsigned long firstT     = 0;
  unsigned long pressStart = 0;
  int           pend       = 0;
  bool          waiting    = false;
  bool          holdFired  = false;
};

//sleeepp
bool     systemSleeping  = false;
bool     webStarted      = false;
bool     printScreenLocked = false;

// wifi stav
volatile bool wifiConnected    = false;
unsigned long wifiReconnectAt  = 0;   // recconect
int           wifiBackoffSec   = 5;   // exponenciální backoff: 5--10----20----30-----
unsigned long wifiConnectedAt  = 0;   // kdy jsme se naposledy připojili

//proměny
Config    cfg;
Preferences prefs;

// Hardware
const int      BTN_PINS[4]    = {2, 3, 4, 1};
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WebServer      server(80);

// Stav 
bool           lightOn        = false;
int            lightColorIdx  = 0;
bool           plugOn         = false;
bool           plugTimer      = false;
unsigned long  plugTimerEnd   = 0;
bool           lightTimer     = false;   // 2× BTN1 
unsigned long  lightTimerEnd  = 0;
bool           bambuLight     = false;
int            bambuSpeed     = 2;
float          bambuNozzle       = 0.0f;
float          bambuBed          = 0.0f;
float          bambuNozzleTarget = 0.0f;   // cílová teplota trysky (z Bambu MQTT)
float          bambuBedTarget    = 0.0f;   // cílová teplota podložky
int            bambuPct       = 0;
bool           bambuOnline    = false;
bool           bambuHoming    = false;
unsigned long  bambuHomingEnd = 0;
bool           bambuPrinting  = false;   // probíhá tisk????
int            bambuRemain    = 0;       // zbývající minuty
char           bambuJobName[40] = "";    // název souboru tisku
char           bambuState[16]   = "";    // IDLE/RUNNING/PAUSE/FAILED

const char* COLOR_NAMES[] = {"Bila","Cervena","Zelena","Modra","Oranzova","Fialova","Cyan"};
const uint8_t COLORS[7][3] = {
  {255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,140,0},{128,0,128},{0,255,255}
};
const char* SPEED_NAMES[] = {"","Silent","Standard","Sport","Ludicrous"};

// Displej
enum DispMode { DM_TIME, DM_WAIT, DM_DONE, DM_BAMBU, DM_PRINT };
DispMode       dm    = DM_TIME;
String         dmL1  = "";
String         dmL2  = "";
unsigned long  dmExp = 0;

// Log
#define HIST_MAX 60
#define ERR_MAX  20
LogEntry  histBuf[HIST_MAX]; int histHead = 0, histCnt = 0;
LogEntry  errBuf[ERR_MAX];   int errHead  = 0, errCnt  = 0;

// Časy
WiFiUDP      ntpUDP;
NTPClient    ntpCli(ntpUDP, "pool.ntp.org");
bool         ntpSynced = false;
unsigned long bootMs   = 0;

// BambuLab
WiFiClientSecure bambuTLS;
PubSubClient     bambuMqtt(bambuTLS);
unsigned long    bambuRetry   = 0;
unsigned long    bambuLastMsg = 0;   
bool             bambuConn   = false;

// FreeRTOS
TaskHandle_t     bambuTaskHandle = NULL;
volatile bool    bambuTaskConnect = false; // spoj se!!
SemaphoreHandle_t bambuMutex = NULL;       // ochrana bambuMqtt přístupu

// Tlačítka
BtnState btn[4];
#define TOUCH_DBN_MS  8      // kapacitní sensor – kratší debounce

//deklarace nezávislosti
void saveCfg();
void loadCfg();
void addHist(const char* t);
void addErr(const char* t);
void showMsg(DispMode m, const char* l1, const char* l2 = "", unsigned long ms = 3000);
void processClick(int idx, int clicks);
void bambuConnect();
void bambuTaskStart();
void bambuSend(const char* json);
void oledBoot(const char* line1, const char* line2 = "");
void renderDisplay();
void enterSleep();
void wakeUp();
void wifiInit();
void onWiFiEvent(WiFiEvent_t event);
String buildStatus();
String buildConfig();
String buildLogStr(LogEntry* buf, int head, int cnt, int maxN);
void webSetup();
void sinricSetup();

//flash
void loadCfg() {
  prefs.begin("sr", true);
  if (prefs.getBytesLength("c") == sizeof(Config))
    prefs.getBytes("c", &cfg, sizeof(Config));
  prefs.end();
}
void saveCfg() {
  prefs.begin("sr", false);
  prefs.putBytes("c", &cfg, sizeof(Config));
  prefs.end();
  Serial.println("[CFG] Flash uložen");
}

//logy jako logy, prostě
void addHist(const char* t) {
  histBuf[histHead].ts = ntpCli.getEpochTime();
  strlcpy(histBuf[histHead].txt, t, 87);
  histHead = (histHead + 1) % HIST_MAX;
  if (histCnt < HIST_MAX) histCnt++;
  Serial.printf("[H] %s\n", t);
}
void addErr(const char* t) {
  errBuf[errHead].ts = ntpCli.getEpochTime();
  strlcpy(errBuf[errHead].txt, t, 87);
  errHead = (errHead + 1) % ERR_MAX;
  if (errCnt < ERR_MAX) errCnt++;
  Serial.printf("[E] %s\n", t);
}

//128x32 displej používáme, takže nezapomenout a taky je to OLED
void showMsg(DispMode m, const char* l1, const char* l2, unsigned long ms) {
  if (printScreenLocked && m != DM_PRINT) return; // print screen zamčen 
  dm = m; dmL1 = l1; dmL2 = l2; dmExp = millis() + ms;
}

//HANDLER,...
void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnected   = true;
      wifiConnectedAt = millis();
      wifiBackoffSec  = 5;
      Serial.printf("[WiFi] Pripojeno! IP: %s  RSSI: %d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      if (!ntpSynced) {
        ntpCli.begin();
        ntpCli.setTimeOffset(cfg.tz_offset);
        ntpCli.update();
        if (ntpCli.getEpochTime() > 1000000) ntpSynced = true;
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnected = false;
      ntpSynced     = false;
      wifiReconnectAt = millis() + (unsigned long)wifiBackoffSec * 1000UL;
      wifiBackoffSec  = min(wifiBackoffSec * 2, 30);
      Serial.printf("[WiFi] Odpojeno – reconnect za %ds\n", wifiBackoffSec / 2);
      break;
    default: break;
  }
}

void wifiInit() {
  wifiConnected  = false;
  wifiBackoffSec = 5;
  WiFi.onEvent(onWiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  delay(150);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_17dBm);
  Serial.println("[WiFi] Zkousim saved creds...");
  WiFi.begin();
  unsigned long t0 = millis();
  while (!wifiConnected && millis() - t0 < 3000) delay(50);
  if (!wifiConnected) {
    Serial.printf("[WiFi] Explicitne: %s\n", cfg.wifi_ssid);
    WiFi.begin(cfg.wifi_ssid, cfg.wifi_pass);
  }
}

//OLED
static const int8_t SPIN_X[8] = { 0, 4, 5, 4, 0,-4,-5,-4};
static const int8_t SPIN_Y[8] = {-5,-4, 0, 4, 5, 4, 0,-4};
static uint8_t spinFrame = 0;


static void drawSpinner(int cx, int cy) {
  spinFrame = (spinFrame + 1) % 8;
  u8g2.drawCircle(cx, cy, 5, U8G2_DRAW_ALL);
  int hx = cx + SPIN_X[spinFrame];
  int hy = cy + SPIN_Y[spinFrame];
  u8g2.drawBox(hx - 1, hy - 1, 3, 3);
}

void oledBoot(const char* line1, const char* line2) {
  u8g2.clearBuffer();
  u8g2.drawRFrame(0, 0, 128, 32, 3);

  // ZAHLAVI
  u8g2.drawRBox(1, 1, 126, 13, 2);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_helvB08_tr);
  u8g2.drawStr((128 - u8g2.getStrWidth(line1)) / 2, 11, line1);
  u8g2.setDrawColor(1);

  // TEXT DOLE
  if (line2 && strlen(line2) > 0) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr((128 - u8g2.getStrWidth(line2)) / 2, 24, line2);
  }

  //VLEVO SPINNER
  drawSpinner(10, 25);

  // Tečky pokroku vpravo dole
  uint8_t dots = (spinFrame / 2) % 4;
  for (int d = 0; d < 3; d++) {
    int bx = 108 + d * 7;
    if (d < (int)dots) u8g2.drawRBox(bx, 27, 5, 3, 1);
    else               u8g2.drawRFrame(bx, 27, 5, 3, 1);
  }

  u8g2.sendBuffer();
}

//RENDER

// WiFi RSSI 
static void drawWiFiBars(int x, int y) {
  int bars = 0;
  if (wifiConnected) {
    int r = WiFi.RSSI();
    bars = (r > -60) ? 3 : (r > -75) ? 2 : 1;
  }
  for (int b = 0; b < 3; b++) {
    int bh = 2 + b * 2;
    int bx = x + b * 5;
    int by = y - bh;
    if (b < bars) u8g2.drawBox(bx, by, 4, bh);
    else          u8g2.drawFrame(bx, by, 4, bh);
  }
}

void renderDisplay() {
  if (!cfg.disp_on) { u8g2.setPowerSave(1); return; }
  u8g2.setPowerSave(0);
  u8g2.setContrast(cfg.disp_brightness);

  // Přepínání módů
  if (printScreenLocked) {
    dm = DM_PRINT;
  } else {
    if (dm != DM_TIME && dm != DM_PRINT && millis() > dmExp) dm = DM_TIME;
    if (dm == DM_TIME  && bambuPrinting)  dm = DM_PRINT;
    if (dm == DM_PRINT && !bambuPrinting) dm = DM_TIME;
  }
  u8g2.clearBuffer();

//DALŠI VĚCI K OLED
  if (dm == DM_TIME) {

    if (!ntpSynced) {
      // Čekací obrazovka
      u8g2.drawRFrame(0, 0, 128, 32, 3);
      u8g2.setFont(u8g2_font_helvB08_tr);
      const char* msg = !wifiConnected ? "Pripojuji WiFi..." : "Sync casu...";
      u8g2.drawStr((128 - u8g2.getStrWidth(msg)) / 2, 11, msg);
      // Spinner uprostřed 
      drawSpinner(64, 24);

    } else {
      int h = ntpCli.getHours(), m = ntpCli.getMinutes(), s = ntpCli.getSeconds();

      // Status bar y=0..9
      u8g2.drawDisc(4, 4, 2, U8G2_DRAW_ALL);          // NTP dot vlevo
      if (bambuConn) u8g2.drawDisc(64, 4, 2, U8G2_DRAW_ALL); // Bambu střed
      drawWiFiBars(110, 8);                             // WiFi vpravo
      u8g2.drawHLine(0, 9, 128);                       // oddělovač

      char tb[6]; snprintf(tb, sizeof(tb), "%02d:%02d", h, m);
      u8g2.setFont(u8g2_font_logisoso20_tr);
      int tw = u8g2.getStrWidth(tb);
      int tx = 4;
      u8g2.drawStr(tx, 31, tb);

      //SE--KUNDY
      char sb[3]; snprintf(sb, sizeof(sb), "%02d", s);
      u8g2.setFont(u8g2_font_helvB08_tr);
      int secX = tx + tw + 3;
      if (secX > 103) secX = 103;                      // nepřeteče za okraj
      u8g2.drawRFrame(secX, 21, 23, 10, 2);            // box 23×10
      // Blikající tep-tečka vlevo v boxu
      if (s % 2 == 1) u8g2.drawDisc(secX + 3, 26, 1, U8G2_DRAW_ALL);
      // Čísla sekund centrovaná v boxu
      int sw = u8g2.getStrWidth(sb);
      u8g2.drawStr(secX + (23 - sw) / 2, 29, sb);
    }

  } else if (dm == DM_PRINT && bambuPrinting) {

    const char* icon = (strcmp(bambuState, "PAUSE") == 0) ? "||" : ">";
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(0, 9, icon);

    char job[20] = "---";
    if (strlen(bambuJobName) > 0) {
      strlcpy(job, bambuJobName, sizeof(job));
      char* dot = strrchr(job, '.'); if (dot) *dot = '\0';
    }
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(12, 9, job);

    if (bambuRemain > 0) {
      char tbuf[10];
      if (bambuRemain >= 60) snprintf(tbuf, sizeof(tbuf), "%d:%02d", bambuRemain/60, bambuRemain%60);
      else                   snprintf(tbuf, sizeof(tbuf), "%dm", bambuRemain);
      u8g2.drawStr(128 - u8g2.getStrWidth(tbuf), 9, tbuf);
    }

    // Progress bar 
    u8g2.drawRFrame(0, 11, 128, 9, 2);
    int bw = (int)(bambuPct / 100.0f * 124.0f);
    if (bw > 0) u8g2.drawRBox(2, 13, bw, 5, 1);
    char pct[6]; snprintf(pct, sizeof(pct), "%d%%", bambuPct);
    u8g2.setFont(u8g2_font_5x7_tr);
    int px = (128 - u8g2.getStrWidth(pct)) / 2;
    u8g2.setDrawColor(0); u8g2.drawStr(px, 19, pct); u8g2.setDrawColor(1);
    if (bw + 2 < px + u8g2.getStrWidth(pct)) {
      u8g2.setClipWindow(bw + 2, 11, 127, 20);
      u8g2.drawStr(px, 19, pct);
      u8g2.setMaxClipWindow();
    }

    // Teploty 
    char nBuf[16], bBuf[16];
    if (bambuNozzleTarget > 5) snprintf(nBuf, sizeof(nBuf), "N:%.0f/%.0f", bambuNozzle, bambuNozzleTarget);
    else                       snprintf(nBuf, sizeof(nBuf), "N:%.0f",      bambuNozzle);
    if (bambuBedTarget > 5)    snprintf(bBuf, sizeof(bBuf), "B:%.0f/%.0f", bambuBed,    bambuBedTarget);
    else                       snprintf(bBuf, sizeof(bBuf), "B:%.0f",      bambuBed);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(0, 31, nBuf);
    u8g2.drawStr(128 - u8g2.getStrWidth(bBuf), 31, bBuf);
    u8g2.drawVLine(63, 21, 10);

  } else if (dm == DM_PRINT && !bambuPrinting) {

    u8g2.drawRFrame(0, 0, 128, 32, 3);
    u8g2.drawRBox(1, 1, 126, 13, 2);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_helvB08_tr);
    const char* ttl = "BAMBU  A1 MINI";
    u8g2.drawStr((128 - u8g2.getStrWidth(ttl)) / 2, 11, ttl);
    u8g2.setDrawColor(1);
    u8g2.drawHLine(1, 14, 126);

    u8g2.setFont(u8g2_font_helvB08_tr);
    char nL[16], bL[16];
    if (bambuNozzleTarget > 5) snprintf(nL, sizeof(nL), "N:%.0f/%.0f", bambuNozzle, bambuNozzleTarget);
    else                       snprintf(nL, sizeof(nL), "N:%.0f",      bambuNozzle);
    if (bambuBedTarget > 5)    snprintf(bL, sizeof(bL), "B:%.0f/%.0f", bambuBed,    bambuBedTarget);
    else                       snprintf(bL, sizeof(bL), "B:%.0f",      bambuBed);
    u8g2.drawStr(3, 23, nL);
    u8g2.drawStr(128 - u8g2.getStrWidth(bL) - 3, 23, bL);
    u8g2.drawVLine(63, 15, 8);

    const char* st  = (strlen(bambuState) > 0) ? bambuState : "---";
    const char* con = bambuConn ? "WiFi OK" : "Offline";
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(3, 31, st);
    u8g2.drawStr(128 - u8g2.getStrWidth(con) - 3, 31, con);
    u8g2.drawVLine(63, 24, 7);

  } else {
    u8g2.drawRFrame(0, 0, 128, 32, 3);
    u8g2.drawRBox(1, 1, 126, 13, 2);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr((128 - u8g2.getStrWidth(dmL1.c_str())) / 2, 11, dmL1.c_str());
    u8g2.setDrawColor(1);
    if (dmL2.length() > 0) {
      u8g2.setFont(u8g2_font_helvB08_tr);
      u8g2.drawStr((128 - u8g2.getStrWidth(dmL2.c_str())) / 2, 28, dmL2.c_str());
    }
  }

  u8g2.sendBuffer();
}


//  BambuLab MQTT

void bambuCallback(char* topic, byte* payload, unsigned int len) {
  if (len > 1400) return;
  bambuLastMsg = millis(); // RESET
  char buf[1401]; memcpy(buf, payload, len); buf[len] = 0;
  StaticJsonDocument<1400> doc;
  if (deserializeJson(doc, buf) != DeserializationError::Ok) return;
  if (doc.containsKey("print")) {
    JsonObject p = doc["print"];
    if (p.containsKey("nozzle_temper"))          bambuNozzle       = p["nozzle_temper"].as<float>();
    if (p.containsKey("bed_temper"))             bambuBed          = p["bed_temper"].as<float>();
    if (p.containsKey("nozzle_target_temper"))   bambuNozzleTarget = p["nozzle_target_temper"].as<float>();
    if (p.containsKey("bed_target_temper"))      bambuBedTarget    = p["bed_target_temper"].as<float>();
    if (p.containsKey("mc_percent"))             bambuPct          = p["mc_percent"].as<int>();
    if (p.containsKey("mc_remaining_time")) bambuRemain = p["mc_remaining_time"].as<int>();

    // Název tiskového jobu
    if (p.containsKey("subtask_name")) {
      const char* jn = p["subtask_name"];
      if (jn && strlen(jn) > 0) strlcpy(bambuJobName, jn, sizeof(bambuJobName));
    }

    // Stav tisku – rozhoduje co zobrazit na OLED
    if (p.containsKey("gcode_state")) {
      const char* gs = p["gcode_state"];
      strlcpy(bambuState, gs ? gs : "", sizeof(bambuState));

      bool wasPrinting = bambuPrinting;
      bambuPrinting = (strcmp(bambuState, "RUNNING") == 0 ||
                       strcmp(bambuState, "PAUSE")   == 0);

      // Homing hotov
      if (strcmp(bambuState, "IDLE") == 0 && bambuHoming) {
        bambuHoming = false;
        addHist("Bambu: homing hotov");
      }
      // Automaticky přepni na print screen při startu tisku
      if (bambuPrinting && !wasPrinting) {
        char h[60]; snprintf(h, sizeof(h), "Tisk zahajen: %s", bambuJobName);
        addHist(h);
        dm = DM_PRINT; // automaticky zobraz progress
      }
      // Tisk skončil 
      if (!bambuPrinting && wasPrinting) {
        dm = DM_TIME;
        addHist("Tisk dokoncen");
      }
    }
  }
  bambuOnline = true;
}


void bambuTask(void* param) {
  // DISK 0
  for (;;) {
    // Čekej na signál z loop()
    if (bambuTaskConnect && wifiConnected) {
      bambuTaskConnect = false;

      if (xSemaphoreTake(bambuMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        Serial.println("[Bambu] Task: TLS connect...");
        bambuTLS.setInsecure();
        bambuTLS.setHandshakeTimeout(10);
        bambuMqtt.setServer(cfg.bambu_ip, 8883);
        bambuMqtt.setCallback(bambuCallback);
        bambuMqtt.setBufferSize(2048);
        bambuMqtt.setKeepAlive(60);   // 60s keepalive – zabrání timeoutům
        bambuMqtt.setSocketTimeout(5); // 5s socket timeout
        String cid = "sr_" + String(cfg.bambu_serial);

        if (bambuMqtt.connect(cid.c_str(), "bblp", cfg.bambu_code)) {
          char sub[80];
          snprintf(sub, sizeof(sub), "device/%s/report", cfg.bambu_serial);
          bambuMqtt.subscribe(sub);
          bambuConn    = true;
          bambuLastMsg = millis();
          Serial.println("[Bambu] Task: Pripojeno!");
          addHist("BambuLab: pripojeno");
        } else {
          Serial.printf("[Bambu] Task: Selhalo, stav %d\n", bambuMqtt.state());
          bambuConn = false;
        }
        xSemaphoreGive(bambuMutex);
      }
    }

    // Udržuj MQTT alive
    if (bambuMqtt.connected()) {
      if (xSemaphoreTake(bambuMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        bambuMqtt.loop();
        xSemaphoreGive(bambuMutex);
      }
    } else if (bambuConn) {
      // PubSubClient hlásí odpojení – aktualizuj flag
      bambuConn = false;
      Serial.println("[Bambu] Task: Detekováno odpojení");
    }

    vTaskDelay(pdMS_TO_TICKS(10)); //10 mS
  }
}

void bambuTaskStart() {
  bambuMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(
    bambuTask,        // funkce
    "BambuTask",      // název
    8192,             // stack 
    NULL,             // parametr
    1,                // priorita
    &bambuTaskHandle, // handle
    0                 // core 0 
  );
  Serial.println("[Bambu] Task spusten na core 0");
}

void bambuConnect() {
  // Pouze nastav příznak 
  if (strlen(cfg.bambu_ip) < 4) return;
  bambuTaskConnect = true;
}

void bambuSend(const char* json) {
  if (!bambuMqtt.connected()) { showMsg(DM_BAMBU, "Bambu", "Offline"); return; }
  char topic[80];
  snprintf(topic, sizeof(topic), "device/%s/request", cfg.bambu_serial);
  if (xSemaphoreTake(bambuMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    bambuMqtt.publish(topic, json);
    xSemaphoreGive(bambuMutex);
  }
}

//sinric, panebože
bool onLightPower(const String& id, bool& state) {
  lightOn = state;
  char h[50]; snprintf(h, sizeof(h), "Svetlo cloud: %s", state ? "ZAP" : "VYP"); addHist(h);
  return true;
}
bool onLightColor(const String& id, uint8_t& r, uint8_t& g, uint8_t& b) {
  char h[50]; snprintf(h, sizeof(h), "Barva R%d G%d B%d", r, g, b); addHist(h);
  return true;
}
bool onPlugPower(const String& id, bool& state) {
  plugOn = state; if (!state) plugTimer = false;
  char h[50]; snprintf(h, sizeof(h), "Zasuvka cloud: %s", state ? "ZAP" : "VYP"); addHist(h);
  return true;
}

void sinricSetup() {
  if (strlen(cfg.sinric_light_id) > 3) {
    SinricProLight& L = SinricPro[cfg.sinric_light_id];
    L.onPowerState(onLightPower);
    L.onColor(onLightColor);
  }
  if (strlen(cfg.sinric_plug_id) > 3) {
    SinricProSwitch& P = SinricPro[cfg.sinric_plug_id];
    P.onPowerState(onPlugPower);
  }
  if (strlen(cfg.sinric_key) > 3 && strlen(cfg.sinric_secret) > 3) {
    SinricPro.onConnected([]    { Serial.println("[Sinric] Pripojeno!"); });
    SinricPro.onDisconnected([] { Serial.println("[Sinric] Odpojeno"); addErr("SinricPro odpojen"); });
    SinricPro.restoreDeviceStates(true); // obnov stav po reconnect
    SinricPro.begin(cfg.sinric_key, cfg.sinric_secret);
    Serial.printf("[Sinric] Key: %.8s... zahájena inicializace\n", cfg.sinric_key);
  } else {
    Serial.println("[Sinric] Přeskakuji – chybí credentials");
  }
}

//  Logika tlačítek
void processClick(int idx, int clicks) {
  char h[90];

  if (idx == 0) {
    if (clicks == 1) {
      lightOn = !lightOn;
      SinricProLight& L = SinricPro[cfg.sinric_light_id];
      L.sendPowerStateEvent(lightOn);
      snprintf(h, sizeof(h), "Svetlo: %s", lightOn ? "ZAP" : "VYP"); addHist(h);
      showMsg(DM_DONE, "Svetlo", lightOn ? "ZAPNUTO" : "VYPNUTO");

    } else if (clicks == 2) {
      // Timer 
      if (!lightOn) {
        lightOn = true;
        SinricProLight& L = SinricPro[cfg.sinric_light_id];
        L.sendPowerStateEvent(true);
      }
      lightTimer    = true;
      lightTimerEnd = millis() + 5UL * 60000UL;
      addHist("Svetlo timer: vyp za 5min");
      showMsg(DM_WAIT, "Svetlo timer", "Vyp za 5 min", 3000);

    } else if (clicks == 3) {
      // Reset na čistou bílou, plný jas
      lightColorIdx = 0; // index 0 = Bila {255,255,255}
      SinricProLight& L = SinricPro[cfg.sinric_light_id];
      if (!lightOn) { lightOn = true; L.sendPowerStateEvent(true); }
      L.sendColorEvent(255, 255, 255);
      addHist("3x BTN1: Bila svetlo");
      showMsg(DM_DONE, "Svetlo", "Bila / MAX");
    }

  } else if (idx == 1) {
    if (clicks == 1) {
      plugOn = !plugOn; plugTimer = false;
      SinricProSwitch& P = SinricPro[cfg.sinric_plug_id];
      P.sendPowerStateEvent(plugOn);
      snprintf(h, sizeof(h), "Zasuvka: %s", plugOn ? "ZAP" : "VYP"); addHist(h);
      showMsg(DM_DONE, "Zasuvka", plugOn ? "ZAPNUTA" : "VYPNUTA");

    } else if (clicks == 2) {
      if (!plugOn) { plugOn = true; SinricProSwitch& P = SinricPro[cfg.sinric_plug_id]; P.sendPowerStateEvent(true); }
      plugTimer = true;
      plugTimerEnd = millis() + (unsigned long)cfg.plug_timer_min * 60000UL;
      snprintf(h, sizeof(h), "Timer %u min", cfg.plug_timer_min); addHist(h);
      char l2[20]; snprintf(l2, sizeof(l2), "Vyp za %u min", cfg.plug_timer_min);
      showMsg(DM_WAIT, "Timer zasuvka", l2, 3000);

    } else if (clicks == 3) {
      plugTimer = false;
      addHist("3x BTN2: Timer zrusen");
      showMsg(DM_DONE, "Timer", "ZRUSEN");
    }

  } else if (idx == 2) {
    if (clicks == 1) {
      // Světlo – vždy povoleno
      bambuLight = !bambuLight;
      char cmd[220];
      snprintf(cmd, sizeof(cmd),
        "{\"system\":{\"sequence_id\":\"2001\",\"command\":\"ledctrl\","
        "\"led_node\":\"work_light\",\"led_mode\":\"%s\","
        "\"led_on_time\":500,\"led_off_time\":500,\"loop_times\":0,\"interval_time\":0}}",
        bambuLight ? "on" : "off");
      bambuSend(cmd);
      snprintf(h, sizeof(h), "Bambu svetlo: %s", bambuLight ? "ZAP" : "VYP"); addHist(h);
      showMsg(DM_BAMBU, "Bambu svetlo", bambuLight ? "ZAP" : "VYP");

    } else if (clicks == 2) {
      // Předehřev – BLOKOVÁNO při tisku 
      if (bambuPrinting) {
        showMsg(DM_BAMBU, "TISK PROBIHA", "Teplota zablokov.");
        addHist("BTN3 2x: blok (tisk)");
        return;
      }
      bambuSend("{\"print\":{\"sequence_id\":\"1001\",\"command\":\"gcode_line\","
                "\"param\":\"M140 S67\\nM104 S230\\n\"}}");
      addHist("Bambu: predehrev 230/67");
      showMsg(DM_BAMBU, "Predehrev", "230C / 67C", 3000);

    } else if (clicks == 3) {
      // Zrušit předehřev – BLOKOVÁNO při tisku
      if (bambuPrinting) {
        showMsg(DM_BAMBU, "TISK PROBIHA", "Teplota zablokov.");
        addHist("BTN3 3x: blok (tisk)");
        return;
      }
      bambuSend("{\"print\":{\"sequence_id\":\"1001\",\"command\":\"gcode_line\","
                "\"param\":\"M104 S0\\nM140 S0\\n\"}}");
      addHist("3x BTN3: Predehrev ZRUSEN");
      showMsg(DM_BAMBU, "Predehrev", "ZRUSEN 0/0", 3000);
    }

  } else if (idx == 3) {
    if (clicks == 1) {
      // Homing – BLOKOVÁNO při tisku (pohyb za tisku = zničený výtisk)
      if (bambuPrinting) {
        showMsg(DM_BAMBU, "TISK PROBIHA", "Homing zabl.");
        addHist("BTN4 1x: homing blok (tisk)");
        return;
      }
      if (bambuHoming) { showMsg(DM_BAMBU, "Homing...", "Probiha!"); return; }
      bambuSend("{\"print\":{\"sequence_id\":\"1002\",\"command\":\"gcode_line\","
                "\"param\":\"G28\\n\"}}");
      bambuHoming = true; bambuHomingEnd = millis() + 40000;
      addHist("Bambu: G28 homing");
      showMsg(DM_BAMBU, "Bambu", "Homing...", 5000);

    } else if (clicks == 2) {
      // Rychlost 
      bambuSpeed = (bambuSpeed % 4) + 1;
      char cmd[160];
      snprintf(cmd, sizeof(cmd),
        "{\"print\":{\"sequence_id\":\"1003\",\"command\":\"print_speed\","
        "\"param\":\"%d\"}}", bambuSpeed);
      bambuSend(cmd);
      snprintf(h, sizeof(h), "Bambu speed: %s", SPEED_NAMES[bambuSpeed]); addHist(h);
      showMsg(DM_BAMBU, "Rychlost", SPEED_NAMES[bambuSpeed]);

    } else if (clicks == 3) {
      // Present plate 
      if (bambuPrinting) {
        showMsg(DM_BAMBU, "TISK PROBIHA", "Pohyb zabl.");
        addHist("BTN4 3x: present zabl (tisk)");
        return;
      }
      // A1 Mini: Z max = 180mm, jedeme na 165mm (15mm rezerva)
      bambuSend("{\"print\":{\"sequence_id\":\"1004\",\"command\":\"gcode_line\","
                "\"param\":\"G28 Z\\nG1 Z165 F1200\\nG1 Y175 F6000\\nM400\\n\"}}");
      addHist("3x BTN4: Present plate A1Mini");
      showMsg(DM_BAMBU, "Present plate", "Z=165 Y=175", 4000);
    }
  }
}

//slep
#define LONG_PRESS_MS 5000UL

void enterSleep() {
  Serial.println("[SLEEP] Uspavam...");
  systemSleeping = true;

  // Odpočítání na displeji
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB08_tr);
  const char* msg = "Uspano";
  u8g2.drawStr((128 - u8g2.getStrWidth(msg)) / 2, 18, msg);
  u8g2.sendBuffer();
  delay(500);
  
  // OLED off
  u8g2.setPowerSave(1);

  // BambuLab odpojit nejdřív (čisté ukončení MQTT)
  if (bambuConn) {
    bambuMqtt.disconnect();
    bambuConn = false;
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  Serial.println("[SLEEP] Hotovo. Drz BTN1 5s pro probuzeni.");
}

void wakeUp() {
  Serial.println("[WAKE] Probouzim...");
  systemSleeping = false;
  printScreenLocked = false;
  ntpSynced = false;

  u8g2.setPowerSave(0);
  u8g2.setContrast(cfg.disp_brightness);
  oledBoot("Smart Remote", "Probouzim...");

  // wifi
  wifiInit();

  // Čekej max 12s s animací
  unsigned long t0 = millis();
  while (!wifiConnected && millis() - t0 < 12000) {
    oledBoot("WiFi...", cfg.wifi_ssid);
    delay(250);
  }

  if (wifiConnected) {
    if (!ntpSynced) {
      ntpCli.setTimeOffset(cfg.tz_offset);
      ntpCli.update();
      if (ntpCli.getEpochTime() > 1000000) ntpSynced = true;
    }
    if (!webStarted) { webSetup(); webStarted = true; }
    if (strlen(cfg.bambu_ip) > 4) bambuRetry = millis();
    Serial.printf("[WAKE] OK – IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[WAKE] WiFi selhalo – loop se postara o reconnect");
    wifiReconnectAt = millis() + 5000; // loop zkusí za 5s
  }

  dm = DM_TIME;
}

//  Obsluha tlačítek 
void handleBtns() {
  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    bool raw = digitalRead(BTN_PINS[i]);

    // debounce
    if (raw != btn[i].lastRaw) { btn[i].lastRaw = raw; btn[i].debounceT = now; }
    if (now - btn[i].debounceT < TOUCH_DBN_MS) continue;

    bool stable  = (raw == HIGH);
    bool rising  = (stable && !btn[i].lastStable);
    bool falling = (!stable && btn[i].lastStable);
    btn[i].lastStable = stable;

    // náběžná hrana
    if (rising) {
      btn[i].pressStart = now;
      btn[i].holdFired  = false;
      if (!systemSleeping) {
        btn[i].pend++;
        if (!btn[i].waiting) {
          btn[i].waiting = true;
          btn[i].firstT  = now;
        } else {
          // ── KLOUZAVÉ OKNO ──
          btn[i].firstT = now;
        }
        // Max 3 kliky – víc nedetekujeme, zpracuj hned
        if (btn[i].pend >= 3) {
          processClick(i, btn[i].pend);
          btn[i].pend    = 0;
          btn[i].waiting = false;
        }
      }
    }

    // ── LONG PRESS BTN1 
    if (i == 0 && stable && !btn[i].holdFired
        && (now - btn[i].pressStart) >= LONG_PRESS_MS) {
      btn[i].holdFired = true;
      btn[i].pend      = 0;
      btn[i].waiting   = false;
      if (systemSleeping) wakeUp(); else enterSleep();
    }

    // ── LONG PRESS BTN3 
    if (i == 2 && stable && !btn[i].holdFired
        && (now - btn[i].pressStart) >= 2000UL) {
      btn[i].holdFired = true;
      btn[i].pend      = 0;
      btn[i].waiting   = false;
      printScreenLocked = !printScreenLocked;
      dm = printScreenLocked ? DM_PRINT : DM_TIME;
      Serial.printf("[DISP] Bambu info %s\n", printScreenLocked ? "ZAP" : "VYP");
    }

    // ── LONG PRESS BTN4 (idx 3) = Present plate (A1 Mini) ──
    // A1 Mini: Z max=180mm, jedeme na Z=160 (20mm rezerva nahoře!)
    // Y=175 = podložka dopředu (A1 Mini Y max=180mm)
    // POZOR: blokováno při tisku!
    if (i == 3 && stable && !btn[i].holdFired
        && (now - btn[i].pressStart) >= 2000UL) {
      btn[i].holdFired = true;
      btn[i].pend      = 0;
      btn[i].waiting   = false;
      if (bambuPrinting) {
        showMsg(DM_BAMBU, "TISK PROBIHA", "Pohyb zabl.");
      } else {
        // G28 Z 
        bambuSend("{\"print\":{\"sequence_id\":\"2010\",\"command\":\"gcode_line\","
                  "\"param\":\"G28 Z\\nG1 Z160 F1200\\nG1 Y175 F6000\\nM400\\n\"}}");
        addHist("Hold BTN4: Present plate Z160");
        showMsg(DM_BAMBU, "Present plate", "Z=160  Y=175", 4000);
        Serial.println("[BTN4 hold] Present plate Z160 Y175");
      }
    }

    // při puštění po long-press 
    if (falling && btn[i].holdFired) {
      btn[i].pend    = 0;
      btn[i].waiting = false;
    }

    // Normální klik – zpracuj až po PUŠTĚNÍ tlačítka + uplynutí okna
    // !btn[i].lastStable = tlačítko není drženo → nesprouštíme klik při hold
    if (!systemSleeping && btn[i].waiting && btn[i].pend > 0
        && !btn[i].lastStable
        && (now - btn[i].firstT) > cfg.dbl_ms[i]) {
      processClick(i, btn[i].pend);
      btn[i].pend    = 0;
      btn[i].waiting = false;
    }
  }
}

//json
String buildStatus() {
  unsigned long s = (millis() - bootMs) / 1000;
  unsigned long mm = s / 60; s %= 60; unsigned long hh = mm / 60; mm %= 60;
  char upt[24]; snprintf(upt, sizeof(upt), "%luh %02lu:%02lu", hh, mm, s);

  char buf[700];
  snprintf(buf, sizeof(buf),
    "{\"light\":%s,\"lightCol\":\"%s\","
    "\"plug\":%s,\"plugTimer\":%s,\"plugLeft\":%lu,"
    "\"bLight\":%s,\"bSpeed\":%d,\"bSpeedN\":\"%s\","
    "\"bNozzle\":%.1f,\"bBed\":%.1f,\"bPct\":%d,\"bOnline\":%s,\"bHoming\":%s,"
    "\"bPrinting\":%s,\"bRemain\":%d,\"bState\":\"%s\",\"bJob\":\"%s\","
    "\"uptime\":\"%s\",\"rssi\":%d,\"ip\":\"%s\",\"ssid\":\"%s\","
    "\"ntpOk\":%s,\"h\":%d,\"m\":%d,\"sec\":%d}",
    lightOn     ? "true":"false", COLOR_NAMES[lightColorIdx],
    plugOn      ? "true":"false", plugTimer ? "true":"false",
    plugTimer   ? (plugTimerEnd - millis()) / 1000UL : 0UL,
    bambuLight  ? "true":"false", bambuSpeed, SPEED_NAMES[bambuSpeed],
    bambuNozzle, bambuBed, bambuPct,
    bambuOnline ? "true":"false", bambuHoming ? "true":"false",
    bambuPrinting ? "true":"false", bambuRemain, bambuState, bambuJobName,
    upt, WiFi.RSSI(), WiFi.localIP().toString().c_str(), cfg.wifi_ssid,
    ntpSynced   ? "true":"false",
    ntpCli.getHours(), ntpCli.getMinutes(), ntpCli.getSeconds()
  );
  return String(buf);
}

String buildLogStr(LogEntry* buf, int head, int cnt, int maxN) {
  String out = "["; int n = min(cnt, maxN);
  for (int i = 0; i < n; i++) {
    int idx = ((head - n + i) + maxN) % maxN;
    if (i) out += ',';
    // Escapujeme apostrofy v textu
    String txt = buf[idx].txt;
    txt.replace("\"", "'");
    out += "{\"ts\":" + String(buf[idx].ts) + ",\"t\":\"" + txt + "\"}";
  }
  return out + "]";
}

String buildConfig() {
  char b[520];
  snprintf(b, sizeof(b),
    "{\"ssid\":\"%s\",\"skey\":\"%s\",\"slid\":\"%s\",\"spid\":\"%s\","
    "\"bip\":\"%s\",\"bser\":\"%s\","
    "\"br\":%d,\"don\":%s,\"tfmt\":%d,\"ptmr\":%d,\"tz\":%ld,"
    "\"dbl\":[%lu,%lu,%lu,%lu]}",
    cfg.wifi_ssid, cfg.sinric_key, cfg.sinric_light_id, cfg.sinric_plug_id,
    cfg.bambu_ip, cfg.bambu_serial,
    cfg.disp_brightness, cfg.disp_on?"true":"false", cfg.time_fmt,
    cfg.plug_timer_min, (long)cfg.tz_offset,
    cfg.dbl_ms[0], cfg.dbl_ms[1], cfg.dbl_ms[2], cfg.dbl_ms[3]);
  return String(b);
}


//  HTML stránka -nenenenennenenne
static const char HTML_PAGE[] PROGMEM = R"SRHTML(<!DOCTYPE html>
<html lang="cs">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Remote</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{--bg:#0c0c0c;--c1:#161616;--c2:#1f1f1f;--c3:#282828;--bd:#303030;--bd2:#404040;--tx:#eeeeee;--tx2:#777;--tx3:#444;--ok:#4ade80;--er:#f87171;--ac:#60a5fa;--r:10px}
html,body{background:var(--bg);color:var(--tx);font-family:system-ui,sans-serif;font-size:13px;height:100%}
.hdr{display:flex;align-items:center;padding:10px 14px;background:var(--c1);border-bottom:1px solid var(--bd);gap:8px;position:sticky;top:0;z-index:99}
.hdr-t{font-weight:700;font-size:14px;flex:1;font-family:monospace}
.hdr-ip{font-size:10px;color:var(--tx2);font-family:monospace}
.led{width:6px;height:6px;border-radius:50%;background:var(--bd);transition:.3s}
.led.on{background:var(--ok);box-shadow:0 0 5px var(--ok)}
.tabs{display:flex;background:var(--c1);border-bottom:1px solid var(--bd);padding:0 12px}
.tab{padding:8px 14px;border:none;background:none;color:var(--tx2);font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:.08em;cursor:pointer;border-bottom:2px solid transparent;position:relative;bottom:-1px;font-family:monospace;transition:.15s}
.tab:hover{color:var(--tx)}.tab.a{color:var(--tx);border-bottom-color:var(--ac)}
.pg{display:none;padding:14px;max-width:800px;margin:0 auto}.pg.a{display:block}
.card{background:var(--c1);border:1px solid var(--bd);border-radius:var(--r);margin-bottom:10px;overflow:hidden}
.ch{display:flex;align-items:center;gap:7px;padding:9px 13px;background:var(--c2);border-bottom:1px solid var(--bd)}
.ct{font-size:9px;font-weight:700;letter-spacing:.1em;text-transform:uppercase;color:var(--tx2);font-family:monospace}
.cb{padding:13px}
.g2{display:grid;grid-template-columns:1fr 1fr;gap:9px}
@media(max-width:540px){.g2{grid-template-columns:1fr}}
.bc{border:1px solid var(--bd);border-radius:var(--r);padding:11px;display:flex;flex-direction:column;gap:7px;transition:.15s}
.bc:hover{border-color:var(--bd2)}
.bh{display:flex;align-items:center;gap:7px}
.bnum{width:22px;height:22px;border-radius:50%;background:var(--c3);display:flex;align-items:center;justify-content:center;font-size:10px;font-weight:700;font-family:monospace;flex-shrink:0}
.bname{font-weight:600;font-size:12px}.bsub{font-size:9px;color:var(--tx3)}.bstat{margin-left:auto}
.row{display:flex;align-items:center;gap:5px;padding:5px 0;border-top:1px solid var(--bd)}
.rl{font-size:9px;color:var(--tx3);text-transform:uppercase;letter-spacing:.05em;min-width:45px;font-family:monospace}
.rd{flex:1;font-size:11px}
.pill{padding:1px 7px;border-radius:16px;font-size:9px;font-weight:700;text-transform:uppercase;font-family:monospace}
.p-on{background:rgba(74,222,128,.12);color:var(--ok);border:1px solid rgba(74,222,128,.3)}
.p-off{background:var(--c3);color:var(--tx3);border:1px solid var(--bd)}
.p-ac{background:rgba(96,165,250,.12);color:var(--ac);border:1px solid rgba(96,165,250,.3)}
.btn{padding:5px 11px;background:var(--c2);border:1px solid var(--bd);border-radius:6px;color:var(--tx);font-size:11px;cursor:pointer;font-weight:500;transition:.15s;white-space:nowrap;font-family:inherit}
.btn:hover{background:var(--c3);border-color:var(--bd2)}
.btn-p{background:var(--tx);color:#000}.btn-p:hover{background:#ddd}
.btn-d{color:var(--er);border-color:rgba(248,113,113,.3)}.btn-d:hover{background:rgba(248,113,113,.07)}
.btn-s{padding:3px 9px;font-size:10px}
.inp{background:var(--c2);border:1px solid var(--bd);border-radius:6px;color:var(--tx);padding:6px 9px;font-size:12px;width:100%;outline:none;font-family:inherit;transition:.15s}
.inp:focus{border-color:var(--bd2);box-shadow:0 0 0 2px rgba(96,165,250,.12)}
.inp-s{width:60px;padding:3px 7px;font-size:11px}
.sel{background:var(--c2);border:1px solid var(--bd);border-radius:6px;color:var(--tx);padding:6px 9px;font-size:12px;cursor:pointer;outline:none;font-family:inherit}
.fld{display:flex;flex-direction:column;gap:4px;margin-bottom:10px}
.flbl{font-size:9px;color:var(--tx2);text-transform:uppercase;letter-spacing:.07em;font-family:monospace}
.rrow{display:flex;align-items:center;gap:7px}
.rng{-webkit-appearance:none;flex:1;height:2px;background:var(--c3);border-radius:1px;cursor:pointer;outline:none}
.rng::-webkit-slider-thumb{-webkit-appearance:none;width:12px;height:12px;background:var(--tx);border-radius:50%}
.rv{font-size:10px;color:var(--tx2);min-width:24px;text-align:right;font-family:monospace}
.tog{position:relative;width:34px;height:18px;flex-shrink:0}
.tog input{opacity:0;width:0;height:0}
.tsl{position:absolute;inset:0;background:var(--c3);border-radius:18px;border:1px solid var(--bd);cursor:pointer;transition:.15s}
.tsl::before{content:'';position:absolute;width:11px;height:11px;left:3px;top:3px;background:var(--tx3);border-radius:50%;transition:.15s}
.tog input:checked+.tsl{border-color:var(--ok)}
.tog input:checked+.tsl::before{transform:translateX(15px);background:var(--ok)}
.hl{max-height:260px;overflow-y:auto}
.hrow{display:flex;gap:7px;padding:7px 13px;border-bottom:1px solid var(--bd);font-size:11px}
.hrow:last-child{border-bottom:none}
.hts{color:var(--tx2);min-width:60px;font-size:9px;font-family:monospace;flex-shrink:0;padding-top:1px}
.htx{color:var(--tx);line-height:1.4}
.empty{padding:20px;text-align:center;color:var(--tx3);font-size:11px}
.ir{display:flex;justify-content:space-between;align-items:center;padding:7px 0;border-bottom:1px solid var(--bd)}
.ir:last-child{border-bottom:none}
.ik{color:var(--tx2);font-size:11px}.iv{font-size:11px;font-weight:500}
.sig{display:flex;gap:2px;align-items:flex-end;height:12px}
.sb{width:3px;background:var(--c3);border-radius:1px}
.sb:nth-child(1){height:3px}.sb:nth-child(2){height:6px}.sb:nth-child(3){height:9px}.sb:nth-child(4){height:12px}
.sb.on{background:var(--ok)}
.upz{border:2px dashed var(--bd);border-radius:var(--r);padding:20px;text-align:center;cursor:pointer;transition:.15s}
.upz:hover,.upz.dv{border-color:var(--bd2);background:rgba(255,255,255,.02)}
.upz-i{font-size:24px;margin-bottom:6px}
.upz-t{color:var(--tx2);font-size:11px}
.prg{height:3px;background:var(--c3);border-radius:2px;overflow:hidden;margin-top:8px}
.prgf{height:100%;background:var(--ac);width:0%;transition:width .3s}
#con{background:#000;padding:9px;font-family:monospace;font-size:10px;color:#4ade80;min-height:80px;max-height:180px;overflow-y:auto;border-radius:0 0 var(--r) var(--r)}
.bst{text-align:center;padding:10px 0}
.bstn{font-size:26px;font-weight:700;font-family:monospace}
.bstl{font-size:9px;color:var(--tx2);margin-top:2px;letter-spacing:.07em;text-transform:uppercase}
.note{background:rgba(96,165,250,.07);border:1px solid rgba(96,165,250,.2);border-radius:6px;padding:8px 10px;font-size:11px;color:var(--ac);margin-bottom:10px;line-height:1.5}
#toast{position:fixed;bottom:14px;right:14px;background:var(--c2);border:1px solid var(--bd);border-radius:7px;padding:7px 12px;font-size:11px;transform:translateY(8px);opacity:0;transition:.15s;z-index:999;pointer-events:none}
#toast.sh{transform:translateY(0);opacity:1}
#toast.ok{border-color:rgba(74,222,128,.4);color:var(--ok)}
#toast.er{border-color:rgba(248,113,113,.4);color:var(--er)}
::-webkit-scrollbar{width:4px}::-webkit-scrollbar-track{background:var(--c1)}::-webkit-scrollbar-thumb{background:var(--bd);border-radius:2px}
</style>
</head>
<body>
<div class="hdr">
  <div class="hdr-t">Smart Remote</div>
  <span class="hdr-ip" id="hdr-ip"></span>
  <div class="led" id="led-w" title="WiFi"></div>
  <div class="led" id="led-b" title="BambuLab"></div>
</div>
<div class="tabs">
  <button class="tab a" onclick="gTab('b',this)">Tla&#x010D;&#xED;tka</button>
  <button class="tab" onclick="gTab('s',this)">Script</button>
  <button class="tab" onclick="gTab('i',this)">Info</button>
</div>

<!-- ═══ TAB TLAČÍTKA ═══ -->
<div class="pg a" id="pg-b">
  <div class="g2">
    <div class="bc">
      <div class="bh">
        <div class="bnum">1</div>
        <div><div class="bname">Sv&#x11B;tlo</div><div class="bsub">SinricPro &middot; GPIO 2</div></div>
        <div class="bstat"><span class="pill p-off" id="p-l">OFF</span></div>
      </div>
      <div class="row"><span class="rl">1&times;</span><span class="rd">Zap / Vyp</span><button class="btn btn-s" onclick="sim(0,1)">&#x25B6;</button></div>
      <div class="row"><span class="rl">2&times;</span><span class="rd">Barva: <b id="lcol">&#x2013;</b></span><button class="btn btn-s" onclick="sim(0,2)">&#x25B6;</button></div>
      <div class="row" style="border-top:none;padding-top:0;flex-wrap:wrap;gap:5px">
        <span style="font-size:9px;color:var(--tx3);font-family:monospace">Okno 2&times;:</span>
        <input class="inp inp-s" type="number" id="d0" min="100" max="1500" value="400">
        <span style="font-size:9px;color:var(--tx3)">ms</span>
        <button class="btn btn-s" onclick="saveDly(0)">OK</button>
      </div>
    </div>
    <div class="bc">
      <div class="bh">
        <div class="bnum">2</div>
        <div><div class="bname">Z&#xE1;suvka</div><div class="bsub">SinricPro &middot; GPIO 3</div></div>
        <div class="bstat"><span class="pill p-off" id="p-p">OFF</span></div>
      </div>
      <div class="row"><span class="rl">1&times;</span><span class="rd">Zap / Vyp</span><button class="btn btn-s" onclick="sim(1,1)">&#x25B6;</button></div>
      <div class="row">
        <span class="rl">2&times;</span>
        <span class="rd">Timer: <input class="inp inp-s" id="tmr-m" type="number" min="1" max="180" value="5"> min</span>
        <button class="btn btn-s" onclick="sim(1,2)">&#x25B6;</button>
      </div>
      <div style="display:flex;gap:6px;align-items:center;padding-top:2px">
        <span id="tmr-st" style="font-size:10px;color:var(--tx2)">&#x17D;&#xE1;dn&#xFD; timer</span>
        <button class="btn btn-s btn-d" id="tmr-x" style="display:none" onclick="sim(1,1)">Zru&#x0161;it</button>
      </div>
      <div class="row" style="border-top:none;padding-top:0;flex-wrap:wrap;gap:5px">
        <span style="font-size:9px;color:var(--tx3);font-family:monospace">Okno 2&times;:</span>
        <input class="inp inp-s" type="number" id="d1" min="100" max="1500" value="400">
        <span style="font-size:9px;color:var(--tx3)">ms</span>
        <button class="btn btn-s" onclick="saveDly(1)">OK</button>
      </div>
    </div>
    <div class="bc">
      <div class="bh">
        <div class="bnum">3</div>
        <div><div class="bname">Bambu &ndash; Sv&#x11B;tlo/Heat</div><div class="bsub">MQTT LAN &middot; GPIO 4</div></div>
        <div class="bstat"><span class="pill p-off" id="p-bl">OFF</span></div>
      </div>
      <div class="row"><span class="rl">1&times;</span><span class="rd">Sv&#x11B;tlo tisk&#xE1;rny</span><button class="btn btn-s" onclick="sim(2,1)">&#x25B6;</button></div>
      <div class="row"><span class="rl">2&times;</span><span class="rd">P&#x159;edeh&#x159;ev 230&deg;C/67&deg;C</span><button class="btn btn-s" onclick="sim(2,2)">&#x25B6;</button></div>
      <div class="row" style="border-top:none;padding-top:0;flex-wrap:wrap;gap:5px">
        <span style="font-size:9px;color:var(--tx3);font-family:monospace">Okno 2&times;:</span>
        <input class="inp inp-s" type="number" id="d2" min="100" max="1500" value="400">
        <span style="font-size:9px;color:var(--tx3)">ms</span>
        <button class="btn btn-s" onclick="saveDly(2)">OK</button>
      </div>
    </div>
    <div class="bc">
      <div class="bh">
        <div class="bnum">4</div>
        <div><div class="bname">Bambu &ndash; Home/Speed</div><div class="bsub">MQTT LAN &middot; GPIO 1</div></div>
        <div class="bstat"><span class="pill p-off" id="p-sp">Std</span></div>
      </div>
      <div class="row"><span class="rl">1&times;</span><span class="rd">Homing (G28)</span><button class="btn btn-s" onclick="sim(3,1)">&#x25B6;</button></div>
      <div class="row"><span class="rl">2&times;</span><span class="rd">Rychlost: <b id="spn">&#x2013;</b></span><button class="btn btn-s" onclick="sim(3,2)">&#x25B6;</button></div>
      <div class="row" style="border-top:none;padding-top:0;flex-wrap:wrap;gap:5px">
        <span style="font-size:9px;color:var(--tx3);font-family:monospace">Okno 2&times;:</span>
        <input class="inp inp-s" type="number" id="d3" min="100" max="1500" value="400">
        <span style="font-size:9px;color:var(--tx3)">ms</span>
        <button class="btn btn-s" onclick="saveDly(3)">OK</button>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="ch"><span class="ct">Displej</span></div>
    <div class="cb">
      <div class="g2">
        <div>
          <div class="fld"><div class="flbl">Jas</div><div class="rrow"><input class="rng" type="range" id="dbr" min="0" max="255" value="200" oninput="document.getElementById('brv').textContent=this.value"><span class="rv" id="brv">200</span></div></div>
          <div class="fld"><div class="flbl">Form&#xE1;t</div><select class="sel" id="dfmt"><option value="0">13:28</option><option value="1">13:28:45</option></select></div>
        </div>
        <div>
          <div class="fld" style="flex-direction:row;align-items:center;gap:7px"><div class="flbl" style="margin:0">Zapnut&#xFD;</div><label class="tog"><input type="checkbox" id="don" checked><span class="tsl"></span></label></div>
          <button class="btn btn-p" style="margin-top:10px" onclick="saveDisp()">Ulo&#x017E;it displej</button>
        </div>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="ch"><span class="ct">Historie</span><span style="font-family:monospace;font-size:9px;color:var(--tx3);margin-left:5px" id="hcnt">0</span><div style="margin-left:auto"><button class="btn btn-s btn-d" onclick="doClr('history')">Smazat</button></div></div>
    <div class="hl" id="hlist"><div class="empty">Zatím pr&#xE1;zdn&#xE9;</div></div>
  </div>
</div>

<!-- ═══ TAB SCRIPT ═══ -->
<div class="pg" id="pg-s">
  <div class="card">
    <div class="ch"><span class="ct">OTA Firmware Upload</span></div>
    <div class="cb">
      <div class="note">Arduino IDE: <b>Sketch &rarr; Export Compiled Binary (.bin)</b> &rarr; p&#x159;etáhni sem nebo klikni.</div>
      <div class="upz" id="upz" ondrop="onDrop(event)" ondragover="onDov(event)" ondragleave="onDlv(event)" onclick="document.getElementById('ufi').click()">
        <div class="upz-i">&#x1F4E6;</div>
        <div class="upz-t"><b>Klikni nebo p&#x159;etáhni .bin soubor</b><br><span id="ufn">Nevybr&#xE1;n &#x17E;&#xE1;dn&#xFD; soubor</span></div>
      </div>
      <input type="file" id="ufi" accept=".bin" style="display:none" onchange="onFS(event)">
      <div style="display:flex;gap:8px;align-items:center;margin-top:10px">
        <button class="btn btn-p" id="otabtn" onclick="doOTA()" disabled>&#x1F680; Nahr&#xE1;t</button>
        <span id="otast" style="font-size:11px;color:var(--tx2)"></span>
      </div>
      <div class="prg" id="prg" style="display:none"><div class="prgf" id="prgf"></div></div>
    </div>
  </div>
  <div class="card">
    <div class="ch"><span class="ct">Konzole</span></div>
    <div id="con">OTA konzole...</div>
  </div>
</div>

<!-- ═══ TAB INFO ═══ -->
<div class="pg" id="pg-i">
  <div class="g2">
    <div class="card">
      <div class="ch"><span class="ct">S&#xED;&#x165;</span></div>
      <div class="cb" style="padding:7px 13px">
        <div class="ir"><span class="ik">SSID</span><span class="iv" id="i-ssid">&#x2013;</span></div>
        <div class="ir"><span class="ik">IP</span><span class="iv" id="i-ip">&#x2013;</span></div>
        <div class="ir"><span class="ik">Sign&#xE1;l</span><span class="iv"><div class="sig" id="i-sig"><div class="sb"></div><div class="sb"></div><div class="sb"></div><div class="sb"></div></div></span></div>
        <div class="ir"><span class="ik">RSSI</span><span class="iv" id="i-rssi">&#x2013;</span></div>
      </div>
    </div>
    <div class="card">
      <div class="ch"><span class="ct">Syst&#xE9;m</span></div>
      <div class="cb" style="padding:7px 13px">
        <div class="ir"><span class="ik">Uptime</span><span class="iv" id="i-up">&#x2013;</span></div>
        <div class="ir"><span class="ik">NTP</span><span class="iv" id="i-ntp">&#x2013;</span></div>
        <div class="ir"><span class="ik">BambuLab</span><span class="iv" id="i-bst">&#x2013;</span></div>
        <div class="ir" style="border-bottom:none;padding-top:8px"><button class="btn btn-s btn-d" onclick="doRestart()">&#x1F504; Restart</button></div>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="ch"><span class="ct">BambuLab &ndash; &#x17E;iv&#xE1; data</span><span class="pill p-off" id="bst-pill" style="margin-left:6px">Offline</span></div>
    <div class="cb">
      <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px">
        <div class="bst"><div class="bstn" id="b-n">&#x2013;</div><div class="bstl">Tryska &deg;C</div></div>
        <div class="bst"><div class="bstn" id="b-b">&#x2013;</div><div class="bstl">Podlo&#x017E;ka &deg;C</div></div>
        <div class="bst"><div class="bstn" id="b-p">&#x2013;</div><div class="bstl">Tisk %</div></div>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="ch"><span class="ct">Nastaven&#xED;</span></div>
    <div class="cb">
      <div class="g2">
        <div>
          <div class="fld"><div class="flbl">WiFi SSID</div><input class="inp" id="c-ssid" type="text"></div>
          <div class="fld"><div class="flbl">WiFi heslo (pr&#xE1;zdn&#xE9; = nezm&#x11B;nit)</div><input class="inp" id="c-wp" type="password" placeholder="&#x25CF;&#x25CF;&#x25CF;&#x25CF;&#x25CF;&#x25CF;"></div>
          <div class="fld"><div class="flbl">Timezone offset (s) &mdash; CET=3600</div><input class="inp" id="c-tz" type="number"></div>
          <div class="fld"><div class="flbl">Timer z&#xE1;suvky (min)</div><input class="inp" id="c-pt" type="number" min="1" max="180"></div>
        </div>
        <div>
          <div class="fld"><div class="flbl">SinricPro App Key</div><input class="inp" id="c-sk" type="text"></div>
          <div class="fld"><div class="flbl">SinricPro Secret (pr&#xE1;zdn&#xE9; = nezm&#x11B;nit)</div><input class="inp" id="c-ss" type="password" placeholder="&#x25CF;&#x25CF;&#x25CF;&#x25CF;&#x25CF;&#x25CF;"></div>
          <div class="fld"><div class="flbl">Light Device ID</div><input class="inp" id="c-sl" type="text"></div>
          <div class="fld"><div class="flbl">Plug Device ID</div><input class="inp" id="c-sp" type="text"></div>
        </div>
        <div>
          <div class="fld"><div class="flbl">BambuLab IP</div><input class="inp" id="c-bi" type="text" placeholder="192.168.x.x"></div>
          <div class="fld"><div class="flbl">BambuLab Serial</div><input class="inp" id="c-bs" type="text"></div>
          <div class="fld"><div class="flbl">BambuLab Code (pr&#xE1;zdn&#xE9; = nezm&#x11B;nit)</div><input class="inp" id="c-bc" type="password" placeholder="&#x25CF;&#x25CF;&#x25CF;&#x25CF;&#x25CF;&#x25CF;"></div>
        </div>
      </div>
      <div style="display:flex;gap:8px;align-items:center;margin-top:4px">
        <button class="btn btn-p" onclick="saveCfg()">&#x1F4BE; Ulo&#x017E;it v&#x0161;e</button>
        <span id="c-st" style="font-size:10px;color:var(--tx2)"></span>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="ch"><span class="ct">Chyby</span><span style="font-family:monospace;font-size:9px;color:var(--tx3);margin-left:5px" id="ecnt">0</span><div style="margin-left:auto"><button class="btn btn-s btn-d" onclick="doClr('errors')">Smazat</button></div></div>
    <div class="hl" id="elist"><div class="empty">&#x17D;&#xE1;dn&#xE9; chyby &#x1F389;</div></div>
  </div>
</div>

<div id="toast"></div>
<script>
// ── Stav ──────────────────────────────────────────────
var selFile = null;
window.onload = function() { fetchAll(); setInterval(fetchAll, 2000); };

// ── Tabs ──────────────────────────────────────────────
function gTab(id, el) {
  document.querySelectorAll('.tab').forEach(function(t){t.classList.remove('a');});
  document.querySelectorAll('.pg').forEach(function(p){p.classList.remove('a');});
  el.classList.add('a');
  document.getElementById('pg-'+id).classList.add('a');
  if (id === 'i') { fetchCfg(); fetchErrs(); }
}

// ── API volání
// KLÍČ: Content-Type je "text/plain" aby server.arg("plain") fungoval!
function apip(url, body) {
  return fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: JSON.stringify(body)
  }).then(function(r) {
    if (!r.ok) throw new Error(r.status);
    return r.json();
  }).catch(function(e) { console.error(url, e); return null; });
}
function apig(url) {
  return fetch(url).then(function(r){
    if(!r.ok) throw new Error(r.status);
    return r.json();
  }).catch(function(e){console.error(url,e);return null;});
}

// ── Polling ───────────────────────────────────────────
function fetchAll() {
  apig('/api/status').then(function(s){ if(s) updateUI(s); });
  apig('/api/history').then(function(h){ if(h) renderLog(h, 'hlist', 'hcnt', false); });
}
function fetchCfg() {
  apig('/api/config').then(function(c){ if(c) fillCfg(c); });
}
function fetchErrs() {
  apig('/api/errors').then(function(e){ if(e) renderLog(e, 'elist', 'ecnt', true); });
}

// ── UI update ─────────────────────────────────────────
function updateUI(s) {
  // Header
  document.getElementById('hdr-ip').textContent = s.ip || '';
  document.getElementById('led-w').classList.toggle('on', true);
  document.getElementById('led-b').classList.toggle('on', !!s.bOnline);
  // Tlačítka
  pill('p-l', s.light);
  pill('p-p', s.plug);
  pill('p-bl', s.bLight);
  set('lcol', s.lightCol || '–');
  set('spn', s.bSpeedN || '–');
  var sp = document.getElementById('p-sp');
  if (sp) sp.textContent = s.bSpeedN || '–';
  // Timer
  var ts = document.getElementById('tmr-st'), tx = document.getElementById('tmr-x');
  if (s.plugTimer) {
    ts.textContent = 'Timer: ~' + Math.ceil(s.plugLeft/60) + ' min';
    tx.style.display = '';
  } else {
    ts.textContent = 'Žádný timer';
    tx.style.display = 'none';
  }
  // Info tab
  set('i-ssid', s.ssid || '–');
  set('i-ip', s.ip || '–');
  set('i-rssi', (s.rssi || '–') + ' dBm');
  set('i-up', s.uptime || '–');
  set('i-ntp', s.ntpOk ? 'Synchronizováno' : 'Čekám...');
  set('i-bst', s.bOnline ? (s.bHoming ? '🔄 Homing' : '🟢 Online') : '🔴 Offline');
  var bp = document.getElementById('bst-pill');
  if (bp) { bp.textContent = s.bOnline ? (s.bHoming?'Homing':'Online') : 'Offline'; bp.className = 'pill ' + (s.bOnline ? (s.bHoming?'p-ac':'p-on') : 'p-off'); }
  // Bambu live
  set('b-n', s.bNozzle != null ? s.bNozzle.toFixed(1) : '–');
  set('b-b', s.bBed != null ? s.bBed.toFixed(1) : '–');
  set('b-p', s.bPct != null ? s.bPct + '%' : '–');
  // RSSI signal bars
  var r = s.rssi || -100, bars = r > -55 ? 4 : r > -65 ? 3 : r > -75 ? 2 : 1;
  document.querySelectorAll('.sb').forEach(function(b,i){b.classList.toggle('on',i<bars);});
}

function set(id, v) { var e = document.getElementById(id); if(e) e.textContent = v; }
function pill(id, on) {
  var e = document.getElementById(id); if(!e) return;
  e.textContent = on ? 'ON' : 'OFF';
  e.className = 'pill ' + (on ? 'p-on' : 'p-off');
}

function renderLog(items, lid, cid, isErr) {
  var el = document.getElementById(lid), ce = document.getElementById(cid);
  if (ce) ce.textContent = items.length;
  if (!items.length) { el.innerHTML = '<div class="empty">' + (isErr ? 'Žádné chyby 🎉' : 'Prázdné') + '</div>'; return; }
  var h = '';
  for (var i = items.length - 1; i >= 0; i--) {
    var it = items[i];
    h += '<div class="hrow"><span class="hts">' + fmtTs(it.ts) + '</span><span class="htx"' + (isErr?' style="color:var(--er)"':'') + '>' + esc(it.t) + '</span></div>';
  }
  el.innerHTML = h;
}
function fmtTs(ts) { if(!ts)return'–'; var d=new Date(ts*1000); return d.toLocaleTimeString('cs-CZ',{hour:'2-digit',minute:'2-digit',second:'2-digit'}); }
function esc(s) { return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }

function fillCfg(c) {
  var m = {
    'c-ssid':c.ssid,'c-tz':c.tz,'c-pt':c.ptmr,
    'c-sk':c.skey,'c-sl':c.slid,'c-sp':c.spid,
    'c-bi':c.bip,'c-bs':c.bser
  };
  for(var k in m){ var e=document.getElementById(k); if(e&&m[k]!=null)e.value=m[k]; }
  var dbr=document.getElementById('dbr'); if(dbr){dbr.value=c.br||200;document.getElementById('brv').textContent=c.br||200;}
  var don=document.getElementById('don'); if(don)don.checked=c.don!==false;
  var dfmt=document.getElementById('dfmt'); if(dfmt)dfmt.value=c.tfmt||0;
  if(c.dbl){for(var i=0;i<4;i++){var di=document.getElementById('d'+i);if(di)di.value=c.dbl[i];}}
  if(c.ptmr){var tm=document.getElementById('tmr-m');if(tm)tm.value=c.ptmr;}
}

// ── Akce ─────────────────────────────────────────────
function sim(b, c) {
  apip('/api/press', {btn:b,clicks:c}).then(function(r){
    if(r&&r.ok){toast('Odesláno','ok');setTimeout(fetchAll,400);}
    else toast('Chyba','er');
  });
}

function saveDly(i) {
  var v = parseInt(document.getElementById('d'+i).value)||400;
  apig('/api/config').then(function(c){
    if(!c)return;
    var d=c.dbl||[400,400,400,400]; d[i]=v;
    apip('/api/config',{dbl:d}).then(function(r){if(r&&r.ok)toast('Uloženo','ok');else toast('Chyba','er');});
  });
}

function saveDisp() {
  apip('/api/config',{
    br:parseInt(document.getElementById('dbr').value),
    don:document.getElementById('don').checked,
    tfmt:parseInt(document.getElementById('dfmt').value)
  }).then(function(r){if(r&&r.ok)toast('Displej uložen','ok');else toast('Chyba','er');});
}

function saveCfg() {
  var body = {
    ssid: document.getElementById('c-ssid').value,
    skey: document.getElementById('c-sk').value,
    slid: document.getElementById('c-sl').value,
    spid: document.getElementById('c-sp').value,
    bip:  document.getElementById('c-bi').value,
    bser: document.getElementById('c-bs').value,
    tz:   parseInt(document.getElementById('c-tz').value)||3600,
    ptmr: parseInt(document.getElementById('c-pt').value)||5
  };
  var wp=document.getElementById('c-wp').value; if(wp)body.wp=wp;
  var ss=document.getElementById('c-ss').value; if(ss)body.ss=ss;
  var bc=document.getElementById('c-bc').value; if(bc)body.bc=bc;
  var st=document.getElementById('c-st');
  apip('/api/config',body).then(function(r){
    if(r&&r.ok){toast('Uloženo ✓','ok');st.textContent='✓ Uloženo';st.style.color='var(--ok)';}
    else{toast('Chyba!','er');st.textContent='✗ Chyba';st.style.color='var(--er)';}
  });
}

function doClr(what) {
  apip('/api/clear',{what:what}).then(function(){toast('Smazáno','ok');fetchAll();fetchErrs();});
}
function doRestart() {
  if(!confirm('Restartovat?'))return;
  apip('/api/restart',{}).then(function(){toast('Restartuji...','ok');});
}

// ── OTA ───────────────────────────────────────────────
function onDrop(e){e.preventDefault();document.getElementById('upz').classList.remove('dv');setF(e.dataTransfer.files[0]);}
function onDov(e){e.preventDefault();document.getElementById('upz').classList.add('dv');}
function onDlv(e){document.getElementById('upz').classList.remove('dv');}
function onFS(e){setF(e.target.files[0]);}
function setF(f){
  if(!f)return; selFile=f;
  document.getElementById('ufn').textContent=f.name+' ('+(f.size/1024).toFixed(1)+' kB)';
  document.getElementById('otabtn').disabled=false;
  clog('Soubor: '+f.name);
}
function doOTA(){
  if(!selFile||!confirm('Nahrát '+selFile.name+'?'))return;
  var pf=document.getElementById('prgf'),st=document.getElementById('otast'),prg=document.getElementById('prg');
  prg.style.display=''; pf.style.width='0%';
  document.getElementById('otabtn').disabled=true;
  clog('Spouštím OTA upload...');
  var fd=new FormData(); fd.append('firmware',selFile,selFile.name);
  var xhr=new XMLHttpRequest(); xhr.open('POST','/api/ota');
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){var p=Math.round(e.loaded/e.total*100);pf.style.width=p+'%';st.textContent=p+'%';clog(p+'%');}
  };
  xhr.onload=function(){
    document.getElementById('otabtn').disabled=false;
    if(xhr.status===200){clog('✓ Hotovo! ESP32 se restartuje...');st.textContent='✓ Hotovo!';toast('OTA OK','ok');}
    else{clog('✗ Chyba '+xhr.status);st.textContent='Chyba';toast('OTA chyba','er');}
  };
  xhr.onerror=function(){clog('✗ Síťová chyba');toast('Chyba sítě','er');document.getElementById('otabtn').disabled=false;};
  xhr.send(fd);
}
function clog(m){var e=document.getElementById('con');e.innerHTML+='\n['+new Date().toLocaleTimeString('cs-CZ')+'] '+esc(m);e.scrollTop=e.scrollHeight;}

// ── Toast ─────────────────────────────────────────────
var _tt=null;
function toast(msg,type){
  var e=document.getElementById('toast');
  e.textContent=msg; e.className='sh '+(type||'');
  clearTimeout(_tt); _tt=setTimeout(function(){e.className='';},2400);
}
</script>
</body>
</html>)SRHTML";

//serverikos
void webSetup() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", HTML_PAGE);
  });

  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", buildStatus());
  });

  server.on("/api/history", HTTP_GET, []() {
    server.send(200, "application/json",
      buildLogStr(histBuf, histHead, histCnt, HIST_MAX));
  });

  server.on("/api/errors", HTTP_GET, []() {
    server.send(200, "application/json",
      buildLogStr(errBuf, errHead, errCnt, ERR_MAX));
  });

  server.on("/api/config", HTTP_GET, []() {
    server.send(200, "application/json", buildConfig());
  });

//pošta
  server.on("/api/config", HTTP_POST, []() {
    String body = server.arg("plain");
    Serial.printf("[WEB] POST /api/config body=%d chars\n", body.length());
    if (body.isEmpty()) { server.send(400, "application/json", "{\"ok\":false,\"e\":\"empty\"}"); return; }

    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
      server.send(400, "application/json", "{\"ok\":false,\"e\":\"json\"}"); return;
    }
    bool ch = false;

    // Kopirovani stringu 
    auto sc = [&](const char* key, char* field, size_t sz) {
      if (doc.containsKey(key)) {
        const char* v = doc[key].as<const char*>();
        if (v && strlen(v) > 0) { strlcpy(field, v, sz); ch = true; }
      }
    };
    sc("ssid", cfg.wifi_ssid,       64);
    sc("wp",   cfg.wifi_pass,        64);
    sc("skey", cfg.sinric_key,       64);
    sc("ss",   cfg.sinric_secret,   128);
    sc("slid", cfg.sinric_light_id,  64);
    sc("spid", cfg.sinric_plug_id,   64);
    sc("bip",  cfg.bambu_ip,         32);
    sc("bser", cfg.bambu_serial,      32);
    sc("bc",   cfg.bambu_code,        32);

    if (doc.containsKey("br"))   { cfg.disp_brightness = doc["br"].as<uint8_t>();  ch = true; }
    if (doc.containsKey("don"))  { cfg.disp_on          = doc["don"].as<bool>();    ch = true; }
    if (doc.containsKey("tfmt")) { cfg.time_fmt          = doc["tfmt"].as<uint8_t>(); ch = true; }
    if (doc.containsKey("ptmr")) { cfg.plug_timer_min   = doc["ptmr"].as<uint16_t>(); ch = true; }
    if (doc.containsKey("tz"))   { cfg.tz_offset         = doc["tz"].as<int32_t>(); ch = true; }

    if (doc.containsKey("dbl") && doc["dbl"].is<JsonArray>()) {
      JsonArray a = doc["dbl"].as<JsonArray>();
      for (int i = 0; i < 4 && i < (int)a.size(); i++) { cfg.dbl_ms[i] = a[i]; ch = true; }
    }
    if (ch) saveCfg();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/press", HTTP_POST, []() {
    String body = server.arg("plain");
    Serial.printf("[WEB] POST /api/press: %s\n", body.c_str());
    StaticJsonDocument<64> doc;
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      int b = doc["btn"] | -1;
      int c = doc["clicks"] | 1;
      if (b >= 0 && b < 4) processClick(b, c);
    }
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/clear", HTTP_POST, []() {
    String body = server.arg("plain");
    StaticJsonDocument<64> doc;
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      String w = doc["what"].as<String>();
      if (w == "history") { histHead = 0; histCnt = 0; }
      else if (w == "errors") { errHead = 0; errCnt = 0; }
    }
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/restart", HTTP_POST, []() {
    server.send(200, "application/json", "{\"ok\":true}");
    delay(300); ESP.restart();
  });

  // OTA
  server.on("/api/ota", HTTP_POST,
    []() {
      server.sendHeader("Connection", "close");
      bool ok = !Update.hasError();
      server.send(200, "text/plain", ok ? "OK" : "FAIL");
      if (ok) { delay(500); ESP.restart(); }
    },
    []() {
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START) {
        Serial.printf("[OTA] Start: %s\n", up.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) { Update.printError(Serial); }
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (!Update.hasError()) {
          if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
        }
      } else if (up.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("[OTA] Hotovo: %u B\n", up.totalSize);
          addHist("OTA dokoncen");
        } else Update.printError(Serial);
      }
    }
  );

  server.onNotFound([]() { server.send(404, "text/plain", "404"); });
  server.begin();
  Serial.printf("[WEB] http://%s/\n", WiFi.localIP().toString().c_str());
}

//seťup
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n╔══════════════════════════════╗");
  Serial.println(  "║   Smart Remote  v19          ║");
  Serial.println(  "╚══════════════════════════════╝");

  // ── 1. Config ────────────────────────────────────────
  loadCfg();
  strlcpy(cfg.wifi_ssid, WIFI_SSID, sizeof(cfg.wifi_ssid));
  strlcpy(cfg.wifi_pass, WIFI_PASS, sizeof(cfg.wifi_pass));
  cfg.tz_offset = TZ_OFFSET;
  saveCfg();

  // tvrdý drive
  for (int i = 0; i < 4; i++) pinMode(BTN_PINS[i], INPUT);
  Wire.begin(I2C_SDA, I2C_SCL);
  u8g2.begin();
  u8g2.setContrast(cfg.disp_brightness);

  // Boot splash
  u8g2.clearBuffer();
  u8g2.drawBox(0, 0, 128, 32);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_helvB08_tr);
  u8g2.drawStr((128 - u8g2.getStrWidth("SMART REMOTE"))/2, 11, "SMART REMOTE");
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr((128 - u8g2.getStrWidth("by Matej  v19"))/2, 24, "by Matej  v19");
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
  delay(1200);

  // wifi
  oledBoot("Smart Remote", "Pripojuji WiFi...");
  wifiInit(); // nast.

  unsigned long wifiStart = millis();
  bool servicesStarted = false;
  while (millis() - wifiStart < 15000) {
    oledBoot("WiFi...", wifiConnected ? "OK!" : cfg.wifi_ssid);
    delay(250);
    if (wifiConnected) {
      delay(200); // krátká pauza 
      break;
    }
  }

  // vysledek
  if (wifiConnected) {
    char ipLine[20];
    WiFi.localIP().toString().toCharArray(ipLine, sizeof(ipLine));
    Serial.printf("[Boot] IP: %s\n", ipLine);
    u8g2.clearBuffer();
    u8g2.drawBox(0, 0, 128, 13);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr((128 - u8g2.getStrWidth("WiFi OK!"))/2, 10, "WiFi OK!");
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr((128 - u8g2.getStrWidth(ipLine))/2, 23, ipLine);
    u8g2.sendBuffer();
    delay(1500);

//ntp
    if (!ntpSynced) {
      ntpCli.begin();
      ntpCli.setTimeOffset(cfg.tz_offset);
      ntpCli.update();
      if (ntpCli.getEpochTime() > 1000000) ntpSynced = true;
    }

    // Ostatní služby
    webSetup(); webStarted = true;
    sinricSetup();
    bambuTaskStart();
    if (strlen(cfg.bambu_ip) > 4) bambuRetry = millis();
    servicesStarted = true;

  } else {
    // AP 
    Serial.println("[WiFi] Selhalo – AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("SmartRemote_AP", "12345678");
    IPAddress apIP = WiFi.softAPIP();
    char apStr[20]; apIP.toString().toCharArray(apStr, sizeof(apStr));
    Serial.printf("[AP] %s\n", apStr);

    u8g2.clearBuffer();
    u8g2.drawBox(0, 0, 128, 13);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr((128 - u8g2.getStrWidth("WIFI SELHALA!"))/2, 10, "WIFI SELHALA!");
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(0, 21, "AP: SmartRemote_AP");
    u8g2.drawStr((128 - u8g2.getStrWidth(apStr))/2, 31, apStr);
    u8g2.sendBuffer();

    webSetup(); webStarted = true;
  }

  bootMs = millis();
  dm     = DM_TIME;

  //5. Watchdog
  esp_task_wdt_init(30, true); 
  esp_task_wdt_add(NULL);
  Serial.println("[WDT] Watchdog 30s aktivovan");
  Serial.println("[Boot] Hotovo!\n");
}

void loop() {
  unsigned long now = millis();

  // Watchdog – VŽDY první
  esp_task_wdt_reset();

  // reccpnect
  if (!systemSleeping && !wifiConnected && now >= wifiReconnectAt && wifiReconnectAt > 0) {
    Serial.println("[WiFi] Reconnect pokus...");
    wifiReconnectAt = 0;
    WiFi.disconnect(false);
    delay(50);
    WiFi.begin(cfg.wifi_ssid, cfg.wifi_pass);
  }

  // web
  if (!systemSleeping && webStarted) {
    server.handleClient();
    server.handleClient();
  }

  // ④ SinricPro (jen pokud WiFi OK)
  if (!systemSleeping && wifiConnected) SinricPro.handle();

  // bambu
  if (!systemSleeping && wifiConnected) {
    unsigned long now2 = millis();
    bool stale = bambuConn && bambuLastMsg > 0 && (now2 - bambuLastMsg) > 180000UL; // 3 min bez zprávy
    bool disconnected = !bambuMqtt.connected() && !bambuTaskConnect;

    if ((disconnected || stale) && (now2 - bambuRetry) > 20000 && strlen(cfg.bambu_ip) > 4) {
      if (stale) {
        Serial.println("[Bambu] Stale connection – force reconnect");
        bambuMqtt.disconnect();
      }
      bambuRetry       = now2;
      bambuConn        = false;
      bambuOnline      = false;
      bambuTaskConnect = true;
    }
  }

//ntp
  static unsigned long ntpLast = 0;
  if (!systemSleeping && wifiConnected) {
    unsigned long ntpInterval = ntpSynced ? 300000UL : 10000UL;
    if (now - ntpLast > ntpInterval) {
      ntpLast = now;
      if (ntpCli.update()) {
        if (!ntpSynced) Serial.println("[NTP] Cas synced!");
        ntpSynced = true;
      }
    }
  }

  // homing
  if (bambuHoming && now > bambuHomingEnd) bambuHoming = false;

  //plug
  if (!systemSleeping && plugTimer && now > plugTimerEnd) {
    plugTimer = false; plugOn = false;
    SinricProSwitch& P = SinricPro[cfg.sinric_plug_id];
    P.sendPowerStateEvent(false);
    addHist("Zasuvka timer: VYP");
    showMsg(DM_DONE, "Timer", "Zasuvka VYP");
  }

  // light
  if (!systemSleeping && lightTimer && now > lightTimerEnd) {
    lightTimer = false; lightOn = false;
    SinricProLight& L = SinricPro[cfg.sinric_light_id];
    L.sendPowerStateEvent(false);
    addHist("Svetlo timer: VYP");
    showMsg(DM_DONE, "Svetlo timer", "VYPNUTO");
  }

  // sleeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeep
  if (!systemSleeping && btn[0].lastStable && !btn[0].holdFired) {
    unsigned long held = now - btn[0].pressStart;
    if (held >= 1000 && held < LONG_PRESS_MS) {
      int secs = (int)((LONG_PRESS_MS - held) / 1000) + 1;
      char cd[20]; snprintf(cd, sizeof(cd), "Usnu za %ds...", secs);
      showMsg(DM_WAIT, "Drz BTN1", cd, 300);
    }
  }

  // buttons
  handleBtns();

  // display
  static unsigned long dispLast = 0;
  if (!systemSleeping && now - dispLast > 50) {
    dispLast = now;
    renderDisplay();
  }
}
