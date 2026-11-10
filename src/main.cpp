// ÖV-Abfahrtstafel – ESP32 + ST7789 (320x240), Daten von transport.opendata.ch

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <qrcode.h>
#include <time.h>
#include "SourceSansBold26.h"
#include "SourceSansBold19.h"
#include "web.h"

static const char* HOSTNAME = "abfahrtstafel";
static const int BOOT_PIN = 0;
static const uint32_t FETCH_INTERVAL_MS = 30000;
static const uint32_t WIFI_TIMEOUT_MS = 30000;
static const uint32_t PORTAL_RETRY_MS = 5 * 60000;
static const char* TZ_CH = "CET-1CEST,M3.5.0,M10.5.0/3";
static const char* API = "https://transport.opendata.ch/v1/";
static const uint32_t UPDATE_INTERVAL_MS = 6UL * 3600 * 1000;
#ifndef FW_BUILD
#define FW_BUILD 0
#endif
#ifndef GITHUB_REPO
#define GITHUB_REPO ""
#endif

static const char* DEFAULT_CFG = R"({"title":"Abfahrten","stations":[
{"name":"Zürich, Helvetiaplatz","lines":"8"},{"name":"Zürich, Militär-/Langstrasse","lines":"31,32"}]})";

static const int SCREEN_W = 320, HEADER_H = 40, ROW_H = 33, MAX_ROWS = 6, BASE = 24;
static const int MAX_DEPS = 8, MAX_ST = 4;
static const int BADGE_H = 22, DEST_X = 62, MIN_RIGHT = 314, CLOCK_X = 282, HDR_BASE = 28;

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return uint16_t(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

struct ColorDef { const char* key; const char* name; const char* def; };
const ColorDef COLORS[] = {
  {"bg", "Hintergrund", "#1a2ab0"}, {"hdr", "Kopfzeile", "#101878"},
  {"line", "Trennlinien", "#4055d8"}, {"text", "Text", "#ffffff"},
  {"delay", "Verspätung", "#ffde15"}, {"muted", "Hinweise", "#b4b8e6"},
  {"go", "Jetzt los", "#ff4a3d"},
};
static const int NUM_COLORS = sizeof(COLORS) / sizeof(COLORS[0]);
String colHex[NUM_COLORS];
uint16_t col[NUM_COLORS];
#define C_BG     col[0]
#define C_HDR    col[1]
#define C_LINE   col[2]
#define C_WHITE  col[3]
#define C_YELLOW col[4]
#define C_LABEL  col[5]
#define C_GO     col[6]
const uint16_t C_RED = rgb(0xEB, 0x00, 0x00);

struct LineColor { const char* line; uint16_t color; };
const LineColor TRAM_COLORS[] = {
  {"2", rgb(0xE2, 0x00, 0x1A)}, {"3", rgb(0x00, 0x9F, 0x4D)}, {"4", rgb(0x3E, 0x42, 0x91)},
  {"5", rgb(0x8D, 0x5B, 0x2D)}, {"6", rgb(0xD8, 0xA1, 0x57)}, {"7", rgb(0x23, 0x1F, 0x20)},
  {"8", rgb(0xA6, 0xCE, 0x39)}, {"9", rgb(0x48, 0x48, 0x94)}, {"10", rgb(0xE5, 0x00, 0x7D)},
  {"11", rgb(0x00, 0x9F, 0x4D)}, {"13", rgb(0xFF, 0xD5, 0x00)}, {"14", rgb(0x00, 0xA7, 0xE5)},
  {"15", rgb(0xE2, 0x00, 0x1A)}, {"17", rgb(0x91, 0x27, 0x8F)},
};

struct Station {
  bool train = false;
  String query, to, lines;
  int walk = 0;
};
String cfgTitle;
Station cfgSt[MAX_ST];
volatile int cfgNumSt = 0;
volatile uint32_t cfgGen = 0;
bool nightOn = false;
int nightFrom = 0, nightTo = 6 * 60;
volatile bool nightNow = false;
bool pixelFont = false;
SemaphoreHandle_t cfgMutex, depMutex;
Preferences prefs;

TFT_eSPI tft;
TFT_eSprite row(&tft), hdr(&tft), badgeSpr(&tft);
WebServer server(80);
DNSServer dns;
TaskHandle_t fetchHandle = nullptr;
uint32_t infoUntil = 0, wakeUntil = 0;
volatile bool updating = false;

struct Departure {
  char cat[6], line[8], dest[48], platform[8];
  time_t when;
  int delay, walk;
};
Departure deps[MAX_ST][MAX_DEPS];
int depCount[MAX_ST];
time_t lastOk[MAX_ST];

// Bildspeicher-Abgleich: nur geänderte Zeilen werden gezeichnet und übertragen
uint32_t rowSig[MAX_ROWS], hdrSig;
bool bottomDirty = true;

void invalidate() {
  memset(rowSig, 0, sizeof rowSig);
  hdrSig = 0;
  bottomDirty = true;
}

struct Hash {
  uint32_t h = 2166136261u;
  Hash& add(const void* p, size_t n) {
    for (auto b = (const uint8_t*)p; n--; b++) { h ^= *b; h *= 16777619u; }
    return *this;
  }
  Hash& str(const char* s) { return add(s, strlen(s) + 1); }
  template <class T> Hash& val(T v) { return add(&v, sizeof v); }
  uint32_t get() const { return h ? h : 1; }
};

// ===================== Helfer =====================
int lum565(uint16_t c) {
  return (((c >> 11) << 3) * 299 + (((c >> 5) & 0x3F) << 2) * 587 + ((c & 0x1F) << 3) * 114) / 1000;
}

uint16_t tramColor(const char* line) {
  for (auto& lc : TRAM_COLORS) if (!strcmp(lc.line, line)) return lc.color;
  return rgb(0x80, 0x80, 0x80);
}

bool isCat(const char* cat, std::initializer_list<const char*> list) {
  for (const char* c : list) if (!strcmp(cat, c)) return true;
  return false;
}
bool isTram(const char* c) { return isCat(c, {"T", "NFT", "TRAM"}); }
bool isBus(const char* c) { return isCat(c, {"B", "NFB", "BUS", "KB"}); }
bool isBoat(const char* c) { return isCat(c, {"BAT", "FAE"}); }

bool validHex(const char* h) {
  if (!h || strlen(h) != 7 || h[0] != '#') return false;
  for (int i = 1; i < 7; i++) if (!isxdigit((uint8_t)h[i])) return false;
  return true;
}

uint16_t hexTo565(const String& h) {
  uint32_t v = strtoul(h.c_str() + 1, nullptr, 16);
  return rgb(v >> 16, (v >> 8) & 0xFF, v & 0xFF);
}

bool lineAllowed(const char* list, const char* line) {
  if (!*list) return true;
  size_t len = strlen(line);
  for (const char* p = list;;) {
    const char* end = strchr(p, ',');
    size_t tl = end ? (size_t)(end - p) : strlen(p);
    if (tl == len && !strncmp(p, line, len)) return true;
    if (!end) return false;
    p = end + 1;
  }
}

String urlEncode(const char* s) {
  String out;
  char buf[4];
  for (; *s; ++s) {
    uint8_t c = *s;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
    else { snprintf(buf, sizeof buf, "%%%02X", c); out += buf; }
  }
  return out;
}

// ===================== Text =====================
// size: 0 = Tafel gross, 1 = Tafel klein (beide mit Pixel-Option), 2 = SourceSans 19, 3 = Font 2
enum { BIG, SMALL, UI19, UI2 };

uint8_t toCp437(uint8_t c) {
  switch (c) {
    case 0xE4: return 0x84; case 0xF6: return 0x94; case 0xFC: return 0x81;
    case 0xC4: return 0x8E; case 0xD6: return 0x99; case 0xDC: return 0x9A;
    case 0xE9: return 0x82; case 0xE8: return 0x8A; case 0xEA: return 0x88;
    case 0xE0: return 0x85; case 0xE2: return 0x83; case 0xE7: return 0x87;
    case 0xF4: return 0x93; case 0xFB: return 0x96; case 0xEB: return 0x89;
    case 0xEF: return 0x8B; case 0xEE: return 0x8C; case 0xC9: return 0x90;
    case 0xB4: return '\'';
  }
  return c < 0x80 ? c : '?';
}

inline const uint8_t* glyph(uint8_t c, int& first, int& last) {
  const uint8_t* g = font + toCp437(c) * 5;
  first = 0; last = -1;
  for (int i = 0; i < 5; i++) if (pgm_read_byte(g + i)) { if (last < 0) first = i; last = i; }
  if (last < 0) { first = 0; last = 2; }
  return g;
}

inline int dotPitch(int size) { return size == BIG ? 3 : 2; }

int dotWidth(const char* s, int size) {
  int w = 0, f, l;
  for (; *s; s++) { glyph(*s, f, l); w += l - f + 2; }
  return w ? (w - 1) * dotPitch(size) : 0;
}

void dotDraw(TFT_eSPI& g, const char* s, int x, int baseline, int size, uint16_t color) {
  const int p = dotPitch(size), top = baseline - 7 * p;
  int f, l;
  for (; *s; s++) {
    const uint8_t* gl = glyph(*s, f, l);
    for (int cx = f; cx <= l; cx++) {
      uint8_t bits = pgm_read_byte(gl + cx);
      for (int ry = 0; bits; ry++, bits >>= 1)
        if (bits & 1) g.fillRect(x + (cx - f) * p, top + ry * p, 2, 2, color);
    }
    x += (l - f + 2) * p;
  }
}

inline bool usesDots(int size) { return pixelFont && size <= SMALL; }

void setFont(TFT_eSPI& g, int size) {
  if (size == UI2) g.setTextFont(2);
  else g.setFreeFont(size == BIG ? &SourceSansBold26 : &SourceSansBold19);
}

int txtWidth(TFT_eSPI& g, const char* s, int size) {
  if (usesDots(size)) return dotWidth(s, size);
  setFont(g, size);
  return g.textWidth(s);
}

void txtDraw(TFT_eSPI& g, const char* s, int x, int baseline, int size, uint16_t color) {
  if (usesDots(size)) { dotDraw(g, s, x, baseline, size, color); return; }
  setFont(g, size);
  g.setTextColor(color);
  g.setCursor(x, baseline);
  g.print(s);
}

// UTF-8 -> Latin-1, bei Bedarf mit "." gekürzt
void drawLabel(TFT_eSPI& g, const char* s, int x, int baseline, int maxW, uint16_t color, int size = BIG) {
  char txt[64];
  int n = 0;
  while (*s && n < 62) {
    uint8_t c = *s++;
    if (c < 0x80) txt[n++] = c;
    else if ((c & 0xE0) == 0xC0 && (*s & 0xC0) == 0x80) {
      uint16_t cp = ((c & 0x1F) << 6) | (*s++ & 0x3F);
      txt[n++] = cp <= 0xFF ? (char)cp : '?';
    } else {
      while ((*s & 0xC0) == 0x80) s++;
      txt[n++] = '?';
    }
  }
  txt[n] = 0;
  if (maxW > 0 && txtWidth(g, txt, size) > maxW) {
    while (n > 1) {
      n--;
      while (n > 1 && txt[n - 1] == ' ') n--;
      txt[n] = '.';
      txt[n + 1] = 0;
      if (txtWidth(g, txt, size) <= maxW) break;
    }
  }
  txtDraw(g, txt, x, baseline, size, color);
}

// ===================== Einstellungen =====================
void applyColors() {
  for (int i = 0; i < NUM_COLORS; i++) col[i] = hexTo565(colHex[i]);
}

void applyConfig(JsonVariantConst j) {
  String title = cfgTitle;
  if (j["title"].is<const char*>()) {
    title = j["title"].as<const char*>();
    title.trim();
    if (title.length() > 30) title = title.substring(0, 30);
  }

  Station st[MAX_ST];
  int n = -1;
  if (j["stations"].is<JsonArrayConst>()) {
    n = 0;
    for (JsonVariantConst s : j["stations"].as<JsonArrayConst>()) {
      if (n >= MAX_ST) break;
      String name = s["name"] | "";
      name.trim();
      if (name.isEmpty() || name.length() > 60) continue;
      bool train = !strcmp(s["type"] | "stop", "train");
      String to = s["to"] | "";
      to.trim();
      if (train && (to.isEmpty() || to.length() > 60)) continue;
      String lines = s["lines"] | "";
      lines.replace(" ", "");
      if (lines.length() > 60) lines = lines.substring(0, 60);
      st[n].train = train;
      st[n].query = name;
      if (train) st[n].to = to; else st[n].lines = lines;
      st[n].walk = constrain((int)(s["walk"] | 0), 0, 60);
      n++;
    }
  }

  bool nOn = nightOn;
  int nFrom = nightFrom, nTo = nightTo;
  if (j["night"].is<JsonObjectConst>()) {
    nOn = j["night"]["on"] | false;
    int h, m;
    if (sscanf(j["night"]["from"] | "", "%d:%d", &h, &m) == 2) nFrom = constrain(h * 60 + m, 0, 1439);
    if (sscanf(j["night"]["to"] | "", "%d:%d", &h, &m) == 2) nTo = constrain(h * 60 + m, 0, 1439);
  }

  for (int i = 0; i < NUM_COLORS; i++) {
    const char* h = j["colors"][COLORS[i].key] | (const char*)nullptr;
    if (validHex(h)) { colHex[i] = h; colHex[i].toLowerCase(); }
  }
  applyColors();

  xSemaphoreTake(cfgMutex, portMAX_DELAY);
  cfgTitle = title;
  if (n >= 0) {
    for (int i = 0; i < MAX_ST; i++) cfgSt[i] = i < n ? st[i] : Station();
    cfgNumSt = n;
  }
  nightOn = nOn; nightFrom = nFrom; nightTo = nTo;
  if (j["font"].is<const char*>()) pixelFont = !strcmp(j["font"], "pixel");
  cfgGen++;
  xSemaphoreGive(cfgMutex);
  invalidate();
}

void configToJson(JsonDocument& d, bool forWeb) {
  char buf[6];
  xSemaphoreTake(cfgMutex, portMAX_DELAY);
  d["title"] = cfgTitle;
  JsonArray a = d["stations"].to<JsonArray>();
  for (int i = 0; i < cfgNumSt; i++) {
    JsonObject o = a.add<JsonObject>();
    o["type"] = cfgSt[i].train ? "train" : "stop";
    o["name"] = cfgSt[i].query;
    if (cfgSt[i].train) o["to"] = cfgSt[i].to; else o["lines"] = cfgSt[i].lines;
    o["walk"] = cfgSt[i].walk;
  }
  d["font"] = pixelFont ? "pixel" : "sans";
  d["night"]["on"] = nightOn;
  snprintf(buf, sizeof buf, "%02d:%02d", nightFrom / 60, nightFrom % 60);
  d["night"]["from"] = buf;
  snprintf(buf, sizeof buf, "%02d:%02d", nightTo / 60, nightTo % 60);
  d["night"]["to"] = buf;
  xSemaphoreGive(cfgMutex);
  for (int i = 0; i < NUM_COLORS; i++) {
    d["colors"][COLORS[i].key] = colHex[i];
    if (forWeb) {
      d["names"][COLORS[i].key] = COLORS[i].name;
      d["defaults"][COLORS[i].key] = COLORS[i].def;
    }
  }
  if (forWeb) {
    d["ssid"] = WiFi.SSID();
    d["ip"] = WiFi.localIP().toString();
    d["version"] = FW_BUILD;
  }
}

void loadConfig() {
  for (int i = 0; i < NUM_COLORS; i++) colHex[i] = COLORS[i].def;
  cfgTitle = "Abfahrten";
  JsonDocument d;
  deserializeJson(d, DEFAULT_CFG);
  applyConfig(d);
  String saved = prefs.getString("cfg", "");
  if (saved.length()) {
    JsonDocument s;
    if (!deserializeJson(s, saved)) applyConfig(s);
  }
}

void saveConfig() {
  JsonDocument d;
  configToJson(d, false);
  String s;
  serializeJson(d, s);
  prefs.putString("cfg", s);
}

// ===================== Abfahrten holen =====================
// Eine TLS-Verbindung für alle Abfragen, wird wiederverwendet solange der Server sie offen hält
WiFiClientSecure tls;
HTTPClient http;

bool getJson(const String& url, JsonDocument& doc, JsonDocument& filter) {
  for (int attempt = 0; attempt < 2; attempt++) {
    if (!http.begin(tls, url)) continue;
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      String body = http.getString();
      http.end();
      return !deserializeJson(doc, body, DeserializationOption::Filter(filter));
    }
    http.end();
    tls.stop();
  }
  return false;
}

