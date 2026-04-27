#include <SPI.h>
#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_PN532.h>

// ============================================================
// ESP32 OpenSpool NFC Writer / Reader - COMPLETE VERSION
// ESP32-S2 + ST7789 + PN532 I2C
//
// KEY0 = select / OK        GPIO7
// KEY1 = back / hold home   GPIO5
//
// Supports:
// - OpenSpool NDEF application/json tags
// - RAW fallback / eSUN-style U1-compatible tags
// - NTAG215 / NTAG216 write, read, verify, clone
// - Generic profile library
// - Custom profiles in flash
// - Partial redraw UI
// ============================================================

// =========================
// TFT pins
// =========================
#define TFT_CS    12
#define TFT_DC    16
#define TFT_RST   17
#define TFT_SCLK  10
#define TFT_MOSI  11
#define TFT_BLK    9

// =========================
// Encoder / buttons
// =========================
#define ENC_A      2
#define ENC_B      3
#define KEY0       7   // select / OK
#define KEY1       5   // back / hold = home

// =========================
// PN532 pins (I2C)
// =========================
#define PN532_SDA  33
#define PN532_SCL  34

// =========================
// Hardware
// =========================
SPIClass mySPI(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&mySPI, TFT_CS, TFT_DC, TFT_RST);
Adafruit_PN532 nfc(PN532_SDA, PN532_SCL);
Preferences prefs;

// =========================
// Colors
// =========================
#define COL_BG       ST77XX_BLACK
#define COL_FRAME    ST77XX_YELLOW
#define COL_TITLE    ST77XX_WHITE
#define COL_TEXT     ST77XX_WHITE
#define COL_DIM      0x8410
#define COL_OK       ST77XX_GREEN
#define COL_WARN     ST77XX_YELLOW
#define COL_ERR      ST77XX_RED
#define COL_TOPBAR   0x2104
#define COL_HILITE   ST77XX_CYAN
#define COL_PANEL    0x18E3

// =========================
// Backlight PWM
// ESP32 Arduino Core 3.x style
// =========================
const int BL_FREQ = 5000;
const int BL_RES  = 8;
int backlightValue = 180;

// =========================
// Layout
// =========================
const int TOPBAR_H = 30;
const int MENU_Y = 58;
const int MENU_ROW_H = 30;
const int VISIBLE_ROWS = 4;
const int FOOTER_Y = 186;

// =========================
// TagProfile
// =========================
struct TagProfile {
  char name[40];
  char brand[20];
  char type[12];
  char subtype[16];
  char colorHex[10];
  char alpha[3];
  int minTemp;
  int maxTemp;
  int bedMin;
  int bedMax;
  int weight;
};

// =========================
// Generic library
// =========================
TagProfile genericProfiles[] = {
  {"Generic PLA White",       "Generic", "PLA",  "Basic", "#FFFFFF", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Black",       "Generic", "PLA",  "Basic", "#111111", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Gray",        "Generic", "PLA",  "Basic", "#888888", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Red",         "Generic", "PLA",  "Basic", "#CC2020", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Blue",        "Generic", "PLA",  "Basic", "#2040FF", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Green",       "Generic", "PLA",  "Basic", "#20AA40", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Yellow",      "Generic", "PLA",  "Basic", "#F0D000", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Orange",      "Generic", "PLA",  "Basic", "#F28C28", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Natural",     "Generic", "PLA",  "Basic", "#F2EFE9", "FF", 190, 220, 50,  60,  1000},
  {"Generic PLA Matte Black", "Generic", "PLA",  "Matte", "#111111", "FF", 195, 225, 50,  60,  1000},
  {"Generic PLA Silk Gold",   "Generic", "PLA",  "Silk",  "#D4AF37", "FF", 205, 225, 50,  60,  1000},
  {"Generic PLA CF Black",    "Generic", "PLA",  "CF",    "#101010", "FF", 210, 240, 45,  60,  1000},

  {"Generic PETG White",      "Generic", "PETG", "Basic", "#FFFFFF", "FF", 230, 250, 70,  85,  1000},
  {"Generic PETG Black",      "Generic", "PETG", "Basic", "#111111", "FF", 230, 250, 70,  85,  1000},
  {"Generic PETG Gray",       "Generic", "PETG", "Basic", "#8A8A8A", "FF", 230, 250, 70,  85,  1000},
  {"Generic PETG Red",        "Generic", "PETG", "Basic", "#CC2020", "FF", 230, 250, 70,  85,  1000},
  {"Generic PETG Blue",       "Generic", "PETG", "Basic", "#2040FF", "FF", 230, 250, 70,  85,  1000},
  {"Generic PETG Natural",    "Generic", "PETG", "Basic", "#E8E8D8", "FF", 230, 250, 70,  85,  1000},
  {"Generic PETG CF Black",   "Generic", "PETG", "CF",    "#101010", "FF", 240, 260, 70,  90,  1000},

  {"Generic ABS White",       "Generic", "ABS",  "Basic", "#FFFFFF", "FF", 240, 260, 90,  110, 1000},
  {"Generic ABS Black",       "Generic", "ABS",  "Basic", "#111111", "FF", 240, 260, 90,  110, 1000},
  {"Generic ABS Gray",        "Generic", "ABS",  "Basic", "#8A8A8A", "FF", 240, 260, 90,  110, 1000},
  {"Generic ABS CF Black",    "Generic", "ABS",  "CF",    "#101010", "FF", 250, 270, 95,  115, 1000},

  {"Generic TPU White",       "Generic", "TPU",  "95A",   "#F5F5F5", "FF", 210, 235, 40,  60,  1000},
  {"Generic TPU Black",       "Generic", "TPU",  "95A",   "#111111", "FF", 210, 235, 40,  60,  1000},
  {"Generic TPU Gray",        "Generic", "TPU",  "95A",   "#8A8A8A", "FF", 210, 235, 40,  60,  1000},

  {"Generic ASA White",       "Generic", "ASA",  "Basic", "#FFFFFF", "FF", 245, 265, 90,  110, 1000},
  {"Generic ASA Black",       "Generic", "ASA",  "Basic", "#111111", "FF", 245, 265, 90,  110, 1000},
  {"Generic ASA Gray",        "Generic", "ASA",  "Basic", "#8A8A8A", "FF", 245, 265, 90,  110, 1000},
  {"Generic ASA CF Black",    "Generic", "ASA",  "CF",    "#101010", "FF", 255, 275, 95,  115, 1000}
};

const int genericProfileCount = sizeof(genericProfiles) / sizeof(genericProfiles[0]);

// =========================
// Custom profiles in flash
// =========================
const int MAX_CUSTOM_PROFILES = 12;
TagProfile customProfiles[MAX_CUSTOM_PROFILES];
int customProfileCount = 0;

// =========================
// Active profile source
// =========================
enum ProfileSourceKind {
  SRC_GENERIC,
  SRC_CUSTOM,
  SRC_TAG_READ
};

TagProfile activeProfile;
ProfileSourceKind activeProfileSource = SRC_GENERIC;

// =========================
// Editor state
// =========================
TagProfile editorProfile;
int editorCustomIndex = -1;
int selectedCustomIndex = -1;

// =========================
// Read-tag source buffers
// =========================
char readJsonBuffer[320];

// =========================
// Editor options
// =========================
const char* typeOptions[] = {"PLA", "PETG", "ABS", "TPU", "ASA"};
const int typeCount = sizeof(typeOptions) / sizeof(typeOptions[0]);

const char* subtypeOptions[] = {"Basic", "Matte", "Silk", "CF", "95A"};
const int subtypeCount = sizeof(subtypeOptions) / sizeof(subtypeOptions[0]);

const char* colorNames[] = {"White", "Black", "Gray", "Red", "Blue", "Green", "Yellow", "Orange", "Natural"};
const char* colorHexes[] = {"#FFFFFF", "#111111", "#888888", "#CC2020", "#2040FF", "#20AA40", "#F0D000", "#F28C28", "#F2EFE9"};
const int colorCount = sizeof(colorNames) / sizeof(colorNames[0]);

int editorTypeIndex = 0;
int editorSubtypeIndex = 0;
int editorColorIndex = 0;

// =========================
// UI state
// =========================
enum ScreenState {
  SCREEN_HOME,
  SCREEN_PROFILE_SOURCE,
  SCREEN_GENERIC_LIST,
  SCREEN_CUSTOM_LIST,
  SCREEN_CUSTOM_ACTIONS,
  SCREEN_PROFILE_VIEW,
  SCREEN_CUSTOM_EDITOR,
  SCREEN_ACTION_RESULT
};

ScreenState currentScreen = SCREEN_HOME;

int selectedIndex = 0;
int topIndex = 0;
bool lastKey0 = HIGH;
bool lastKey1 = HIGH;

unsigned long key1PressTime = 0;
bool key1LongHandled = false;
const unsigned long KEY1_LONG_MS = 900;

bool nfcReady = false;

// Dirty flags
bool redrawFull = true;
bool redrawMenu = false;
bool redrawFooter = false;
bool redrawBody = false;

// =========================
// Robust encoder decode
// =========================
uint8_t encPrevState = 0;
int8_t encAcc = 0;
unsigned long lastEncMoveMs = 0;
unsigned long lastMenuStepMs = 0;
const unsigned long ENC_DEBOUNCE_MS = 2;

// =========================
// Status buffers
// =========================
char statusLine[64] = "Ready";
char resultLine1[64] = "";
char resultLine2[64] = "";
char resultLine3[64] = "";
char lastUidStr[64] = "No tag";

// =========================
// Buffers
// =========================
char jsonBuffer[320];
uint8_t ndefBuffer[384];
size_t ndefLength = 0;

// ============================================================
// Utility helpers
// ============================================================
void copyProfile(TagProfile& dst, const TagProfile& src) {
  memcpy(&dst, &src, sizeof(TagProfile));
}

void clearProfile(TagProfile& p) {
  memset(&p, 0, sizeof(TagProfile));
}

bool activeProfileIsCustom() {
  return activeProfileSource == SRC_CUSTOM;
}

void setStatus(const char* s) {
  strncpy(statusLine, s, sizeof(statusLine) - 1);
  statusLine[sizeof(statusLine) - 1] = '\0';
  redrawBody = true;
}

void setResult(const char* a = "", const char* b = "", const char* c = "") {
  strncpy(resultLine1, a, sizeof(resultLine1) - 1);
  resultLine1[sizeof(resultLine1) - 1] = '\0';

  strncpy(resultLine2, b, sizeof(resultLine2) - 1);
  resultLine2[sizeof(resultLine2) - 1] = '\0';

  strncpy(resultLine3, c, sizeof(resultLine3) - 1);
  resultLine3[sizeof(resultLine3) - 1] = '\0';

  redrawBody = true;
}

void drawCenteredText(const char* txt, int y, uint8_t size, uint16_t color) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.setTextWrap(false);
  tft.getTextBounds((char*)txt, 0, y, &x1, &y1, &w, &h);
  int16_t x = (tft.width() - w) / 2;
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.print(txt);
}

