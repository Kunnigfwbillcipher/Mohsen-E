/*
 * MOHSEN-E — ESP-32  
 * ─────────────────────────────────────────────────────
 * 1. Receives rubbish coordinates from AI team via WiFi
 * 2. Translates (x,y) into movement commands
 * 3. Sends commands to Arduino via Serial2
 * ─────────────────────────────────────────────────────
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

//  WiFi Config 
const char* WIFI_SSID = "Wokwi-GUEST";   // we will change it in the future
const char* WIFI_PASS = "";   // we will change it in the future
const int   UDP_PORT  = 1234; // can be changed but no problem to be that number

WiFiUDP        udp;
HardwareSerial ArduinoSerial(2);  // GPIO16=RX2, GPIO17=TX2

//  Thresholds (meters) 
const float TURN_THRESHOLD   = 0.20;  // لو y > 20cm → استدر
const float ARRIVE_THRESHOLD = 0.30;  // لو x < 30cm → إحنا وصلنا

//  State 
float target_x = 0;
float target_y = 0;
bool  hasTarget = false;

//  Setup 
void setup() {
  Serial.begin(115200);
  ArduinoSerial.begin(9600, SERIAL_8N1, 16, 17);

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString()); // we will need it to know the IP address

  udp.begin(UDP_PORT);
  Serial.println("Listening on port " + String(UDP_PORT));

  // send command to arduino
  sendToArduino("READY");
}

// Loop 
void loop() {

  // receive coordinates from ai team
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[128];
    int length = udp.read(buffer, sizeof(buffer) - 1); // this function read the the UDP data and put it in buffer and return the length of reading
    buffer[length] = '\0';

    Serial.println("[AI IN] " + String(buffer));
    parseAIData(String(buffer));
  }

  
  if (hasTarget) {
    String commad = translateToCommand(target_x, target_y);
    sendToArduino(commad);

    if (commad == "COLLECT") {
      hasTarget = false;
      target_x = 0;
      target_y = 0;
    }

    delay(300);  
  }
}

// ── Parse JSON from AI Team ───────────────────
/*
  Expected format:
  { "object": "rubbish", "x": 1.5, "y": -0.3 }

  x = distance forward in meters  (+ = forward)
  y = distance sideways in meters (+ = right, - = left)
*/
void parseAIData(String jsonStr) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, jsonStr);

  if (err) {
    Serial.println("[ERROR] JSON parse failed: " + String(err.c_str()));
    return;
  }

  const char* obj = doc["object"];
  if (String(obj) == "rubbish") {
    target_x = doc["x"].as<float>();
    target_y = doc["y"].as<float>();
    hasTarget = true;

    Serial.printf("[TARGET] x=%.2f  y=%.2f\n", target_x, target_y);

    String info = "TARGET:" + String(target_x, 1) + "," + String(target_y, 1);
    sendToArduino(info);
  }
}

// ── Coordinate → Command Translation ─────────
String translateToCommand(float x, float y) {

  // when we reached
  if (x < ARRIVE_THRESHOLD && abs(y) < TURN_THRESHOLD) {
    Serial.println("[COMMAD] COLLECT");
    return "COLLECT";
  }

  // when y is big -> turn first
  if (abs(y) > TURN_THRESHOLD && abs(y) > abs(x) * 0.5) {
    if (y > 0) {
      Serial.println("[COMMAD] RIGHT");
      return "RIGHT";
    } else {
      Serial.println("[COMMAD] LEFT");
      return "LEFT";
    }
  }

  if (x > ARRIVE_THRESHOLD) {
    Serial.println("[COMMAD] FWD");
    return "FWD";
  }

  Serial.println("[COMMAD] SCAN");
  return "SCAN";
}

// ── Send Command to Arduino ───────────────────
void sendToArduino(String cmd) {
  ArduinoSerial.println(cmd);
  Serial.println("[→ ARDUINO] " + cmd);

  // ACK للـ AI Team
  if (udp.remotePort()) {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print("ACKNOWLEGDE:" + cmd);
    udp.endPacket();
  }
}