void storeDepartures(int s, Departure* tmp, int n, uint32_t gen) {
  for (int i = 1; i < n; i++) {
    Departure k = tmp[i];
    int j = i - 1;
    while (j >= 0 && tmp[j].when > k.when) { tmp[j + 1] = tmp[j]; j--; }
    tmp[j + 1] = k;
  }
  xSemaphoreTake(depMutex, portMAX_DELAY);
  if (gen == cfgGen) {
    depCount[s] = min(n, MAX_DEPS);
    memcpy(deps[s], tmp, sizeof(Departure) * depCount[s]);
    lastOk[s] = time(nullptr);
  }
  xSemaphoreGive(depMutex);
}

String cityPrefix(const char* official) {
  const char* c = strstr(official, ", ");
  return c ? String(official).substring(0, c - official + 2) : String();
}

void copyDest(Departure& d, const char* to, const String& prefix) {
  if (prefix.length() && !strncmp(to, prefix.c_str(), prefix.length())) to += prefix.length();
  strlcpy(d.dest, to, sizeof d.dest);
}

bool fetchTrain(int s, const Station& st, uint32_t gen) {
  JsonDocument filter, doc;
  JsonObject c = filter["connections"][0].to<JsonObject>();
  c["from"]["departureTimestamp"] = true;
  c["from"]["delay"] = true;
  c["from"]["platform"] = true;
  c["from"]["station"]["name"] = true;
  c["to"]["station"]["name"] = true;
  c["products"] = true;
  c["sections"][0]["journey"]["category"] = true;
  c["sections"][0]["journey"]["number"] = true;

  String url = String(API) + "connections?direct=1&limit=16"
      "&fields[]=connections/from/departureTimestamp&fields[]=connections/from/delay"
      "&fields[]=connections/from/platform&fields[]=connections/from/station/name"
      "&fields[]=connections/to/station/name&fields[]=connections/products"
      "&fields[]=connections/sections/journey/category&fields[]=connections/sections/journey/number"
      "&from=" + urlEncode(st.query.c_str()) + "&to=" + urlEncode(st.to.c_str());
  if (!getJson(url, doc, filter)) return false;

  Departure tmp[16];
  int n = 0;
  time_t now = time(nullptr);
  for (JsonObject e : doc["connections"].as<JsonArray>()) {
    if (n >= 16) break;
    if (e["products"].size() != 1) continue;  // nur direkte Züge
    long ts = e["from"]["departureTimestamp"] | 0L;
    int dl = e["from"]["delay"] | 0;
    if (!ts || ts + dl * 60 < now - 30) continue;

    JsonObject jn;
    for (JsonObject sec : e["sections"].as<JsonArray>())
      if (sec["journey"].is<JsonObject>()) { jn = sec["journey"]; break; }

    Departure& d = tmp[n++];
    strlcpy(d.cat, jn["category"] | "", sizeof d.cat);
    strlcpy(d.line, jn["number"] | "", sizeof d.line);
    copyDest(d, e["to"]["station"]["name"] | st.to.c_str(), cityPrefix(e["from"]["station"]["name"] | ""));
    strlcpy(d.platform, e["from"]["platform"] | "", sizeof d.platform);
    d.when = ts + dl * 60;
    d.delay = dl;
    d.walk = st.walk * 60;
  }
  storeDepartures(s, tmp, n, gen);
  return true;
}