void setBacklight(int value) {
  backlightValue = constrain(value, 5, 255);
  ledcWrite(TFT_BLK, backlightValue);
}

uint16_t hexTo565(const char* hex) {
  if (!hex || strlen(hex) < 7 || hex[0] != '#') return ST77XX_WHITE;

  char rs[3] = {hex[1], hex[2], 0};
  char gs[3] = {hex[3], hex[4], 0};
  char bs[3] = {hex[5], hex[6], 0};

  int r = strtol(rs, NULL, 16);
  int g = strtol(gs, NULL, 16);
  int b = strtol(bs, NULL, 16);

  return tft.color565(r, g, b);
}

void formatUidToString(const uint8_t *uid, uint8_t uidLength, char *out, size_t outSize) {
  out[0] = '\0';
  for (uint8_t i = 0; i < uidLength; i++) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%02X", uid[i]);
    strncat(out, buf, outSize - strlen(out) - 1);
    if (i < uidLength - 1) strncat(out, ":", outSize - strlen(out) - 1);
  }
}

void printHexLine(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    if (i + 1 < len) Serial.print(' ');
  }
  Serial.println();
}

void cleanCString(char* s) {
  if (!s) return;

  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\0' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
    s[len - 1] = '\0';
    len--;
  }

  char* start = s;
  while (*start == ' ') start++;

  if (start != s) {
    memmove(s, start, strlen(start) + 1);
  }
}

bool waitForTag(uint8_t *uid, uint8_t *uidLength, unsigned long timeoutMs) {
  unsigned long startMs = millis();

  while (millis() - startMs < timeoutMs) {
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength, 100)) {
      return true;
    }
    delay(20);
  }

  return false;
}

bool isLikelyNtagUid(uint8_t uidLength) {
  return uidLength == 7;
}

bool rejectUnsupportedUidIfNeeded(uint8_t uidLength) {
  if (uidLength == 4) {
    setStatus("Unsupported tag");
    setResult("4-byte UID detected", "Likely locked/proprietary", "Use NTAG215/216");
    return true;
  }

  if (!isLikelyNtagUid(uidLength)) {
    setStatus("Unknown tag type");
    setResult("UID length not 7 byte", "Use NTAG215/216", "");
    return true;
  }

  return false;
}

void resetMenu() {
  selectedIndex = 0;
  topIndex = 0;
}

void ensureSelectionVisible(int count) {
  if (count <= 0) return;

  if (selectedIndex < topIndex) topIndex = selectedIndex;
  if (selectedIndex >= topIndex + VISIBLE_ROWS) topIndex = selectedIndex - VISIBLE_ROWS + 1;
  if (topIndex < 0) topIndex = 0;
  if (topIndex > count - VISIBLE_ROWS) topIndex = max(0, count - VISIBLE_ROWS);
}

void moveSelection(int delta, int count) {
  if (count <= 0) return;

  int oldSel = selectedIndex;
  int oldTop = topIndex;

  selectedIndex += delta;

  if (selectedIndex < 0) selectedIndex = 0;
  if (selectedIndex >= count) selectedIndex = count - 1;

  ensureSelectionVisible(count);

  if (oldSel != selectedIndex || oldTop != topIndex) {
    redrawMenu = true;
  }
}

const char* sourceToString(ProfileSourceKind s) {
  switch (s) {
    case SRC_GENERIC: return "Generic";
    case SRC_CUSTOM:  return "Custom";
    case SRC_TAG_READ:return "TagRead";
    default:          return "Unknown";
  }
}

uint16_t sourceColor(ProfileSourceKind s) {
  switch (s) {
    case SRC_CUSTOM:   return COL_WARN;
    case SRC_TAG_READ: return COL_HILITE;
    default:           return COL_OK;
  }
}

int homeItemCount() { return 7; }
int profileSourceCount() { return 3; }
int genericListCount() { return genericProfileCount + 1; }
int customListCount() { return customProfileCount + 2; }
int customActionsCount() { return 4; }
int customEditorCount() { return 10; }

int getCurrentMenuCount() {
  switch (currentScreen) {
    case SCREEN_HOME:           return homeItemCount();
    case SCREEN_PROFILE_SOURCE: return profileSourceCount();
    case SCREEN_GENERIC_LIST:   return genericListCount();
    case SCREEN_CUSTOM_LIST:    return customListCount();
    case SCREEN_CUSTOM_ACTIONS: return customActionsCount();
    case SCREEN_CUSTOM_EDITOR:  return customEditorCount();
    default:                    return 0;
  }
}

// ============================================================
// Flash helpers
// ============================================================
void loadCustomProfiles() {
  prefs.begin("u1writer", true);

  customProfileCount = prefs.getUInt("customCount", 0);

  if (customProfileCount < 0) customProfileCount = 0;
  if (customProfileCount > MAX_CUSTOM_PROFILES) customProfileCount = MAX_CUSTOM_PROFILES;

  for (int i = 0; i < customProfileCount; i++) {
    char key[12];
    snprintf(key, sizeof(key), "p%d", i);

    size_t got = prefs.getBytesLength(key);
    if (got == sizeof(TagProfile)) {
      prefs.getBytes(key, &customProfiles[i], sizeof(TagProfile));
    } else {
      clearProfile(customProfiles[i]);
    }
  }

  prefs.end();
}

void saveCustomProfiles() {
  prefs.begin("u1writer", false);

  prefs.putUInt("customCount", customProfileCount);

  for (int i = 0; i < customProfileCount; i++) {
    char key[12];
    snprintf(key, sizeof(key), "p%d", i);
    prefs.putBytes(key, &customProfiles[i], sizeof(TagProfile));
  }

  for (int i = customProfileCount; i < MAX_CUSTOM_PROFILES; i++) {
    char key[12];
    snprintf(key, sizeof(key), "p%d", i);
    prefs.remove(key);
  }

  prefs.end();
}

bool addCustomProfile(const TagProfile& p) {
  if (customProfileCount >= MAX_CUSTOM_PROFILES) return false;

  copyProfile(customProfiles[customProfileCount], p);
  customProfileCount++;
  saveCustomProfiles();

  return true;
}

bool updateCustomProfile(int idx, const TagProfile& p) {
  if (idx < 0 || idx >= customProfileCount) return false;

  copyProfile(customProfiles[idx], p);
  saveCustomProfiles();

  return true;
}

bool deleteCustomProfile(int idx) {
  if (idx < 0 || idx >= customProfileCount) return false;

  for (int i = idx; i < customProfileCount - 1; i++) {
    copyProfile(customProfiles[i], customProfiles[i + 1]);
  }

  customProfileCount--;
  saveCustomProfiles();

  return true;
}

// ============================================================
// Profile helpers
// ============================================================
void activateGenericProfile(int idx) {
  copyProfile(activeProfile, genericProfiles[idx]);
  activeProfileSource = SRC_GENERIC;
}

