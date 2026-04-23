/*
 * ================================================================
 *   MOHSEN-E — ESP-32 
 * ================================================================
 *
 *  ── Object Detection ─────────────────────────────────────────
 *    AI  → ESP  :  "O,220,80,R"
 *    Parsed here:  arr[0]="O" arr[1]="220" arr[2]="80" arr[3]="R"
 *    Angle calc :  dX=60 → angle=11°  side=R
 *    ESP → Arduino:  "OBJ,R,11"   (no parsing needed on Arduino side)
 *
 *  ── Face Recognition ─────────────────────────────────────────
 *    AI  → ESP  :  "FACE,0"   or   "FACE,1"
 *    Parsed here:  arr[0]="FACE"  arr[1]="0"
 *    ESP → Arduino:  "FACE,0"   or   "FACE,1"
 * ================================================================
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

// ── WiFi ─────────────────────────────────────────────────────
const char* WIFI_SSID = "YOUR_WIFI_NAME";  // will be changed
const char* WIFI_PASS = "YOUR_WIFI_PASS";  // will be changed
const int   UDP_PORT  = 1234;

WiFiUDP        udp;
HardwareSerial ArduinoSerial(2);   // RX2=GPIO16 | TX2=GPIO17

// ── Camera config ─────────────────────────────────────────────
const int   CAM_W      = 320;
const float H_FOV      = 60.0;
const float DEG_PER_PX = H_FOV / CAM_W;   // 0.1875 °/pixel
const int   DEAD_ZONE  = 15;               // pixels

// ── Max tokens ────────────────────────────────────────────────
const int MAX_TOKENS = 6;

// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  ArduinoSerial.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[ESP] Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n[ESP] IP: " + WiFi.localIP().toString());

  udp.begin(UDP_PORT);
  Serial.printf("[ESP] Listening on UDP port %d\n", UDP_PORT);

  String readyMsg[2] = {"SYS", "READY"};
  sendArrayToArduino(readyMsg, 2);
}

// ═════════════════════════════════════════════════════════════
void loop() {
  int pkt = udp.parsePacket();
  if (pkt) {
    char buf[128];
    int  len = udp.read(buf, sizeof(buf) - 1);
    buf[len]  = '\0';

    String raw = String(buf);
    raw.trim();
    Serial.println("\n[AI IN] '" + raw + "'");

    // ── Split here in ESP ────────────────────────────────
    String tokens[MAX_TOKENS];
    int    count = splitToArray(raw, ',', tokens, MAX_TOKENS);

    printArray(tokens, count); // for debugging

    // ── Route by mode ────────────────────────────────────
    if (count >= 2) {
      String mode = tokens[0];
      mode.toUpperCase();

      if (mode == "O") {
        handleObject(tokens, count);
      }
      else if (mode == "FACE") {
        handleFace(tokens, count);
      }
      else {
        Serial.println("[ESP] Unknown mode: " + mode);
      }
    }

    // ACK to AI
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print("ACK:" + raw);
    udp.endPacket();
  }

  // Log Arduino feedback 
  if (ArduinoSerial.available()) {
    String fb = ArduinoSerial.readStringUntil('\n');   // fb refers to FeedBack
    fb.trim();
    Serial.println("[ARD] " + fb);
  }
}

// ═════════════════════════════════════════════════════════════
//  splitToArray  —  "O,220,80,R" → ["O","220","80","R"]
//  returns number of tokens
// ═════════════════════════════════════════════════════════════
int splitToArray(String input, char delim,
                 String* out, int maxCount) {
  int count = 0, start = 0;
  for (int i = 0; i <= (int)input.length(); i++) {
    if (i == (int)input.length() || input.charAt(i) == delim) {
      if (count < maxCount) {
        out[count] = input.substring(start, i);
        out[count].trim();
        count++;
      }
      start = i + 1;
    }
  }
  return count;
}

// ── Debug print array ─────────────────────────────────────────
void printArray(String* arr, int count) {
  Serial.printf("[ARRAY] count=%d → ", count);
  for (int i = 0; i < count; i++)
    Serial.printf("[%d]='%s' ", i, arr[i].c_str());
  Serial.println();
}

// ═════════════════════════════════════════════════════════════
//  MODE O  —  Object Detection
//
//  tokens[0] = "O"
//  tokens[1] = X pixel (string)
//  tokens[2] = Y pixel (string)
//  tokens[3] = side "R"/"L"/"C" from AI (ESP recalculates)
//
//  → Sends "OBJ,<side>,<angle>" to Arduino
//    Arduino just uses the values directly, no parsing
// ═════════════════════════════════════════════════════════════
void handleObject(String* tokens, int count) {
  if (count < 3) {
    Serial.println("[OBJ] Need at least 3 tokens: O,X,Y");
    return;
  }

  int objX = tokens[1].toInt();
  int objY = tokens[2].toInt();   // not used for angle but logged

  Serial.printf("[OBJ] X=%d  Y=%d\n", objX, objY);

  // ── Angle calculation ─────────────────────────────────
  int   dX       = objX - (CAM_W / 2);
  float angleF   = (float)abs(dX) * DEG_PER_PX;
  int   angleDeg = (int)round(angleF);

  // ── Side ──────────────────────────────────────────────
  String side;
  if (abs(dX) <= DEAD_ZONE) {
    side     = "C";
    angleDeg = 0;
  } else if (dX > 0) {
    side = "R";
  } else {
    side = "L";
  }

  Serial.printf("[OBJ] dX=%d  angle=%d°  side=%s\n",
                dX, angleDeg, side.c_str());

  // ── Send clean string to Arduino ──────────────────────
  // Format: "OBJ,<side>,<angle>"
  // Arduino reads: cmd="OBJ" | side="R" | angle=11
  String out[3] = {"OBJ", side, String(angleDeg)};
  sendArrayToArduino(out, 3);
}

// ═════════════════════════════════════════════════════════════
//  MODE FACE  —  Face Recognition
//
//  tokens[0] = "FACE"
//  tokens[1] = "0" (Friend) or "1" (Stranger)
//
//  → Sends "FACE,<cat>" to Arduino directly
// ═════════════════════════════════════════════════════════════
void handleFace(String* tokens, int count) {
  if (count < 2) {
    Serial.println("[FACE] Need 2 tokens: FACE,cat");
    return;
  }

  String cat = tokens[1];
  Serial.printf("[FACE] cat=%s  (%s)\n",
    cat.c_str(), cat == "0" ? "Friend" : "Stranger");

  // Forward clean to Arduino: "FACE,0" or "FACE,1"
  String out[2] = {"FACE", cat};
  sendArrayToArduino(out, 2);
}

// ═════════════════════════════════════════════════════════════
String buildArrayString(String* items, int count) {
  String payload = "[";

  for (int i = 0; i < count; i++) {
    if (i > 0) payload += ",";
    payload += "\"";
    payload += items[i];
    payload += "\"";
  }

  payload += "]";
  return payload;
}

void sendArrayToArduino(String* items, int count) {
  String cmd = buildArrayString(items, count);
  ArduinoSerial.println(cmd);
  Serial.println("[ARD ARRAY] " + cmd);
}

void sendToArduino(String cmd) {
  ArduinoSerial.println(cmd);
  Serial.println("[→ARD] '" + cmd + "'");
}