bool fetchStop(int s, const Station& st, uint32_t gen) {
  JsonDocument filter, doc;
  JsonObject b = filter["stationboard"][0].to<JsonObject>();
  b["category"] = true;
  b["number"] = true;
  b["to"] = true;
  b["stop"]["departureTimestamp"] = true;
  b["stop"]["delay"] = true;
  filter["station"]["name"] = true;

  String url = String(API) + "stationboard?limit=30&fields[]=station/name"
      "&fields[]=stationboard/category&fields[]=stationboard/number&fields[]=stationboard/to"
      "&fields[]=stationboard/stop/departureTimestamp&fields[]=stationboard/stop/delay&station=" +
      urlEncode(st.query.c_str());
  if (!getJson(url, doc, filter)) return false;

  String prefix = cityPrefix(doc["station"]["name"] | st.query.c_str());
  Departure tmp[16];
  int n = 0;
  time_t now = time(nullptr);
  for (JsonObject e : doc["stationboard"].as<JsonArray>()) {
    if (n >= 16) break;
    long ts = e["stop"]["departureTimestamp"] | 0L;
    int dl = e["stop"]["delay"] | 0;
    if (!ts || ts + dl * 60 < now - 30) continue;
    const char* num = e["number"] | "";
    if (!lineAllowed(st.lines.c_str(), num)) continue;

    Departure& d = tmp[n++];
    strlcpy(d.cat, e["category"] | "", sizeof d.cat);
    strlcpy(d.line, num, sizeof d.line);
    copyDest(d, e["to"] | "", prefix);
    d.platform[0] = 0;
    d.when = ts + dl * 60;
    d.delay = dl;
    d.walk = st.walk * 60;
  }
  storeDepartures(s, tmp, n, gen);
  return true;
}