void activateCustomProfile(int idx) {
  copyProfile(activeProfile, customProfiles[idx]);
  activeProfileSource = SRC_CUSTOM;
}

void normalizeProfile(TagProfile& p) {
  cleanCString(p.brand);
  cleanCString(p.type);
  cleanCString(p.subtype);
  cleanCString(p.colorHex);
  cleanCString(p.alpha);

  if (!strlen(p.brand)) strncpy(p.brand, "Generic", sizeof(p.brand) - 1);
  if (!strlen(p.type)) strncpy(p.type, "PETG", sizeof(p.type) - 1);
  if (!strlen(p.subtype)) strncpy(p.subtype, "Basic", sizeof(p.subtype) - 1);
  if (!strlen(p.colorHex)) strncpy(p.colorHex, "#FFFFFF", sizeof(p.colorHex) - 1);

  if (p.colorHex[0] != '#') {
    char tmp[10];
    snprintf(tmp, sizeof(tmp), "#%s", p.colorHex);
    strncpy(p.colorHex, tmp, sizeof(p.colorHex) - 1);
    p.colorHex[sizeof(p.colorHex) - 1] = '\0';
  }

  if (!strlen(p.alpha)) strncpy(p.alpha, "FF", sizeof(p.alpha) - 1);

  if (p.minTemp <= 0) p.minTemp = 220;
  if (p.maxTemp <= 0) p.maxTemp = 250;
  if (p.bedMin <= 0) p.bedMin = 70;
  if (p.bedMax <= 0) p.bedMax = 80;
  if (p.weight <= 0) p.weight = 1000;

  if (p.maxTemp < p.minTemp) p.maxTemp = p.minTemp;
  if (p.bedMax < p.bedMin) p.bedMax = p.bedMin;
}

void findEditorIndicesFromProfile(const TagProfile& p) {
  editorTypeIndex = 0;
  editorSubtypeIndex = 0;
  editorColorIndex = 0;

  for (int i = 0; i < typeCount; i++) {
    if (strcmp(p.type, typeOptions[i]) == 0) {
      editorTypeIndex = i;
      break;
    }
  }

  for (int i = 0; i < subtypeCount; i++) {
    if (strcmp(p.subtype, subtypeOptions[i]) == 0) {
      editorSubtypeIndex = i;
      break;
    }
  }

  for (int i = 0; i < colorCount; i++) {
    if (strcmp(p.colorHex, colorHexes[i]) == 0) {
      editorColorIndex = i;
      break;
    }
  }
}

void rebuildEditorName() {
  snprintf(editorProfile.name, sizeof(editorProfile.name), "Custom %s %s",
           editorProfile.type, colorNames[editorColorIndex]);
}

void syncEditorProfile() {
  strncpy(editorProfile.brand, "Custom", sizeof(editorProfile.brand) - 1);
  editorProfile.brand[sizeof(editorProfile.brand) - 1] = '\0';

  strncpy(editorProfile.type, typeOptions[editorTypeIndex], sizeof(editorProfile.type) - 1);
  editorProfile.type[sizeof(editorProfile.type) - 1] = '\0';

  strncpy(editorProfile.subtype, subtypeOptions[editorSubtypeIndex], sizeof(editorProfile.subtype) - 1);
  editorProfile.subtype[sizeof(editorProfile.subtype) - 1] = '\0';

  strncpy(editorProfile.colorHex, colorHexes[editorColorIndex], sizeof(editorProfile.colorHex) - 1);
  editorProfile.colorHex[sizeof(editorProfile.colorHex) - 1] = '\0';

  strncpy(editorProfile.alpha, "FF", sizeof(editorProfile.alpha) - 1);
  editorProfile.alpha[sizeof(editorProfile.alpha) - 1] = '\0';

  normalizeProfile(editorProfile);
  rebuildEditorName();
}

void prepareEditorFromActiveProfile() {
  copyProfile(editorProfile, activeProfile);
  normalizeProfile(editorProfile);
  findEditorIndicesFromProfile(editorProfile);
  syncEditorProfile();
}

void prepareNewCustomFromActive() {
  editorCustomIndex = -1;
  prepareEditorFromActiveProfile();
}

void prepareEditExistingCustom(int idx) {
  if (idx < 0 || idx >= customProfileCount) return;

  editorCustomIndex = idx;
  copyProfile(editorProfile, customProfiles[idx]);
  normalizeProfile(editorProfile);
  findEditorIndicesFromProfile(editorProfile);
  syncEditorProfile();
}

// ============================================================
// Tiny JSON field extractors
// ============================================================
bool jsonGetStringValue(const char* json, const char* key, char* out, size_t outSize) {
  char pattern[40];
  snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

  const char* p = strstr(json, pattern);
  if (!p) return false;
  p += strlen(pattern);

  const char* end = strchr(p, '"');
  if (!end) return false;

  size_t len = end - p;
  if (len >= outSize) len = outSize - 1;

  memcpy(out, p, len);
  out[len] = '\0';

  return true;
}

bool jsonGetIntValue(const char* json, const char* key, int* outVal) {
  char pattern[40];
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);

  const char* p = strstr(json, pattern);
  if (!p) return false;
  p += strlen(pattern);

  *outVal = atoi(p);
  return true;
}

void buildProfileNameFromFields(TagProfile& p) {
  snprintf(p.name, sizeof(p.name), "%s %s %s",
           strlen(p.brand) ? p.brand : "Tag",
           strlen(p.type) ? p.type : "Unknown",
           strlen(p.subtype) ? p.subtype : "");
}

bool parseOpenSpoolJsonToProfile(const char* json, TagProfile& outProfile) {
  clearProfile(outProfile);

  bool gotAny = false;

  if (jsonGetStringValue(json, "brand", outProfile.brand, sizeof(outProfile.brand))) gotAny = true;
  if (jsonGetStringValue(json, "type", outProfile.type, sizeof(outProfile.type))) gotAny = true;
  if (jsonGetStringValue(json, "subtype", outProfile.subtype, sizeof(outProfile.subtype))) gotAny = true;
  if (jsonGetStringValue(json, "color_hex", outProfile.colorHex, sizeof(outProfile.colorHex))) gotAny = true;
  if (jsonGetStringValue(json, "alpha", outProfile.alpha, sizeof(outProfile.alpha))) gotAny = true;

  jsonGetIntValue(json, "min_temp", &outProfile.minTemp);
  jsonGetIntValue(json, "max_temp", &outProfile.maxTemp);
  jsonGetIntValue(json, "bed_min_temp", &outProfile.bedMin);
  jsonGetIntValue(json, "bed_max_temp", &outProfile.bedMax);
  jsonGetIntValue(json, "weight", &outProfile.weight);

  if (!gotAny) return false;

  normalizeProfile(outProfile);
  buildProfileNameFromFields(outProfile);

  return true;
}

// ============================================================
// OpenSpool JSON + NDEF
// ============================================================
bool buildOpenSpoolJson(const TagProfile& p, char* out, size_t outSize) {
  TagProfile safe;
  copyProfile(safe, p);
  normalizeProfile(safe);

  int written = snprintf(
    out, outSize,
    "{\"protocol\":\"openspool\","
    "\"version\":\"1.0\","
    "\"type\":\"%s\","
    "\"color_hex\":\"%s\","
    "\"brand\":\"%s\","
    "\"min_temp\":%d,"
    "\"max_temp\":%d,"
    "\"bed_min_temp\":%d,"
    "\"bed_max_temp\":%d,"
    "\"subtype\":\"%s\"}",
    safe.type,
    safe.colorHex,
    safe.brand,
    safe.minTemp,
    safe.maxTemp,
    safe.bedMin,
    safe.bedMax,
    safe.subtype
  );

  return written > 0 && (size_t)written < outSize;
}

bool buildNdefMimeJsonRecord(const char* json, uint8_t* out, size_t outCapacity, size_t* outLen) {
  const char* mimeType = "application/json";
  const uint8_t typeLen = 16;

  size_t payloadLen = strlen(json);
  if (payloadLen > 255) return false;

  size_t recordLen = 3 + typeLen + payloadLen;
  size_t tlvLen = 2 + recordLen + 1;

  if (tlvLen > outCapacity) return false;

  size_t i = 0;

  out[i++] = 0x03;
  out[i++] = (uint8_t)recordLen;

  out[i++] = 0xD2;
  out[i++] = typeLen;
  out[i++] = (uint8_t)payloadLen;

  memcpy(&out[i], mimeType, typeLen);
  i += typeLen;

  memcpy(&out[i], json, payloadLen);
  i += payloadLen;

  out[i++] = 0xFE;

  while ((i % 4) != 0 && i < outCapacity) {
    out[i++] = 0x00;
  }

  *outLen = i;
  return true;
}

// ============================================================
// NFC helpers
// ============================================================
bool readSinglePage4(uint8_t page, uint8_t* out4) {
  uint8_t buf[16];
  memset(buf, 0, sizeof(buf));

  if (!nfc.ntag2xx_ReadPage(page, buf)) return false;

  memcpy(out4, buf, 4);
  return true;
}

