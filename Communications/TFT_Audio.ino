/*
 * MOHSEN-E — Arduino  |  TFT and Audio module
 * ─────────────────────────────────────────────
 * Recieve commands from ESP and process TFT and Audio Module
 * ─────────────────────────────────────────────
 */

#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ── DFPlayer ──────────────────────────────────
SoftwareSerial      dfSerial(9, 10);
DFRobotDFPlayerMini player;
bool audioReady = false;

#define SND_READY    1   // 0001.mp3 — startup
#define SND_MOVING   2   // 0002.mp3 — moving
#define SND_COLLECT  3   // 0003.mp3 — collecting
#define SND_DONE     4   // 0004.mp3 — task done
#define SND_ALERT    5   // 0005.mp3 — alert

// ── TFT ───────────────────────────────────────
#define TFT_CS  3
#define TFT_DC  4
#define TFT_RST 8
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

#define C_BG     0x0000
#define C_YELLOW 0xFFE0
#define C_GREEN  0x07E0
#define C_RED    0xF800
#define C_BLUE   0x001F
#define C_WHITE  0xFFFF
#define C_GRAY   0x7BEF
#define C_ORANGE 0xFC00
#define C_CYAN   0x07FF

// ── State ─────────────────────────────────────
String cmdBuffer = "";
String lastCmd   = "";
float  tgt_x = 0, tgt_y = 0;

// ── Setup ─────────────────────────────────────
void setup() {
  Serial.begin(9600);

  dfSerial.begin(9600);
  if (player.begin(dfSerial)) {
    audioReady = true;
    player.volume(25);
    player.play(SND_READY);
  }

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(C_BG);
  drawBootScreen();
  delay(1500);
  drawMainUI();
}

// ── Loop ──────────────────────────────────────
void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      handleCommand(cmdBuffer);
      cmdBuffer = "";
    } else {
      cmdBuffer += c;
    }
  }
}

// ── Command Handler ───────────────────────────
void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  lastCmd = cmd;

  // ── Coordinates update from ESP ──
  if (cmd.startsWith("TARGET:")) {
    // Format: TARGET:1.5,-0.3
    String coords = cmd.substring(7);
    int comma = coords.indexOf(',');
    tgt_x = coords.substring(0, comma).toFloat();
    tgt_y = coords.substring(comma + 1).toFloat();
    updateCoordinates(tgt_x, tgt_y);
    return;
  }

  // ── Movement commands (معاذ handles: TFT + Audio only) ──
  if (cmd == "FWD") {
    playSound(SND_MOVING);
    updateStatus("MOVING →", C_GREEN);
    updateDirection("▲  FORWARD");
  }
  else if (cmd == "BWD") {
    playSound(SND_MOVING);
    updateStatus("MOVING ←", C_ORANGE);
    updateDirection("▼  BACKWARD");
  }
  else if (cmd == "RIGHT") {
    playSound(SND_MOVING);
    updateStatus("TURNING", C_CYAN);
    updateDirection("► RIGHT");
  }
  else if (cmd == "LEFT") {
    playSound(SND_MOVING);
    updateStatus("TURNING", C_CYAN);
    updateDirection("◄ LEFT");
  }
  else if (cmd == "STOP") {
    updateStatus("STOPPED", C_WHITE);
    updateDirection("■  STOP");
  }
  else if (cmd == "COLLECT") {
    playSound(SND_COLLECT);
    updateStatus("COLLECTING!", C_YELLOW);
    updateDirection("✦ COLLECT");
  }
  else if (cmd == "SCAN") {
    updateStatus("SCANNING...", C_BLUE);
    updateDirection("↻  SCAN");
  }
  else if (cmd == "READY") {
    updateStatus("SYSTEM READY", C_GREEN);
  }
  else if (cmd == "ALERT") {
    playSound(SND_ALERT);
    updateStatus("!! ALERT !!", C_RED);
  }
  else if (cmd.startsWith("BAT:")) {
    int pct = constrain(cmd.substring(4).toInt(), 0, 100);
    updateBattery(pct);
  }

  // بعت ACK للـ ESP
  Serial.println("ACKNOWLEDGE:" + cmd);
}

// ── Audio ─────────────────────────────────────
void playSound(int id) {
  if (audioReady) player.play(id);
}

// ── TFT ───────────────────────────────────────
void drawBootScreen() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_YELLOW); tft.setTextSize(2);
  tft.setCursor(15, 15); tft.println("MOHSEN-E");
  tft.setTextColor(C_GRAY); tft.setTextSize(1);
  tft.setCursor(20, 45); tft.println("Rubbish Collector");
  tft.drawRect(10, 65, 140, 10, C_WHITE);
  for (int i = 0; i <= 136; i += 4) {
    tft.fillRect(12, 67, i, 6, C_GREEN); delay(20);
  }
  tft.setTextColor(C_GREEN); tft.setCursor(45, 82);
  tft.println("READY!");
}

void drawMainUI() {
  tft.fillScreen(C_BG);

  // Header
  tft.fillRect(0, 0, 160, 16, C_YELLOW);
  tft.setTextColor(C_BG); tft.setTextSize(1);
  tft.setCursor(30, 4); tft.println("MOHSEN-E  v1.0");

  // Status box
  tft.drawRect(2, 20, 156, 22, C_GRAY);
  tft.setTextColor(C_GRAY); tft.setCursor(6, 22);
  tft.println("STATUS:");
  tft.setTextColor(C_GREEN); tft.setCursor(6, 32);
  tft.println("STANDBY");

  // Direction box
  tft.drawRect(2, 46, 156, 22, C_GRAY);
  tft.setTextColor(C_GRAY); tft.setCursor(6, 48);
  tft.println("ACTION:");
  tft.setTextColor(C_BLUE); tft.setCursor(6, 58);
  tft.println("---");

  // Coordinates box
  tft.drawRect(2, 72, 156, 28, C_GRAY);
  tft.setTextColor(C_GRAY); tft.setCursor(6, 74);
  tft.println("TARGET (m):");
  tft.setTextColor(C_CYAN); tft.setCursor(6, 84);
  tft.println("X: --   Y: --");

  // Battery
  updateBattery(100);
}

void updateStatus(String msg, uint16_t color) {
  tft.fillRect(3, 30, 154, 10, C_BG);
  tft.setTextColor(color); tft.setTextSize(1);
  tft.setCursor(6, 32); tft.println(msg);
}

void updateDirection(String dir) {
  tft.fillRect(3, 56, 154, 10, C_BG);
  tft.setTextColor(C_ORANGE); tft.setTextSize(1);
  tft.setCursor(6, 58); tft.println(dir);
}

void updateCoordinates(float x, float y) {
  tft.fillRect(3, 82, 154, 14, C_BG);
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(6, 84);
  tft.print("X:"); tft.print(x, 1);
  tft.print("m  Y:"); tft.print(y, 1); tft.print("m");
}

void updateBattery(int pct) {
  tft.fillRect(2, 104, 156, 14, C_BG);
  tft.setTextColor(C_GRAY); tft.setTextSize(1);
  tft.setCursor(2, 106); tft.print("BAT:");
  tft.drawRect(30, 106, 100, 8, C_GRAY);
  tft.fillRect(130, 108, 3, 4, C_GRAY);
  uint16_t c = (pct > 50) ? C_GREEN : (pct > 20) ? C_ORANGE : C_RED;
  tft.fillRect(31, 107, map(pct, 0, 100, 0, 98), 6, c);
  tft.setTextColor(C_WHITE);
  tft.setCursor(136, 106); tft.print(String(pct)+"%");
}