void clearDepartures() {
  xSemaphoreTake(depMutex, portMAX_DELAY);
  memset(depCount, 0, sizeof depCount);
  memset(lastOk, 0, sizeof lastOk);
  xSemaphoreGive(depMutex);
}

// Neuestes GitHub-Release prüfen (Tag "v<Build>") und bei neuerer Version installieren
void checkUpdate() {
  if (!*GITHUB_REPO) return;
  JsonDocument filter, doc;
  filter["tag_name"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  if (!getJson(String("https://api.github.com/repos/") + GITHUB_REPO + "/releases/latest", doc, filter)) return;
  const char* tag = doc["tag_name"] | "";
  if (*tag != 'v' || atoi(tag + 1) <= FW_BUILD) return;
  const char* url = nullptr;
  for (JsonObject a : doc["assets"].as<JsonArray>())
    if (!strcmp(a["name"] | "", "firmware.bin")) url = a["browser_download_url"];
  if (!url) return;

  http.end();
  tls.stop();
  updating = true;
  delay(1500);  // Hinweis auf dem Display anzeigen lassen
  WiFiClientSecure c;
  c.setInsecure();
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);
  httpUpdate.update(c, url);  // startet bei Erfolg neu
  updating = false;
}

void fetchTask(void*) {
  tls.setInsecure();
  http.setReuse(true);
  http.setTimeout(8000);
  checkUpdate();
  uint32_t lastUpdateCheck = millis();
  for (;;) {
    if (millis() - lastUpdateCheck > UPDATE_INTERVAL_MS) {
      lastUpdateCheck = millis();
      checkUpdate();
    }
    if (!nightNow && WiFi.status() == WL_CONNECTED) {
      Station st[MAX_ST];
      xSemaphoreTake(cfgMutex, portMAX_DELAY);
      int n = cfgNumSt;
      uint32_t gen = cfgGen;
      for (int i = 0; i < n; i++) st[i] = cfgSt[i];
      xSemaphoreGive(cfgMutex);
      for (int s = 0; s < n && gen == cfgGen; s++)
        st[s].train ? fetchTrain(s, st[s], gen) : fetchStop(s, st[s], gen);
    } else if (!nightNow) {
      WiFi.reconnect();
    }
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(FETCH_INTERVAL_MS));
  }
}