bool writeOnePageRobust(uint8_t page, const uint8_t* pageData) {
  uint8_t verifyData[4];

  for (int attempt = 0; attempt < 3; attempt++) {
    if (!nfc.ntag2xx_WritePage(page, (uint8_t*)pageData)) {
      Serial.print("Write failed at page ");
      Serial.print(page);
      Serial.print(" attempt ");
      Serial.println(attempt + 1);
      delay(10);
      continue;
    }

    delay(10);

    if (!readSinglePage4(page, verifyData)) {
      Serial.print("Readback failed at page ");
      Serial.print(page);
      Serial.print(" attempt ");
      Serial.println(attempt + 1);
      delay(10);
      continue;
    }

    if (memcmp(pageData, verifyData, 4) == 0) return true;

    Serial.print("Page verify mismatch at ");
    Serial.print(page);
    Serial.print(" attempt ");
    Serial.println(attempt + 1);

    Serial.print("Wrote: ");
    printHexLine(pageData, 4);

    Serial.print("Read : ");
    printHexLine(verifyData, 4);

    delay(10);
  }

  Serial.print("Giving up on page ");
  Serial.println(page);

  return false;
}

bool writeBufferToTagPages(const uint8_t* data, size_t len) {
  size_t pageCount = (len + 3) / 4;
  uint8_t pageData[4];

  for (size_t pageOffset = 0; pageOffset < pageCount; pageOffset++) {
    memset(pageData, 0x00, sizeof(pageData));

    size_t srcOffset = pageOffset * 4;
    size_t copyLen = min((size_t)4, len - srcOffset);

    memcpy(pageData, &data[srcOffset], copyLen);

    uint8_t page = 4 + pageOffset;

    if (!writeOnePageRobust(page, pageData)) return false;
  }

  for (size_t extra = 0; extra < 4; extra++) {
    uint8_t page = 4 + pageCount + extra;
    uint8_t blank[4] = {0x00, 0x00, 0x00, 0x00};

    if (!writeOnePageRobust(page, blank)) return false;
  }

  return true;
}

bool readBackPages(uint8_t* out, size_t len) {
  size_t pageCount = (len + 3) / 4;
  uint8_t pageData[4];

  for (size_t pageOffset = 0; pageOffset < pageCount; pageOffset++) {
    uint8_t page = 4 + pageOffset;

    if (!readSinglePage4(page, pageData)) {
      Serial.print("Readback failed at page ");
      Serial.println(page);
      return false;
    }

    size_t dstOffset = pageOffset * 4;
    size_t copyLen = min((size_t)4, len - dstOffset);

    memcpy(&out[dstOffset], pageData, copyLen);
  }

  return true;
}

bool verifyCurrentPayload() {
  uint8_t readBuf[384];
  memset(readBuf, 0, sizeof(readBuf));

  if (!readBackPages(readBuf, ndefLength)) {
    Serial.println("verifyCurrentPayload: readBackPages failed");
    return false;
  }

  for (size_t i = 0; i < ndefLength; i++) {
    if (readBuf[i] != ndefBuffer[i]) {
      Serial.print("Mismatch at byte ");
      Serial.print(i);
      Serial.print(": wrote ");

      if (ndefBuffer[i] < 0x10) Serial.print('0');
      Serial.print(ndefBuffer[i], HEX);

      Serial.print(" read ");

      if (readBuf[i] < 0x10) Serial.print('0');
      Serial.println(readBuf[i], HEX);

      return false;
    }
  }

  Serial.println("Verify OK: payload matches");
  return true;
}

// ============================================================
// Read tag helpers - NDEF + RAW fallback
// ============================================================
bool readLikelyNdefFromTag(char* outJson, size_t outJsonSize) {
  uint8_t raw[384];
  memset(raw, 0, sizeof(raw));

  const uint8_t startPage = 4;
  const int maxPagesToRead = 80;

  for (int i = 0; i < maxPagesToRead; i++) {
    uint8_t pageData[4];

    if (!readSinglePage4(startPage + i, pageData)) {
      Serial.print("Read fail page ");
      Serial.println(startPage + i);
      return false;
    }

    memcpy(&raw[i * 4], pageData, 4);
  }

  Serial.println("Raw tag bytes:");
  printHexLine(raw, 64);

  if (raw[0] != 0x03) {
    Serial.println("No TLV 0x03 at page 4");
    return false;
  }

  int tlvLen = raw[1];

  if (tlvLen <= 0 || tlvLen > 250) {
    Serial.println("Bad TLV length");
    return false;
  }

  uint8_t hdr = raw[2];

  if (hdr != 0xD2 && hdr != 0xC2) {
    Serial.print("Unexpected NDEF header: ");
    Serial.println(hdr, HEX);
  }

  uint8_t typeLen = raw[3];
  uint8_t payloadLen = raw[4];

  if (typeLen == 0 || payloadLen == 0) {
    Serial.println("Empty type/payload");
    return false;
  }

  int typePos = 5;
  int payloadPos = typePos + typeLen;

  if (payloadPos + payloadLen > (int)sizeof(raw)) {
    Serial.println("Payload outside buffer");
    return false;
  }

  char mime[32];
  memset(mime, 0, sizeof(mime));

  int mimeCopy = min((int)sizeof(mime) - 1, (int)typeLen);
  memcpy(mime, &raw[typePos], mimeCopy);
  mime[mimeCopy] = '\0';

  Serial.print("MIME: ");
  Serial.println(mime);

  if (strcmp(mime, "application/json") != 0) {
    Serial.println("Not application/json");
    return false;
  }

  int copyLen = min((int)outJsonSize - 1, (int)payloadLen);
  memcpy(outJson, &raw[payloadPos], copyLen);
  outJson[copyLen] = '\0';

  Serial.println("JSON read from tag:");
  Serial.println(outJson);

  return true;
}

bool readRawFallbackProfile(TagProfile& outProfile) {
  uint8_t raw[384];
  memset(raw, 0, sizeof(raw));

  const uint8_t startPage = 4;
  const int maxPagesToRead = 80;

  for (int i = 0; i < maxPagesToRead; i++) {
    uint8_t pageData[4];
    if (!readSinglePage4(startPage + i, pageData)) {
      Serial.print("RAW fallback read fail page ");
      Serial.println(startPage + i);
      return false;
    }
    memcpy(&raw[i * 4], pageData, 4);
  }

  Serial.println("RAW fallback bytes:");
  printHexLine(raw, 128);

  clearProfile(outProfile);

  // eSUN / U1-style fixed layout observed from working dump:
  // Page 05: brand, e.g. eSUN
  // Page 0A-0C: subtype, e.g. Lightweight
  // Page 0F: type, e.g. PLA
  memcpy(outProfile.brand, &raw[(0x05 - 4) * 4], 4);
  outProfile.brand[4] = '\0';
  cleanCString(outProfile.brand);

  memcpy(outProfile.subtype, &raw[(0x0A - 4) * 4], 12);
  outProfile.subtype[12] = '\0';
  cleanCString(outProfile.subtype);

  memcpy(outProfile.type, &raw[(0x0F - 4) * 4], 4);
  outProfile.type[4] = '\0';
  cleanCString(outProfile.type);

  // Try to locate JSON or JSON fragment anywhere in raw memory.
  // If a complete-ish JSON exists, this will fill temps/color/etc.
  char rawText[385];
  memset(rawText, 0, sizeof(rawText));
  memcpy(rawText, raw, 384);

  char* jsonStart = strchr(rawText, '{');
  if (jsonStart) {
    TagProfile parsed;
    clearProfile(parsed);
    if (parseOpenSpoolJsonToProfile(jsonStart, parsed)) {
      if (strlen(parsed.brand)) strncpy(outProfile.brand, parsed.brand, sizeof(outProfile.brand) - 1);
      if (strlen(parsed.type)) strncpy(outProfile.type, parsed.type, sizeof(outProfile.type) - 1);
      if (strlen(parsed.subtype)) strncpy(outProfile.subtype, parsed.subtype, sizeof(outProfile.subtype) - 1);
      if (strlen(parsed.colorHex)) strncpy(outProfile.colorHex, parsed.colorHex, sizeof(outProfile.colorHex) - 1);
      if (strlen(parsed.alpha)) strncpy(outProfile.alpha, parsed.alpha, sizeof(outProfile.alpha) - 1);
      outProfile.minTemp = parsed.minTemp;
      outProfile.maxTemp = parsed.maxTemp;
      outProfile.bedMin = parsed.bedMin;
      outProfile.bedMax = parsed.bedMax;
      outProfile.weight = parsed.weight;
    }
  } else {
    // Scan partial fragment for known fields.
    // This handles dumps where page 4 is not a complete JSON start.
    jsonGetStringValue(rawText, "brand", outProfile.brand, sizeof(outProfile.brand));
    jsonGetStringValue(rawText, "type", outProfile.type, sizeof(outProfile.type));
    jsonGetStringValue(rawText, "subtype", outProfile.subtype, sizeof(outProfile.subtype));
    jsonGetStringValue(rawText, "color_hex", outProfile.colorHex, sizeof(outProfile.colorHex));
    jsonGetIntValue(rawText, "min_temp", &outProfile.minTemp);
    jsonGetIntValue(rawText, "max_temp", &outProfile.maxTemp);
    jsonGetIntValue(rawText, "bed_min_temp", &outProfile.bedMin);
    jsonGetIntValue(rawText, "bed_max_temp", &outProfile.bedMax);
  }

  normalizeProfile(outProfile);
  buildProfileNameFromFields(outProfile);

  Serial.println("RAW fallback profile:");
  Serial.print("Brand: "); Serial.println(outProfile.brand);
  Serial.print("Type: "); Serial.println(outProfile.type);
  Serial.print("Subtype: "); Serial.println(outProfile.subtype);
  Serial.print("Color: "); Serial.println(outProfile.colorHex);
  Serial.print("Nozzle: "); Serial.print(outProfile.minTemp); Serial.print("-"); Serial.println(outProfile.maxTemp);
  Serial.print("Bed: "); Serial.print(outProfile.bedMin); Serial.print("-"); Serial.println(outProfile.bedMax);

  return strlen(outProfile.brand) > 0 || strlen(outProfile.type) > 0 || strlen(outProfile.subtype) > 0;
}