// ===================== Zeichnen: Tafel =====================
const uint16_t PIX_BUS[8]   = {0x0FE, 0x1FF, 0x125, 0x125, 0x1FF, 0x1FF, 0x0C6, 0x0C6};
const uint16_t PIX_TRAM[8]  = {0x07C, 0x010, 0x1FF, 0x155, 0x1FF, 0x1FF, 0x082, 0x1FF};
const uint16_t PIX_TRAIN[8] = {0x0FC, 0x1B6, 0x1FF, 0x1FF, 0x101, 0x1FF, 0x092, 0x1FF};
static const int PIX_VEH_W = 27, VEH_W = 26;

void drawVehicle(TFT_eSPI& g, int xRight, int cy, uint16_t c) {
  const int w = 26, h = 14, x = xRight - w, y = cy - h / 2 - 2;
  g.fillRoundRect(x, y, w, h, 3, c);
  for (int i = 0; i < 3; i++) g.fillRect(x + 3 + i * 8, y + 3, 6, 5, C_BG);
  g.fillCircle(x + 6, y + h + 1, 2, c);
  g.fillCircle(x + w - 7, y + h + 1, 2, c);
}

void drawPixelVehicle(TFT_eSPI& g, int xRight, int cy, const char* cat, uint16_t c) {
  const uint16_t* bm = isBus(cat) ? PIX_BUS : isTram(cat) ? PIX_TRAM : PIX_TRAIN;
  const int x0 = xRight - PIX_VEH_W, y0 = cy - 12;
  for (int ry = 0; ry < 8; ry++)
    for (int cx = 0; cx < 9; cx++)
      if (bm[ry] & (1 << (8 - cx))) g.fillRect(x0 + cx * 3, y0 + ry * 3, 2, 2, c);
}

// Kursiv wie die SBB-Zugsignete: Text ins Hilfs-Sprite, dann zeilenweise versetzt kopieren
void drawItalic(TFT_eSPI& g, const char* txt, int cx, int cy, uint16_t color) {
  const int H = badgeSpr.height(), W = badgeSpr.width();
  int w = min(txtWidth(badgeSpr, txt, SMALL), W);
  badgeSpr.fillSprite(TFT_BLACK);
  txtDraw(badgeSpr, txt, 0, pixelFont ? 19 : 18, SMALL, TFT_WHITE);
  int x0 = cx - w / 2 - 2, y0 = cy - H / 2;
  for (int y = 0; y < H; y++) {
    int shift = (H - 1 - y) / 4;
    for (int x = 0; x < w; x++)
      if (badgeSpr.readPixel(x, y)) g.drawPixel(x0 + x + shift, y0 + y, color);
  }
}

// LED-Tafel: Linie nur als Leuchtschrift, bei Platzmangel kleiner
void drawPixelLine(TFT_eSPI& g, const char* label, int x) {
  const int maxW = DEST_X - x - 8;
  if (dotWidth(label, BIG) <= maxW) dotDraw(g, label, x, BASE, BIG, C_WHITE);
  else drawLabel(g, label, x, BASE - 1, maxW, C_WHITE, SMALL);
}

void drawBadge(TFT_eSPI& g, const Departure& d, int x, int cy) {
  const char* cat = d.cat;
  bool tram = isTram(cat), bus = isBus(cat), boat = isBoat(cat);
  const int y = cy - BADGE_H / 2;
  const char* num = strlen(d.line) <= 3 ? d.line : "";  // lange Zugnummern (z. B. 000095) weglassen

  if (pixelFont) {
    char label[12];
    if (tram || bus || boat) snprintf(label, sizeof label, "%s", d.line[0] ? d.line : cat);
    else snprintf(label, sizeof label, "%s%s", cat, num);
    drawPixelLine(g, label, x);
    return;
  }

  if (tram || bus || boat) {
    const char* label = d.line[0] ? d.line : cat;
    const int w = 42;
    uint16_t bg = tram ? tramColor(d.line) : boat ? rgb(0x00, 0x9E, 0xE0) : TFT_WHITE;
    g.fillRoundRect(x, y, w, BADGE_H, 3, bg);
    if (lum565(bg) < 50) g.drawRoundRect(x, y, w, BADGE_H, 3, TFT_WHITE);
    txtDraw(g, label, x + (w - txtWidth(g, label, SMALL)) / 2, cy + 6, SMALL,
            lum565(bg) > 150 ? TFT_BLACK : TFT_WHITE);
    return;
  }

  bool sbahn = isCat(cat, {"S", "SN"}), regio = isCat(cat, {"RE", "R", "REX"});
  char label[12];
  snprintf(label, sizeof label, (sbahn || !*num) ? "%s%s" : "%s %s", cat, num);
  int w = constrain(txtWidth(g, label, SMALL) + 12, 42, 54);
  uint16_t bg = (sbahn || regio) ? TFT_WHITE : C_RED;
  uint16_t fg = sbahn ? TFT_BLACK : regio ? C_RED : TFT_WHITE;
  uint16_t border = sbahn ? TFT_BLACK : C_RED;
  g.fillRect(x, y, w, BADGE_H, bg);
  g.drawRect(x, y, w, BADGE_H, border);
  if (regio) g.drawRect(x + 1, y + 1, w - 2, BADGE_H - 2, border);
  drawItalic(g, label, x + w / 2, cy, fg);
}

void drawTitleBar(const char* title) {
  hdr.fillSprite(C_HDR);
  drawLabel(hdr, title, 8, HDR_BASE, 304, C_WHITE);
  hdr.pushSprite(0, 0);
}

// Kopfzeile nur bei Änderung (Titel, Minute, Status); der blinkende Doppelpunkt direkt aufs Display
void drawHeader(const tm& t, bool colon, bool stale) {
  Hash sig;
  sig.val(t.tm_hour).val(t.tm_min).val(stale).val(cfgGen).str(cfgTitle.c_str());
  if (sig.get() != hdrSig) {
    hdrSig = sig.get();
    hdr.fillSprite(C_HDR);
    drawLabel(hdr, cfgTitle.c_str(), 8, HDR_BASE, 220, C_WHITE);
    char hh[4], mm[4];
    snprintf(hh, sizeof hh, "%02d", t.tm_hour);
    snprintf(mm, sizeof mm, "%02d", t.tm_min);
    txtDraw(hdr, hh, CLOCK_X - 4 - txtWidth(hdr, hh, BIG), HDR_BASE, BIG, C_WHITE);
    txtDraw(hdr, mm, CLOCK_X + 4, HDR_BASE, BIG, C_WHITE);
    if (stale) hdr.fillCircle(240, HEADER_H / 2, 4, C_YELLOW);
    hdr.pushSprite(0, 0);
  }
  const int s = pixelFont ? 2 : 3;
  const uint16_t c = colon ? C_WHITE : C_HDR;
  tft.fillRect(CLOCK_X - 1, HDR_BASE - (pixelFont ? 15 : 14), s, s, c);
  tft.fillRect(CLOCK_X - 1, HDR_BASE - 6, s, s, c);
}

inline int rowY(int r) { return HEADER_H + r * ROW_H; }

void fillBottom() {
  if (!bottomDirty) return;
  bottomDirty = false;
  int y = rowY(MAX_ROWS);
  tft.fillRect(0, y, SCREEN_W, tft.height() - y, C_BG);
}

void beginRow() {
  row.fillSprite(C_BG);
  row.drawFastHLine(0, ROW_H - 1, SCREEN_W, C_LINE);
}

void drawMessageRow(int r, const char* msg) {
  uint32_t sig = Hash().str(msg).val(cfgGen).get();
  if (sig == rowSig[r]) return;
  rowSig[r] = sig;
  beginRow();
  drawLabel(row, msg, DEST_X, BASE, 250, C_LABEL);
  row.pushSprite(0, rowY(r));
}

void drawRow(int r, const Departure* d, time_t now, bool blink) {
  char buf[8] = "";
  uint16_t c = 0;
  bool leaving = false;
  Hash sig;
  sig.val(cfgGen);
  if (d) {
    long diff = (long)(d->when - now);
    c = d->delay > 0 ? C_YELLOW : C_WHITE;
    if (d->walk > 0 && diff - d->walk < 60) c = C_GO;
    leaving = diff < 60;
    if (!leaving) {
      long m = diff / 60;
      if (m < 60) snprintf(buf, sizeof buf, "%ld'", m);
      else { struct tm t; localtime_r(&d->when, &t); strftime(buf, sizeof buf, "%H:%M", &t); }
    }
    sig.add(d, offsetof(Departure, when)).str(buf).val(c).val(leaving && blink);
  }
  if (sig.get() == rowSig[r]) return;
  rowSig[r] = sig.get();

  beginRow();
  if (d) {
    drawBadge(row, *d, 6, ROW_H / 2);
    int minW;
    if (leaving) {
      minW = pixelFont ? PIX_VEH_W : VEH_W;
      if (blink) pixelFont ? drawPixelVehicle(row, MIN_RIGHT, ROW_H / 2, d->cat, c)
                           : drawVehicle(row, MIN_RIGHT, ROW_H / 2, c);
    } else {
      minW = txtWidth(row, buf, BIG);
      txtDraw(row, buf, MIN_RIGHT - minW, BASE, BIG, c);
    }
    int right = MIN_RIGHT - minW - 10;
    if (d->platform[0]) {
      char pl[14];
      snprintf(pl, sizeof pl, "Gl. %s", d->platform);
      int pw = txtWidth(row, pl, SMALL);
      txtDraw(row, pl, right - pw, BASE - (pixelFont ? 2 : 1), SMALL, C_LABEL);
      right -= pw + 10;
    }
    drawLabel(row, d->dest, DEST_X, BASE, right - DEST_X, C_WHITE);
  }
  row.pushSprite(0, rowY(r));
}

void drawStatus(const char* msg) {
  invalidate();
  drawTitleBar(cfgTitle.c_str());
  drawMessageRow(0, msg);
  for (int r = 1; r < MAX_ROWS; r++) drawRow(r, nullptr, 0, false);
  fillBottom();
}

// ===================== QR-Bildschirme =====================
void drawQR(const char* text, int x, int y, int maxSize) {
  static uint8_t buf[((7 * 4 + 17) * (7 * 4 + 17) + 7) / 8];
  static const int CAP[] = {0, 17, 32, 53, 78, 106, 134, 154};  // Byte-Modus, ECC_LOW
  QRCode qr;
  int len = strlen(text), ver = 1;
  while (ver <= 7 && CAP[ver] < len) ver++;
  if (ver > 7) return;
  qrcode_initText(&qr, buf, ver, ECC_LOW, text);
  int scale = maxSize / (qr.size + 4), total = scale * (qr.size + 4);
  tft.fillRect(x, y, total, total, TFT_WHITE);
  for (int qy = 0; qy < qr.size; qy++)
    for (int qx = 0; qx < qr.size; qx++)
      if (qrcode_getModule(&qr, qx, qy))
        tft.fillRect(x + (qx + 2) * scale, y + (qy + 2) * scale, scale, scale, TFT_BLACK);
}