bool readProfileFromTag(TagProfile& outProfile) {
  if (readLikelyNdefFromTag(readJsonBuffer, sizeof(readJsonBuffer))) {
    if (parseOpenSpoolJsonToProfile(readJsonBuffer, outProfile)) {
      Serial.println("NDEF OpenSpool read success");
      return true;
    }
  }

  Serial.println("NDEF failed -> trying RAW fallback");

  if (readRawFallbackProfile(outProfile)) {
    Serial.println("RAW fallback success");
    return true;
  }

  Serial.println("All read methods failed");
  return false;
}

bool prepareActiveProfilePayload() {
  normalizeProfile(activeProfile);

  if (!buildOpenSpoolJson(activeProfile, jsonBuffer, sizeof(jsonBuffer))) {
    setStatus("JSON build fail");
    setResult("Profile too large", "", "");
    currentScreen = SCREEN_ACTION_RESULT;
    redrawFull = true;
    return false;
  }

  if (!buildNdefMimeJsonRecord(jsonBuffer, ndefBuffer, sizeof(ndefBuffer), &ndefLength)) {
    setStatus("NDEF build fail");
    setResult("Payload too large", "Use NTAG215/216", "");
    currentScreen = SCREEN_ACTION_RESULT;
    redrawFull = true;
    return false;
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("PROFILE PAYLOAD");
  Serial.println("========================================");

  Serial.print("Profile: ");
  Serial.println(activeProfile.name);

  Serial.print("Source: ");
  Serial.println(sourceToString(activeProfileSource));

  Serial.print("JSON: ");
  Serial.println(jsonBuffer);

  Serial.print("NDEF length: ");
  Serial.println(ndefLength);

  return true;
}

// ============================================================
// UI drawing
// ============================================================
void drawFrame(const char* title, const char* bottomHint) {
  tft.fillScreen(COL_BG);
  tft.drawRect(0, 0, tft.width(), tft.height(), COL_FRAME);

  tft.fillRect(1, 1, tft.width() - 2, TOPBAR_H, COL_TOPBAR);
  drawCenteredText(title, 8, 2, COL_TITLE);

  tft.drawFastHLine(8, tft.height() - 18, tft.width() - 16, COL_DIM);

  tft.setTextSize(1);
  tft.setTextColor(COL_DIM);
  tft.setCursor(10, tft.height() - 10);
  tft.print(bottomHint);
}

void clearMenuArea() {
  tft.fillRect(6, MENU_Y - 6, tft.width() - 12, VISIBLE_ROWS * MENU_ROW_H + 12, COL_BG);
}

void drawMenuLine(int y, bool selected, const char* text, bool smallText = false) {
  if (selected) {
    tft.fillRoundRect(10, y - 3, tft.width() - 20, 24, 5, COL_PANEL);
    tft.drawRoundRect(10, y - 3, tft.width() - 20, 24, 5, COL_HILITE);
    tft.setTextColor(COL_HILITE);
  } else {
    tft.setTextColor(COL_TEXT);
  }

  tft.setTextSize(smallText ? 1 : 2);
  tft.setCursor(14, smallText ? y + 4 : y);
  tft.print(text);
}

void drawProfileMenuLine(int y, bool selected, const char* text, const char* colorHex) {
  if (selected) {
    tft.fillRoundRect(10, y - 3, tft.width() - 20, 24, 5, COL_PANEL);
    tft.drawRoundRect(10, y - 3, tft.width() - 20, 24, 5, COL_HILITE);
    tft.setTextColor(COL_HILITE);
  } else {
    tft.setTextColor(COL_TEXT);
  }

  uint16_t c = hexTo565(colorHex);
  tft.fillRect(14, y + 4, 14, 14, c);
  tft.drawRect(14, y + 4, 14, 14, COL_DIM);

  tft.setTextSize(2);
  tft.setCursor(36, y);
  tft.print(text);
}

void drawFooterProfileName(const char* label, const char* value, uint16_t valueColor) {
  tft.fillRect(10, FOOTER_Y, tft.width() - 20, 42, COL_BG);
  tft.drawFastHLine(12, FOOTER_Y, tft.width() - 24, COL_DIM);

  tft.setTextSize(1);
  tft.setTextColor(COL_DIM);
  tft.setCursor(12, FOOTER_Y + 12);
  tft.print(label);

  tft.setTextSize(2);
  tft.setTextColor(valueColor);
  tft.setCursor(12, FOOTER_Y + 28);
  tft.print(value);
}

void drawHomeStatic() {
  drawFrame("OpenSpool Writer", "ENC=move  K0=select  K1=back  hold=home");
}

void drawHomeMenu() {
  clearMenuArea();

  static const char* homeItems[] = {
    "Choose profile",
    "View profile",
    "Custom editor",
    "Read tag",
    "Write tag",
    "Verify tag",
    "Clone tag"
  };

  for (int i = 0; i < VISIBLE_ROWS; i++) {
    int idx = topIndex + i;
    if (idx >= homeItemCount()) break;

    int y = MENU_Y + i * MENU_ROW_H;
    drawMenuLine(y, idx == selectedIndex, homeItems[idx], false);
  }
}

void drawHomeFooter() {
  drawFooterProfileName("Active profile:", activeProfile.name, sourceColor(activeProfileSource));
}

void drawProfileSourceStatic() {
  drawFrame("Choose Profile", "ENC=move  K0=select  K1=back");
}

void drawProfileSourceMenu() {
  clearMenuArea();

  static const char* items[] = {"Generic library", "Custom library", "< Back"};

  for (int i = 0; i < 3; i++) {
    int y = MENU_Y + i * MENU_ROW_H;
    drawMenuLine(y, i == selectedIndex, items[i], false);
  }
}

void drawGenericListStatic() {
  drawFrame("Generic Profiles", "ENC=move  K0=select  K1=back");
}

void drawGenericListMenu() {
  clearMenuArea();

  for (int i = 0; i < VISIBLE_ROWS; i++) {
    int idx = topIndex + i;
    if (idx >= genericListCount()) break;

    int y = MENU_Y + i * MENU_ROW_H;

    if (idx == 0) {
      drawMenuLine(y, idx == selectedIndex, "< Back", false);
    } else {
      drawProfileMenuLine(
        y,
        idx == selectedIndex,
        genericProfiles[idx - 1].name,
        genericProfiles[idx - 1].colorHex
      );
    }
  }
}

void drawCustomListStatic() {
  drawFrame("Custom Profiles", "ENC=move  K0=select  K1=back");
}

void drawCustomListMenu() {
  clearMenuArea();

  for (int i = 0; i < VISIBLE_ROWS; i++) {
    int idx = topIndex + i;
    if (idx >= customListCount()) break;

    int y = MENU_Y + i * MENU_ROW_H;

    if (idx == 0) {
      drawMenuLine(y, idx == selectedIndex, "+ New custom", false);
    } else if (idx == customProfileCount + 1) {
      drawMenuLine(y, idx == selectedIndex, "< Back", false);
    } else {
      drawProfileMenuLine(
        y,
        idx == selectedIndex,
        customProfiles[idx - 1].name,
        customProfiles[idx - 1].colorHex
      );
    }
  }
}

void drawCustomActionsStatic() {
  drawFrame("Custom Profile", "ENC=move  K0=select  K1=back");
}

void drawCustomActionsMenu() {
  clearMenuArea();

  static const char* items[] = {
    "Activate",
    "Edit",
    "Delete",
    "< Back"
  };

  for (int i = 0; i < 4; i++) {
    int y = MENU_Y + i * MENU_ROW_H;
    drawMenuLine(y, i == selectedIndex, items[i], false);
  }
}

void drawCustomActionsFooter() {
  if (selectedCustomIndex >= 0 && selectedCustomIndex < customProfileCount) {
    drawFooterProfileName("Selected custom:", customProfiles[selectedCustomIndex].name, COL_WARN);
  }
}

void drawProfileViewFull() {
  drawFrame("Profile Detail", "K0=home  K1=back");

  tft.setTextSize(2);
  tft.setTextColor(sourceColor(activeProfileSource));
  tft.setCursor(12, 44);
  tft.print(activeProfile.name);

  tft.drawFastHLine(12, 68, tft.width() - 24, COL_DIM);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT);

  tft.setCursor(12, 84);
  tft.print("Source: ");
  tft.print(sourceToString(activeProfileSource));

  tft.setCursor(12, 100);
  tft.print("Brand: ");
  tft.print(activeProfile.brand);

  tft.setCursor(12, 116);
  tft.print("Type: ");
  tft.print(activeProfile.type);

  tft.setCursor(12, 132);
  tft.print("Subtype: ");
  tft.print(activeProfile.subtype);

  tft.setCursor(12, 148);
  tft.print("Color: ");
  tft.print(activeProfile.colorHex);

  uint16_t c = hexTo565(activeProfile.colorHex);
  tft.fillRect(120, 144, 24, 14, c);
  tft.drawRect(120, 144, 24, 14, COL_DIM);

  tft.setCursor(12, 164);
  tft.print("Nozzle: ");
  tft.print(activeProfile.minTemp);
  tft.print("-");
  tft.print(activeProfile.maxTemp);
  tft.print(" C");

  tft.setCursor(12, 180);
  tft.print("Bed: ");
  tft.print(activeProfile.bedMin);
  tft.print("-");
  tft.print(activeProfile.bedMax);
  tft.print(" C");

  tft.setCursor(12, 196);
  tft.print("Weight: ");
  tft.print(activeProfile.weight);
  tft.print(" g");
}

void drawCustomEditorStatic() {
  drawFrame("Custom Editor", "ENC=move  K0=change  K1=back");
}

void drawCustomEditorMenu() {
  clearMenuArea();

  char line[64];

  for (int i = 0; i < VISIBLE_ROWS; i++) {
    int idx = topIndex + i;
    if (idx >= customEditorCount()) break;

    int y = MENU_Y + i * MENU_ROW_H;

    switch (idx) {
      case 0: snprintf(line, sizeof(line), "Type: %s", typeOptions[editorTypeIndex]); break;
      case 1: snprintf(line, sizeof(line), "Subtype: %s", subtypeOptions[editorSubtypeIndex]); break;
      case 2: snprintf(line, sizeof(line), "Color: %s", colorNames[editorColorIndex]); break;
      case 3: snprintf(line, sizeof(line), "Nozzle min: %d", editorProfile.minTemp); break;
      case 4: snprintf(line, sizeof(line), "Nozzle max: %d", editorProfile.maxTemp); break;
      case 5: snprintf(line, sizeof(line), "Bed temp: %d", editorProfile.bedMin); break;
      case 6: snprintf(line, sizeof(line), "Weight: %d", editorProfile.weight); break;
      case 7: snprintf(line, sizeof(line), "%s custom", (editorCustomIndex >= 0) ? "Update" : "Save new"); break;
      case 8: snprintf(line, sizeof(line), "%s", (editorCustomIndex >= 0) ? "Delete custom" : "Reset draft"); break;
      case 9: snprintf(line, sizeof(line), "< Back"); break;
      default: line[0] = 0; break;
    }

    drawMenuLine(y, idx == selectedIndex, line, true);
  }
}

void drawCustomEditorFooter() {
  drawFooterProfileName("Editor profile:", editorProfile.name, COL_WARN);
}

void drawActionStatic() {
  drawFrame("Tag action", "K0=home  K1=back");
}

void drawActionBody() {
  tft.fillRect(10, 42, tft.width() - 20, 120, COL_BG);

  tft.setTextSize(2);
  tft.setTextColor(COL_WARN);
  tft.setCursor(12, 46);
  tft.print("Status:");

  tft.setTextColor(COL_OK);
  tft.setCursor(12, 70);
  tft.print(statusLine);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT);

  tft.setCursor(12, 108);
  tft.print(resultLine1);

  tft.setCursor(12, 124);
  tft.print(resultLine2);

  tft.setCursor(12, 140);
  tft.print(resultLine3);
}

void drawActionFooter() {
  drawFooterProfileName("Last UID:", lastUidStr, COL_TEXT);
}

void drawCurrentScreenFull() {
  switch (currentScreen) {
    case SCREEN_HOME:
      drawHomeStatic();
      drawHomeMenu();
      drawHomeFooter();
      break;

    case SCREEN_PROFILE_SOURCE:
      drawProfileSourceStatic();
      drawProfileSourceMenu();
      break;

    case SCREEN_GENERIC_LIST:
      drawGenericListStatic();
      drawGenericListMenu();
      break;

    case SCREEN_CUSTOM_LIST:
      drawCustomListStatic();
      drawCustomListMenu();
      break;

    case SCREEN_CUSTOM_ACTIONS:
      drawCustomActionsStatic();
      drawCustomActionsMenu();
      drawCustomActionsFooter();
      break;

    case SCREEN_PROFILE_VIEW:
      drawProfileViewFull();
      break;

    case SCREEN_CUSTOM_EDITOR:
      drawCustomEditorStatic();
      drawCustomEditorMenu();
      drawCustomEditorFooter();
      break;

    case SCREEN_ACTION_RESULT:
      drawActionStatic();
      drawActionBody();
      drawActionFooter();
      break;
  }

  redrawFull = false;
  redrawMenu = false;
  redrawFooter = false;
  redrawBody = false;
}

void redrawCurrentParts() {
  if (redrawFull) {
    drawCurrentScreenFull();
    return;
  }

  if (redrawMenu) {
    switch (currentScreen) {
      case SCREEN_HOME: drawHomeMenu(); break;
      case SCREEN_PROFILE_SOURCE: drawProfileSourceMenu(); break;
      case SCREEN_GENERIC_LIST: drawGenericListMenu(); break;
      case SCREEN_CUSTOM_LIST: drawCustomListMenu(); break;
      case SCREEN_CUSTOM_ACTIONS: drawCustomActionsMenu(); break;
      case SCREEN_CUSTOM_EDITOR: drawCustomEditorMenu(); break;
      default: break;
    }

    redrawMenu = false;
  }

  if (redrawFooter) {
    switch (currentScreen) {
      case SCREEN_HOME: drawHomeFooter(); break;
      case SCREEN_CUSTOM_ACTIONS: drawCustomActionsFooter(); break;
      case SCREEN_CUSTOM_EDITOR: drawCustomEditorFooter(); break;
      case SCREEN_ACTION_RESULT: drawActionFooter(); break;
      default: break;
    }

    redrawFooter = false;
  }

  if (redrawBody) {
    if (currentScreen == SCREEN_ACTION_RESULT) {
      drawActionBody();
      drawActionFooter();
    }

    redrawBody = false;
  }
}

// ============================================================
// Navigation
// ============================================================
void openScreen(ScreenState s) {
  currentScreen = s;
  resetMenu();
  redrawFull = true;
}

void goBackOneLevel() {
  switch (currentScreen) {
    case SCREEN_HOME:
      break;

    case SCREEN_PROFILE_SOURCE:
    case SCREEN_PROFILE_VIEW:
    case SCREEN_ACTION_RESULT:
      openScreen(SCREEN_HOME);
      break;

    case SCREEN_GENERIC_LIST:
    case SCREEN_CUSTOM_LIST:
      openScreen(SCREEN_PROFILE_SOURCE);
      break;

    case SCREEN_CUSTOM_ACTIONS:
      openScreen(SCREEN_CUSTOM_LIST);
      break;

    case SCREEN_CUSTOM_EDITOR:
      openScreen(SCREEN_HOME);
      break;
  }
}

// ============================================================
// Actions
// ============================================================
void startActionScreen(const char* s1, const char* s2, const char* s3) {
  currentScreen = SCREEN_ACTION_RESULT;

  strncpy(resultLine1, s1, sizeof(resultLine1) - 1);
  resultLine1[sizeof(resultLine1) - 1] = '\0';

  strncpy(resultLine2, s2, sizeof(resultLine2) - 1);
  resultLine2[sizeof(resultLine2) - 1] = '\0';

  strncpy(resultLine3, s3, sizeof(resultLine3) - 1);
  resultLine3[sizeof(resultLine3) - 1] = '\0';

  redrawFull = true;
}

void applyEditorToActiveProfile() {
  copyProfile(activeProfile, editorProfile);
  normalizeProfile(activeProfile);
  activeProfileSource = SRC_CUSTOM;
}