void drawQrScreen(const char* title, const String& qrText,
                  std::initializer_list<const char*> steps, const String& footer) {
  invalidate();
  tft.fillScreen(C_BG);
  drawTitleBar(title);
  drawQR(qrText.c_str(), 8, 46, 170);
  int y = 70;
  for (const char* s : steps) { drawLabel(tft, s, 190, y, 126, C_WHITE, UI19); y += 24; }
  drawLabel(tft, footer.c_str(), 8, 220, 304, C_LABEL, UI2);
}

void showInfo(uint32_t ms) {
  String url = "http://" + WiFi.localIP().toString() + "/";
  drawQrScreen("Einstellungen", url,
               {"Code mit dem", "Handy scannen", "", "Handy muss im", "selben WLAN", "sein"}, url);
  infoUntil = millis() + ms;
  if (!infoUntil) infoUntil = 1;
}

// ===================== Reset / BOOT-Taste =====================
[[noreturn]] void restartWith(const char* title) {
  tft.fillScreen(C_BG);
  drawTitleBar(title);
  invalidate();
  drawMessageRow(0, "Starte neu...");
  delay(1500);
  ESP.restart();
  for (;;) {}
}

void resetWifi() {
  prefs.remove("ssid");
  prefs.remove("pass");
  restartWith("WLAN zurückgesetzt");
}

void factoryReset() {
  prefs.clear();
  restartWith("Werkseinstellungen");
}

inline bool awake() { return wakeUntil && (int32_t)(millis() - wakeUntil) < 0; }

void handleButton() {
  static uint32_t downAt = 0;
  static bool longDone = false;
  bool down = digitalRead(BOOT_PIN) == LOW;
  if (down && !downAt) { downAt = millis(); longDone = false; }
  if (down && !longDone && millis() - downAt > 5000) { longDone = true; resetWifi(); }
  if (!down && downAt) {
    if (!longDone && millis() - downAt > 50) {
      if (nightNow) {
        wakeUntil = millis() + 120000;
        if (fetchHandle) xTaskNotifyGive(fetchHandle);
      } else if (infoUntil) {
        infoUntil = 0;
      } else {
        showInfo(60000);
      }
    }
    downAt = 0;
  }
}

// ===================== Webserver =====================
void sendPage(const char* body) {
  server.setContentLength(strlen(PAGE_HEAD) + strlen(body));
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent(PAGE_HEAD);
  server.sendContent(body);
}

void sendJson(JsonDocument& d) {
  String s;
  serializeJson(d, s);
  server.send(200, "application/json", s);
}

void sendOk() { server.send(200, "text/plain", "ok"); }

String scanJson() {
  int n = WiFi.scanNetworks();
  std::vector<int> idx;
  for (int i = 0; i < n; i++) if (WiFi.SSID(i).length()) idx.push_back(i);
  std::sort(idx.begin(), idx.end(), [](int a, int b) { return WiFi.RSSI(a) > WiFi.RSSI(b); });
  JsonDocument d;
  JsonArray a = d.to<JsonArray>();
  std::vector<String> seen;
  for (int i : idx) {
    String s = WiFi.SSID(i);
    if (std::find(seen.begin(), seen.end(), s) != seen.end()) continue;
    seen.push_back(s);
    JsonObject o = a.add<JsonObject>();
    o["ssid"] = s;
    o["lock"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    if (seen.size() >= 20) break;
  }
  WiFi.scanDelete();
  String out;
  serializeJson(d, out);
  return out;
}

[[noreturn]] void runPortal(bool hadWifi) {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);
  String networks = scanJson();

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char apSsid[24], apPass[9];
  snprintf(apSsid, sizeof apSsid, "Abfahrtstafel-%02X%02X", mac[4], mac[5]);
  static const char chars[] = "abcdefghjkmnpqrstuvwxyz23456789";
  for (int i = 0; i < 8; i++) apPass[i] = chars[esp_random() % (sizeof chars - 1)];
  apPass[8] = 0;

  WiFi.softAP(apSsid, apPass);
  delay(200);
  IPAddress ip = WiFi.softAPIP();
  String home = "http://" + ip.toString() + "/";
  dns.start(53, "*", ip);

  server.on("/", HTTP_GET, [] { sendPage(PORTAL_HTML); });
  server.on("/api/networks", HTTP_GET, [&networks] {
    if (server.hasArg("rescan")) networks = scanJson();
    server.send(200, "application/json", networks);
  });
  server.on("/save", HTTP_POST, [] {
    String ssid = server.arg("ssid");
    if (ssid == "__other") ssid = server.arg("other");
    ssid.trim();
    if (ssid.isEmpty()) { server.sendHeader("Location", "/"); server.send(302); return; }
    prefs.putString("ssid", ssid);
    prefs.putString("pass", server.arg("pass"));
    sendPage(SAVED_HTML);
    drawStatus("Gespeichert, starte neu...");
    delay(2000);
    ESP.restart();
  });
  server.onNotFound([home] { server.sendHeader("Location", home); server.send(302, "text/plain", ""); });
  server.begin();

  String wifiQr = String("WIFI:T:WPA;S:") + apSsid + ";P:" + apPass + ";;";
  String footer = String("WLAN: ") + apSsid + "   Passwort: " + apPass;
  const char* title = hadWifi ? "WLAN nicht erreichbar" : "WLAN einrichten";

  int lastClients = -1;
  uint32_t idleSince = millis();
  for (;;) {
    dns.processNextRequest();
    server.handleClient();
    int clients = WiFi.softAPgetStationNum();
    if (clients != lastClients) {
      lastClients = clients;
      if (clients == 0)
        drawQrScreen(title, wifiQr, {"1. Code mit der", "Handy-Kamera", "scannen", "2. Heim-WLAN", "auswählen"}, footer);
      else
        drawQrScreen("Fast geschafft", home, {"Seite öffnet", "sich von selbst.", "Sonst diesen", "Code scannen"},
                     "Adresse: " + home);
    }
    if (clients > 0) idleSince = millis();
    if (hadWifi && millis() - idleSince > PORTAL_RETRY_MS) ESP.restart();
    delay(5);
  }
}