void saveEditorProfile() {
  syncEditorProfile();

  if (editorCustomIndex >= 0) {
    if (updateCustomProfile(editorCustomIndex, editorProfile)) {
      applyEditorToActiveProfile();
      setStatus("Custom updated");
      setResult("Updated in flash", editorProfile.name, "");
      openScreen(SCREEN_ACTION_RESULT);
    } else {
      setStatus("Update failed");
      setResult("Bad custom index", "", "");
      openScreen(SCREEN_ACTION_RESULT);
    }
  } else {
    if (addCustomProfile(editorProfile)) {
      editorCustomIndex = customProfileCount - 1;
      applyEditorToActiveProfile();
      setStatus("Custom saved");
      setResult("Saved to flash", editorProfile.name, "");
      openScreen(SCREEN_ACTION_RESULT);
    } else {
      setStatus("Save failed");
      setResult("Custom storage full", "Delete one first", "");
      openScreen(SCREEN_ACTION_RESULT);
    }
  }
}

void resetEditorDraftFromActive() {
  prepareNewCustomFromActive();
  redrawMenu = true;
  redrawFooter = true;
}

void deleteEditorProfile() {
  if (editorCustomIndex < 0) {
    resetEditorDraftFromActive();
    setStatus("Draft reset");
    setResult("Unsaved draft reset", "", "");
    openScreen(SCREEN_ACTION_RESULT);
    return;
  }

  int deletedIndex = editorCustomIndex;

  if (!deleteCustomProfile(deletedIndex)) {
    setStatus("Delete failed");
    setResult("Custom not deleted", "", "");
    openScreen(SCREEN_ACTION_RESULT);
    return;
  }

  if (activeProfileIsCustom()) {
    if (deletedIndex < customProfileCount) {
      activateCustomProfile(deletedIndex);
    } else if (customProfileCount > 0) {
      activateCustomProfile(customProfileCount - 1);
    } else {
      activateGenericProfile(0);
    }
  }

  editorCustomIndex = -1;

  setStatus("Custom deleted");
  setResult("Removed from flash", "", "");
  openScreen(SCREEN_ACTION_RESULT);
}

bool waitForSupportedTagOrFail(uint8_t* uid, uint8_t* uidLength, unsigned long timeoutMs) {
  if (!waitForTag(uid, uidLength, timeoutMs)) {
    setStatus("No tag found");
    setResult("Timeout after 15 sec", "Try again", "");
    return false;
  }

  formatUidToString(uid, *uidLength, lastUidStr, sizeof(lastUidStr));
  redrawFooter = true;

  if (rejectUnsupportedUidIfNeeded(*uidLength)) {
    return false;
  }

  return true;
}

void readTagToActiveProfile() {
  if (!nfcReady) {
    setStatus("PN532 not ready");
    startActionScreen("Check PN532 wiring", "", "");
    return;
  }

  currentScreen = SCREEN_ACTION_RESULT;
  redrawFull = true;

  setStatus("Waiting for tag...");
  setResult("Hold OpenSpool tag still", "near PN532 antenna", "");
  redrawCurrentParts();

  uint8_t uid[10] = {0};
  uint8_t uidLength = 0;

  if (!waitForSupportedTagOrFail(uid, &uidLength, 15000)) {
    return;
  }

  TagProfile parsed;
  clearProfile(parsed);

  setStatus("Reading...");
  setResult("Trying NDEF / RAW", lastUidStr, "");
  redrawCurrentParts();

  if (!readProfileFromTag(parsed)) {
    setStatus("READ FAIL");
    setResult("No valid tag profile", lastUidStr, "Unknown / locked tag");
    return;
  }

  normalizeProfile(parsed);

  copyProfile(activeProfile, parsed);
  activeProfileSource = SRC_TAG_READ;

  setStatus("READ OK");
  setResult(parsed.type, parsed.subtype, parsed.colorHex);

  openScreen(SCREEN_PROFILE_VIEW);
}

void writeSelectedProfileToTag() {
  if (!nfcReady) {
    setStatus("PN532 not ready");
    startActionScreen("Check PN532 wiring", "", "");
    return;
  }

  if (!prepareActiveProfilePayload()) return;

  currentScreen = SCREEN_ACTION_RESULT;
  redrawFull = true;

  setStatus("Waiting for tag...");
  setResult("Hold NTAG215/216 still", "near PN532 antenna", "");
  redrawCurrentParts();

  uint8_t uid[10] = {0};
  uint8_t uidLength = 0;

  if (!waitForSupportedTagOrFail(uid, &uidLength, 15000)) {
    return;
  }

  Serial.print("UID: ");
  Serial.println(lastUidStr);

  const int maxJobRetries = 3;
  bool success = false;
  bool verifyOk = false;

  for (int jobAttempt = 0; jobAttempt < maxJobRetries; jobAttempt++) {
    Serial.println();
    Serial.print("=== Full write attempt ");
    Serial.print(jobAttempt + 1);
    Serial.print(" / ");
    Serial.print(maxJobRetries);
    Serial.println(" ===");

    char line2[32];
    snprintf(line2, sizeof(line2), "Attempt %d/%d", jobAttempt + 1, maxJobRetries);

    setStatus("Writing...");
    setResult("Writing OpenSpool data", line2, lastUidStr);
    redrawCurrentParts();

    bool writeOk = writeBufferToTagPages(ndefBuffer, ndefLength);

    if (!writeOk) {
      Serial.println("Full write attempt failed");
      delay(30);
      continue;
    }

    setStatus("Verifying...");
    setResult("Write OK, verifying", line2, lastUidStr);
    redrawCurrentParts();

    verifyOk = verifyCurrentPayload();

    if (verifyOk) {
      success = true;
      break;
    }

    Serial.println("Verify failed after write attempt");
    delay(30);
  }

  if (success && verifyOk) {
    setStatus("WRITE+VERIFY PASS");
    setResult("OpenSpool tag ready", lastUidStr, "Auto-verify passed");
  } else {
    setStatus("WRITE/VERIFY FAIL");
    setResult("All retries used", lastUidStr, "Try hold tag steadier");
  }
}

void verifySelectedProfileOnTag() {
  if (!nfcReady) {
    setStatus("PN532 not ready");
    startActionScreen("Check PN532 wiring", "", "");
    return;
  }

  if (!prepareActiveProfilePayload()) return;

  Serial.println();
  Serial.println("========================================");
  Serial.println("VERIFY OPENSPOOL TAG");
  Serial.println("========================================");

  currentScreen = SCREEN_ACTION_RESULT;
  redrawFull = true;

  setStatus("Waiting for tag...");
  setResult("Hold written tag still", "near PN532 antenna", "");
  redrawCurrentParts();

  uint8_t uid[10] = {0};
  uint8_t uidLength = 0;

  if (!waitForSupportedTagOrFail(uid, &uidLength, 15000)) {
    return;
  }

  Serial.print("UID: ");
  Serial.println(lastUidStr);

  bool ok = verifyCurrentPayload();

  setStatus(ok ? "VERIFY PASS" : "VERIFY FAIL");
  setResult(ok ? "Tag matches active profile" : "Payload mismatch",
            lastUidStr,
            ok ? "" : "Wrong tag/profile/format");
}

void cloneTagReadNormalizeWrite() {
  if (!nfcReady) {
    setStatus("PN532 not ready");
    startActionScreen("Check PN532 wiring", "", "");
    return;
  }

  currentScreen = SCREEN_ACTION_RESULT;
  redrawFull = true;

  setStatus("Clone source");
  setResult("Hold source tag", "Read -> normalize", "");
  redrawCurrentParts();

  uint8_t uid[10] = {0};
  uint8_t uidLength = 0;

  if (!waitForSupportedTagOrFail(uid, &uidLength, 15000)) {
    return;
  }

  TagProfile cloned;
  clearProfile(cloned);

  setStatus("Reading source...");
  setResult("Trying NDEF / RAW", lastUidStr, "");
  redrawCurrentParts();

  if (!readProfileFromTag(cloned)) {
    setStatus("CLONE READ FAIL");
    setResult("No valid tag profile", lastUidStr, "Source not cloneable");
    return;
  }

  normalizeProfile(cloned);
  copyProfile(activeProfile, cloned);
  activeProfileSource = SRC_TAG_READ;

  setStatus("Source OK");
  setResult("Remove source tag", "Then hold blank NTAG", "Writing in 2 sec");
  redrawCurrentParts();

  delay(2000);

  writeSelectedProfileToTag();
}

// ============================================================
// Custom editor and select handler
// ============================================================
void handleCustomEditorPress() {
  switch (selectedIndex) {
    case 0:
      editorTypeIndex = (editorTypeIndex + 1) % typeCount;
      break;

    case 1:
      editorSubtypeIndex = (editorSubtypeIndex + 1) % subtypeCount;
      break;

    case 2:
      editorColorIndex = (editorColorIndex + 1) % colorCount;
      break;

    case 3:
      editorProfile.minTemp += 5;
      if (editorProfile.minTemp > 280) editorProfile.minTemp = 180;
      break;

    case 4:
      editorProfile.maxTemp += 5;
      if (editorProfile.maxTemp > 320) editorProfile.maxTemp = 200;
      break;

    case 5:
      editorProfile.bedMin += 5;
      if (editorProfile.bedMin > 120) editorProfile.bedMin = 0;
      editorProfile.bedMax = editorProfile.bedMin;
      break;

    case 6:
      editorProfile.weight += 250;
      if (editorProfile.weight > 3000) editorProfile.weight = 250;
      break;

    case 7:
      saveEditorProfile();
      return;

    case 8:
      deleteEditorProfile();
      return;

    case 9:
      openScreen(SCREEN_HOME);
      return;
  }

  syncEditorProfile();

  redrawMenu = true;
  redrawFooter = true;
}

void handleSelect() {
  switch (currentScreen) {
    case SCREEN_HOME:
      switch (selectedIndex) {
        case 0:
          openScreen(SCREEN_PROFILE_SOURCE);
          break;

        case 1:
          openScreen(SCREEN_PROFILE_VIEW);
          break;

        case 2:
          if (activeProfileIsCustom()) {
            int found = -1;

            for (int i = 0; i < customProfileCount; i++) {
              if (strcmp(activeProfile.name, customProfiles[i].name) == 0 &&
                  strcmp(activeProfile.type, customProfiles[i].type) == 0 &&
                  strcmp(activeProfile.colorHex, customProfiles[i].colorHex) == 0) {
                found = i;
                break;
              }
            }

            if (found >= 0) {
              prepareEditExistingCustom(found);
            } else {
              prepareNewCustomFromActive();
            }
          } else {
            prepareNewCustomFromActive();
          }

          openScreen(SCREEN_CUSTOM_EDITOR);
          break;

        case 3:
          readTagToActiveProfile();
          break;

        case 4:
          writeSelectedProfileToTag();
          break;

        case 5:
          verifySelectedProfileOnTag();
          break;

        case 6:
          cloneTagReadNormalizeWrite();
          break;
      }
      break;

    case SCREEN_PROFILE_SOURCE:
      switch (selectedIndex) {
        case 0:
          openScreen(SCREEN_GENERIC_LIST);
          break;

        case 1:
          openScreen(SCREEN_CUSTOM_LIST);
          break;

        case 2:
          openScreen(SCREEN_HOME);
          break;
      }
      break;

    case SCREEN_GENERIC_LIST:
      if (selectedIndex == 0) {
        openScreen(SCREEN_PROFILE_SOURCE);
      } else {
        activateGenericProfile(selectedIndex - 1);
        openScreen(SCREEN_HOME);
      }
      break;

    case SCREEN_CUSTOM_LIST:
      if (selectedIndex == 0) {
        prepareNewCustomFromActive();
        openScreen(SCREEN_CUSTOM_EDITOR);
      } else if (selectedIndex == customProfileCount + 1) {
        openScreen(SCREEN_PROFILE_SOURCE);
      } else {
        selectedCustomIndex = selectedIndex - 1;
        openScreen(SCREEN_CUSTOM_ACTIONS);
      }
      break;

    case SCREEN_CUSTOM_ACTIONS:
      switch (selectedIndex) {
        case 0:
          activateCustomProfile(selectedCustomIndex);
          openScreen(SCREEN_HOME);
          break;

        case 1:
          prepareEditExistingCustom(selectedCustomIndex);
          openScreen(SCREEN_CUSTOM_EDITOR);
          break;

        case 2:
          if (deleteCustomProfile(selectedCustomIndex)) {
            if (activeProfileIsCustom()) {
              if (customProfileCount > 0) {
                activateCustomProfile(min(selectedCustomIndex, customProfileCount - 1));
              } else {
                activateGenericProfile(0);
              }
            }

            setStatus("Custom deleted");
            setResult("Removed from flash", "", "");
            openScreen(SCREEN_ACTION_RESULT);
          } else {
            setStatus("Delete failed");
            setResult("Could not delete", "", "");
            openScreen(SCREEN_ACTION_RESULT);
          }
          break;

        case 3:
          openScreen(SCREEN_CUSTOM_LIST);
          break;
      }
      break;

    case SCREEN_PROFILE_VIEW:
    case SCREEN_ACTION_RESULT:
      openScreen(SCREEN_HOME);
      break;

    case SCREEN_CUSTOM_EDITOR:
      handleCustomEditorPress();
      break;
  }
}

// ============================================================
// Encoder and buttons
// ============================================================
void handleEncoder() {
  unsigned long now = millis();

  if (now - lastEncMoveMs < ENC_DEBOUNCE_MS) return;

  uint8_t a = digitalRead(ENC_A);
  uint8_t b = digitalRead(ENC_B);
  uint8_t state = (a << 1) | b;

  static const int8_t transTable[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  uint8_t idx = (encPrevState << 2) | state;
  int8_t movement = transTable[idx];

  if (movement != 0) {
    encAcc += movement;
    lastEncMoveMs = now;

    if (encAcc >= 4 || encAcc <= -4) {
      int dir = (encAcc >= 4) ? 1 : -1;
      int count = getCurrentMenuCount();

      int stepSize = 1;

      if (now - lastMenuStepMs < 90) {
        stepSize = 3;
      } else if (now - lastMenuStepMs < 150) {
        stepSize = 2;
      }

      if (count > 0) {
        moveSelection(dir * stepSize, count);
      }

      lastMenuStepMs = now;
      encAcc = 0;
    }
  }

  encPrevState = state;
}

void handleKey1BackHome() {
  bool key1Now = digitalRead(KEY1);

  if (lastKey1 == HIGH && key1Now == LOW) {
    key1PressTime = millis();
    key1LongHandled = false;
  }

  if (key1Now == LOW && !key1LongHandled) {
    if (millis() - key1PressTime > KEY1_LONG_MS) {
      openScreen(SCREEN_HOME);
      key1LongHandled = true;
    }
  }

  if (lastKey1 == LOW && key1Now == HIGH) {
    if (!key1LongHandled) {
      goBackOneLevel();
    }

    delay(60);
  }

  lastKey1 = key1Now;
}

void handleKey0Select() {
  bool key0Now = digitalRead(KEY0);

  if (lastKey0 == HIGH && key0Now == LOW) {
    handleSelect();
    delay(180);
  }

  lastKey0 = key0Now;
}

// ============================================================
// PN532 init
// ============================================================
void initPn532() {
  Serial.println("Initializing PN532 on I2C...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();

  if (!versiondata) {
    Serial.println("Didn't find PN532 board");

    nfcReady = false;

    setStatus("PN532 not found");
    setResult("Check module mode", "Set PN532 to I2C", "Check SDA/SCL");
    openScreen(SCREEN_ACTION_RESULT);

    return;
  }

  Serial.print("Found PN532 firmware ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.SAMConfig();

  nfcReady = true;

  setStatus("PN532 ready");
}

// ============================================================
// Setup / loop
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(800);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(KEY0, INPUT_PULLUP);
  pinMode(KEY1, INPUT_PULLUP);

  lastKey0 = digitalRead(KEY0);
  lastKey1 = digitalRead(KEY1);

  encPrevState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);

  ledcAttach(TFT_BLK, BL_FREQ, BL_RES);
  setBacklight(backlightValue);

  mySPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.init(240, 320);
  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);
  drawCenteredText("OpenSpool Writer", 70, 3, COL_HILITE);
  drawCenteredText("ESP32-S2 + ST7789 + PN532", 108, 2, COL_TEXT);
  drawCenteredText("NDEF + RAW FALLBACK", 138, 2, COL_DIM);

  delay(1600);

  loadCustomProfiles();

  copyProfile(activeProfile, genericProfiles[0]);
  normalizeProfile(activeProfile);
  activeProfileSource = SRC_GENERIC;

  prepareNewCustomFromActive();

  Wire.begin(PN532_SDA, PN532_SCL);
  initPn532();

  openScreen(nfcReady ? SCREEN_HOME : SCREEN_ACTION_RESULT);

  Serial.println();
  Serial.println("========================================");
  Serial.println("ESP32 OPENSPOOL NFC WRITER READY");
  Serial.println("Use NTAG215 or NTAG216");
  Serial.println("Read NDEF OpenSpool enabled");
  Serial.println("Read RAW/eSUN fallback enabled");
  Serial.println("Write / Verify / Clone enabled");
  Serial.println("Generic + Custom(in flash) enabled");
  Serial.println("Edit / Update / Delete custom enabled");
  Serial.println("Robust encoder decode enabled");
  Serial.println("Encoder acceleration enabled");
  Serial.println("KEY0 = select");
  Serial.println("KEY1 = back / hold = home");
  Serial.println("Material color boxes enabled");
  Serial.println("Minimal OpenSpool JSON enabled");
  Serial.println("Partial redraw enabled");
  Serial.println("========================================");
}

void loop() {
  handleEncoder();
  handleKey1BackHome();
  handleKey0Select();
  redrawCurrentParts();
}