void setupWeb() {
  server.on("/", HTTP_GET, [] { sendPage(CONFIG_HTML); });
  server.on("/api/config", HTTP_GET, [] {
    JsonDocument d;
    configToJson(d, true);
    sendJson(d);
  });
  server.on("/api/config", HTTP_POST, [] {
    JsonDocument d;
    if (deserializeJson(d, server.arg("plain")) || !d.is<JsonObject>()) {
      server.send(400, "text/plain", "Ungültige Daten");
      return;
    }
    applyConfig(d);
    saveConfig();
    clearDepartures();
    if (fetchHandle) xTaskNotifyGive(fetchHandle);
    infoUntil = 0;
    sendOk();
  });
  server.on("/api/wifireset", HTTP_POST, [] { sendOk(); delay(300); resetWifi(); });
  server.on("/api/factoryreset", HTTP_POST, [] { sendOk(); delay(300); factoryReset(); });
  server.onNotFound([] { server.send(404, "text/plain", "Nicht gefunden"); });
  server.begin();
}

// ===================== Setup / Loop =====================
void setup() {
  pinMode(BOOT_PIN, INPUT_PULLUP);
  cfgMutex = xSemaphoreCreateMutex();
  depMutex = xSemaphoreCreateMutex();
  prefs.begin("tafel", false);
  loadConfig();

  tft.init();
  tft.setRotation(1);
  tft.setTextWrap(false, false);
  tft.setAttribute(UTF8_SWITCH, 0);
  tft.fillScreen(C_BG);
  row.createSprite(SCREEN_W, ROW_H);
  hdr.createSprite(SCREEN_W, HEADER_H);
  badgeSpr.createSprite(64, 24);
  for (TFT_eSprite* s : {&row, &hdr, &badgeSpr}) {
    s->setTextWrap(false, false);
    s->setAttribute(UTF8_SWITCH, 0);
  }

  String ssid = prefs.getString("ssid", "");
  if (ssid.isEmpty()) runPortal(false);

  drawStatus("Verbinde WLAN...");
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setSleep(false);
  WiFi.begin(ssid.c_str(), prefs.getString("pass", "").c_str());
  for (uint32_t start = millis(); WiFi.status() != WL_CONNECTED; delay(250))
    if (millis() - start > WIFI_TIMEOUT_MS) runPortal(true);

  drawStatus("Warte auf Uhrzeit...");
  configTzTime(TZ_CH, "ch.pool.ntp.org", "pool.ntp.org");
  while (time(nullptr) < 1700000000) delay(200);

  setupWeb();
  xTaskCreatePinnedToCore(fetchTask, "fetch", 12288, nullptr, 1, &fetchHandle, 0);
  showInfo(20000);
}

void loop() {
  server.handleClient();
  handleButton();

  static int lastSec = -1;
  static bool updateShown = false;
  if (updating) {
    if (!updateShown) { updateShown = true; drawStatus("Update wird installiert..."); }
    delay(20);
    return;
  }
  if (updateShown) { updateShown = false; invalidate(); }

  if (infoUntil) {
    if ((int32_t)(millis() - infoUntil) < 0) { delay(5); return; }
    infoUntil = 0;
    lastSec = -1;
    invalidate();
  }

  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  if (t.tm_sec == lastSec) { delay(5); return; }
  lastSec = t.tm_sec;
  bool blink = !(t.tm_sec & 1);

  int mins = t.tm_hour * 60 + t.tm_min;
  bool dark = nightOn && nightFrom != nightTo && !awake() &&
              (nightFrom < nightTo ? (mins >= nightFrom && mins < nightTo)
                                   : (mins >= nightFrom || mins < nightTo));
  if (dark != nightNow) {
    nightNow = dark;
    if (dark) {
      tft.fillScreen(TFT_BLACK);
      tft.writecommand(TFT_DISPOFF);
      tft.writecommand(TFT_SLPIN);
    } else {
      tft.writecommand(TFT_SLPOUT);
      delay(120);
      tft.writecommand(TFT_DISPON);
      if (fetchHandle) xTaskNotifyGive(fetchHandle);
    }
    invalidate();
  }
  if (dark) return;

  Departure feed[MAX_ST * MAX_DEPS];
  int n = 0;
  bool anyOk = false, stale = false;
  xSemaphoreTake(depMutex, portMAX_DELAY);
  for (int s = 0; s < cfgNumSt; s++) {
    if (lastOk[s]) { anyOk = true; if (now - lastOk[s] > 180) stale = true; }
    for (int i = 0; i < depCount[s]; i++) {
      const Departure& d = deps[s][i];
      if (d.when < now - 30 || (d.walk > 0 && d.when - now < d.walk)) continue;
      feed[n++] = d;
    }
  }
  xSemaphoreGive(depMutex);

  for (int i = 1; i < n; i++) {
    Departure k = feed[i];
    int j = i - 1;
    while (j >= 0 && feed[j].when > k.when) { feed[j + 1] = feed[j]; j--; }
    feed[j + 1] = k;
  }

  drawHeader(t, blink, stale);
  int shown = min(n, MAX_ROWS);
  if (!anyOk || !shown) {
    drawMessageRow(0, cfgNumSt == 0 ? "Haltestelle einstellen" : !anyOk ? "Lade Abfahrten..." : "Keine Abfahrten");
    shown = 1;
  } else {
    for (int r = 0; r < shown; r++) drawRow(r, &feed[r], now, blink);
  }
  for (int r = shown; r < MAX_ROWS; r++) drawRow(r, nullptr, now, blink);
  fillBottom();
}
