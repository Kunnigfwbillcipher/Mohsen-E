/*
 * ================================================================
 *   MOHSEN-E — Arduino Uno 
 * ================================================================
 *
 *  CHANGES v5:
 *    1. No string splitting here — ESP sends ready values
 *       Arduino uses simple startsWith() + substring() only
 *    2. OLED changed to Adafruit SH1106G (Adafruit_SH110X lib)
 *       + battery reading from A0 (WALL-E style bars)
 *
 *  Receives from ESP-32:
 *    "OBJ,R,11"    →  turn RIGHT 11°, then FWD
 *    "OBJ,L,19"    →  turn LEFT  19°, then FWD
 *    "OBJ,C,0"     →  already centered → FWD
 *    "FACE,0"      →  Friend   → wave + happy
 *    "FACE,1"      →  Stranger → alert
 *    "SYS,READY"   →  boot complete
 *
 * ================================================================
 */

#include <Wire.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Adafruit_PWMServoDriver.h>
#include <MPU6050_light.h> 

// ── OLED SH1106 ───────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define BATTERY_PIN   A0

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── DFPlayer ──────────────────────────────────────────────────
SoftwareSerial      dfSerial(9, 10);
DFRobotDFPlayerMini player;
bool audioReady = false;

#define SND_READY    1
#define SND_FRIEND   2
#define SND_STRANGER 3
#define SND_TURNING  4
#define SND_MOVING   5
#define SND_ALERT    6


void playWalleSound() { /* أضف كود الصوت هنا */ }





// ── Serial buffer ─────────────────────────────────────────────
String serialBuf = "";
const int MAX_TOKENS = 4;

// ── Battery tracking ──────────────────────────────────────────
int   batteryLevel  = 10;   // 0–10 bars
unsigned long lastBattUpdate = 0;
const unsigned long BATT_INTERVAL = 500;   // ms


//════ DC motors ═════════════════════════════════════════════════════════

const int TARGET_DIST = 6; 
const int DIST_THRESHOLD = 1;
int distance;
int Target = 0;
char cmd;


#define EN_A 9
#define IN1 8
#define IN2 7
#define EN_B 11
#define IN3 5
#define IN4 4
#define MID_SPEED 180




void alignToObject() {
  stopMoving();
  while (true) {
     distance = getDistance();
    if (distance > (TARGET_DIST + DIST_THRESHOLD)) {
      analogWrite(EN_A, 120); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      analogWrite(EN_B, 120); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    } 
    else if (distance < (TARGET_DIST - DIST_THRESHOLD) && distance > 0) {
      analogWrite(EN_A, 120); digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      analogWrite(EN_B, 120); digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    } 
    else {
      stopMoving();
      break; 
    }
    delay(50); 
  }
}

void stopMoving() {
  analogWrite(EN_A, 0); digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  analogWrite(EN_B, 0); digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}



void moveForward() {
  analogWrite(EN_A, MID_SPEED); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  analogWrite(EN_B, MID_SPEED); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}


// ═══════════ ultrasonic ══════════════════════════════════════════════════
#define TRIG_PIN 2
#define ECHO_PIN 12




int getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}



// ═══════servoes══════════════════════════════════════════════════════

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
MPU6050 mpu(Wire);


#define SERVO_MIN 150
#define SERVO_MAX 600





void channel(int servo, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(servo, 0, pulse);
}

void leftEye(int a)   { channel(0, a); }
void rightEye(int a)  { channel(1, a); }
void leftJoint(int a) { channel(2, a); }
void rightJoint(int a) { channel(3, a); }
void leftRight(int a) { channel(4, a); }
void botNeck(int a)   { channel(5, a); }
void topNeck(int a)   { channel(6, a); }
void leftArm(int a)   { channel(7, a); }
void rightArm(int a)  { channel(8, a); }
void leftGrip(int a)  { channel(9, a); }
void rightGrip(int a) { channel(10, a); }
void door(int a)      { channel(11, a); }


void Expressions(int index){
  switch(index){
    case 0:leftEye(180);rightEye(180);//Angry
    break;
    case 1: leftEye(90);rightEye(90); //Regular
    break;
    case 2:leftEye(45);rightEye(45);//Empathy
    break;
    case 3: leftEye(0);rightEye(0); //Terrified
  }
    
  
}


void sweep(void (*servoFn)(int), int from, int to, int durationMs) {
  int steps = abs(to - from);
  if (steps == 0) return;
  int delayPerStep = durationMs / steps;
  int dir = (to > from) ? 1 : -1;
  for (int a = from; a != to; a += dir) {
    servoFn(a);
    delay(delayPerStep);
  }
  servoFn(to);
}

void neutral() {
  leftEye(90);  rightEye(90);
  leftArm(90);  rightArm(90);
  leftJoint(0); rightJoint(0);
  leftGrip(90); rightGrip(90);
  botNeck(90);  topNeck(90);
  leftRight(90);
  door(90);
}


// ══════════════ Object handling═══════════════════════════════════════════════

typedef void (*ServoFunc)(int);
void handleObj(String side, int angle) {
  stopMoving();
  Expressions(1);
  
 
  alignToObject(); 
  delay(300);

  ServoFunc activeArm;
  ServoFunc activeJoint;
  ServoFunc activegrip;
  
  if (side == "left") {
    activeArm = leftArm;
    activeJoint = leftJoint;
    activegrip=leftGrip;
  } else {
    activeArm = rightArm;
    activeJoint = rightJoint;
    activegrip=rightGrip;
  }

 
  sweep(activeArm, 90, 30, 800);
  sweep(activeJoint, 0, 45, 600);
  sweep(activegrip, 0, 180, 600);
  
  
 
  delay(400);
  
  sweep(activeArm, 30, 25, 300);
  sweep(activegrip, 180, 0, 600);
  delay(500);

 
  sweep(activeJoint, 45, 0, 600);
  door(0);
  delay(400);
  
  
  neutral();
  moveForward();
}


// ════ face handling ═════════════════════════════════════════════════════════



void faceDetectionSequence() {
  bool reached = false,faceLocked = false;
  stopMoving();
  Expressions(3);
  sweep(&botNeck, 90, 120, 500);
  sweep(&topNeck, 90, 120, 500);
  
  unsigned long steerLast = millis();
  int steerAngle = 45;
  leftRight(steerAngle);

  mpu.update();
  float targetHeading = mpu.getAngleZ(); // Lock current heading
  float Kp = 4.5; // Precision correction factor
  
  while (!faceLocked) {
    if (millis() - steerLast >= 2500) {
      steerLast = millis();
      steerAngle = (steerAngle == 45) ? 135 : 45;
      leftRight(steerAngle);
    }
    if (Serial.available()) {
      if (cmd == 'L') faceLocked = true;
    }
  }
  if(faceLocked){
    Expressions(2);
    while (!reached) {
      distance = getDistance();
      mpu.update();
      float error = targetHeading - mpu.getAngleZ();
      int correction = (int)(error * Kp);

    if (distance <= 6 && distance > 0) {
      stopMoving();
      reached = true;
    } 
    else {
      // Speed logic
      int baseSpeed = (distance > 16) ? MID_SPEED : 120;
      
      // Apply MPU precision steering
      analogWrite(EN_A, constrain(baseSpeed + correction, 0, 255)); 
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      
      analogWrite(EN_B, constrain(baseSpeed - correction, 0, 255)); 
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    }
  }
  delay(30); 
}

}
// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);

  // OLED init  (same as uploaded file)
  display.begin(0x3C, true);
  display.clearDisplay();

  // Boot screen
  //drawBoot(); <----------------moaaz code
  delay(1500);

  // DFPlayer
  dfSerial.begin(9600);
  if (player.begin(dfSerial)) {
    audioReady = true;
    player.volume(25);
    player.play(SND_READY);
  }

  //drawIdle(); <----------------moaaz code
}

// ═════════════════════════════════════════════════════════════
void loop() {
  // ── Read battery periodically ────────────────────────────
  if (millis() - lastBattUpdate >= BATT_INTERVAL) {
    lastBattUpdate = millis();
    int raw = analogRead(BATTERY_PIN);
    batteryLevel = constrain(map(raw, 700, 1023, 0, 10), 0, 10);
  }

  // ── Receive from ESP-32 ──────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCmd(serialBuf);
      serialBuf = "";
    } else {
      serialBuf += c;
    }
  }
}

// ═════════════════════════════════════════════════════════════
//  processCmd  —  NO splitting needed, ESP already did it
//  Just read the values directly from the formatted string
// ═════════════════════════════════════════════════════════════
int splitLegacyCmd(String input, char delim, String* out, int maxCount) {
  int count = 0;
  int start = 0;

  for (int i = 0; i <= input.length(); i++) {
    if (i == input.length() || input.charAt(i) == delim) {
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

int parseQuotedArray(String input, String* out, int maxCount) {
  input.trim();
  if (!input.startsWith("[") || !input.endsWith("]")) return 0;

  int count = 0;
  bool inQuotes = false;
  String current = "";

  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);

    if (c == '"') {
      if (inQuotes && count < maxCount) {
        out[count] = current;
        out[count].trim();
        count++;
        current = "";
      }
      inQuotes = !inQuotes;
    } else if (inQuotes) {
      current += c;
    }
  }

  return count;
}

int parseCmdTokens(String input, String* out, int maxCount) {
  input.trim();
  if (input.startsWith("[")) return parseQuotedArray(input, out, maxCount);
  return splitLegacyCmd(input, ',', out, maxCount);
}

void printTokens(String* tokens, int count) {
  Serial.print("[TOKENS]");
  for (int i = 0; i < count; i++) {
    Serial.print(" [");
    Serial.print(i);
    Serial.print("]='");
    Serial.print(tokens[i]);
    Serial.print("'");
  }
  Serial.println();
}

void processCmd(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  Serial.println("ACK:" + cmd);

  String tokens[MAX_TOKENS];
  int count = parseCmdTokens(cmd, tokens, MAX_TOKENS);
  if (count == 0) {
    Serial.println("ERR:BAD_CMD");
    return;
  }

  printTokens(tokens, count);

  String mode = tokens[0];
  mode.toUpperCase();

  // ── "OBJ,R,11"  or  "OBJ,L,19"  or  "OBJ,C,0" ──────────
  if (mode == "OBJ") {
    if (count < 3) {
      Serial.println("ERR:OBJ_TOKENS");
      return;
    }

    String side = tokens[1];
    side.toUpperCase();
    int angle = tokens[2].toInt();
    handleObj(side, angle);                      // handleObj() -> the whole logic of mohsen in <Object Detection Mode> must be written here 
  }

  // ── "FACE,0"  or  "FACE,1" ──────────────────────────────
  else if (mode == "FACE") {
    if (count < 2) {
      Serial.println("ERR:FACE_TOKENS");
      return;
    }

    handleFace(tokens[1].toInt());             // handleFace() -> the whole logic of mohsen in <Face Detection Mode> must be written here 
  }

  // ── "SYS,READY" ─────────────────────────────────────────
  else if (mode == "SYS") {
    if (count < 2) {
      Serial.println("ERR:SYS_TOKENS");
      return;
    }

    String sub = tokens[1];
    sub.toUpperCase();
    if (sub == "READY") {
      playSound(SND_READY); 
      drawIdle(); 
    }
    else if (sub == "ALERT") {
      playSound(SND_ALERT); 
      drawAlert(); 
    }
  }
  else {
    Serial.println("ERR:UNKNOWN_MODE");
  }
}

// ═════════════════════════════════════════════════════════════
//  handleObj  —  side = "R"/"L"/"C"  |  angle = degrees
// ═════════════════════════════════════════════════════════════

void drawObjScreen(const char* action, int angle, char side) {
  display.clearDisplay();

  // header bar
  display.fillRect(0, 0, 128, 13, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(18, 3);
  display.println("OBJ DETECTED");

  // action
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 18);
  if      (side == 'R') display.println(">> TURN RIGHT");
  else if (side == 'L') display.println("<< TURN LEFT");
  else                  display.println("   CENTERED");

  // divider
  display.drawLine(0, 28, 128, 28, SH110X_WHITE);

  // angle
  if (side != 'C') {
    display.setCursor(4, 34);
    display.print("Angle : ");
    display.print(angle);
    display.println(" deg");
    display.setCursor(4, 46);
    display.println("Aligning...");
  } else {
    display.setCursor(4, 34);
    display.println("Already aligned");
    display.setCursor(4, 46);
    display.println("Moving FWD...");
  }

  // battery bar at bottom
  // (compact version — just text)
  display.setCursor(70, 56);
  display.print("BAT:");
  display.print(batteryLevel * 10);
  display.print("%");

  display.display();
}

// ═════════════════════════════════════════════════════════════
//  Audio
// ═════════════════════════════════════════════════════════════
void playSound(int id) {
  if (audioReady) player.play(id);
}

// ═════════════════════════════════════════════════════════════
//  OLED Screens  —  using Adafruit SH1106G (from uploaded file)
// ═════════════════════════════════════════════════════════════

// ── Battery bars  (WALL-E style — from uploaded code) ─────────
void drawBatteryBars() {
  // outer frame
  display.drawRect(5, 10, 118, 44, SH110X_WHITE);
  // terminal nub
  display.fillRect(123, 17, 4, 30, SH110X_WHITE);
  // fill bars
  for (int i = 0; i < batteryLevel; i++) {
    int xPos = 10 + (i * 11);
    display.fillRect(xPos, 15, 8, 34, SH110X_WHITE);
  }
  // percentage text
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(40, 56);
  display.print("BATT: ");
  display.print(batteryLevel * 10);
  display.print("%");
}

// ── Boot splash ───────────────────────────────────────────────
void drawBoot() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(14, 10);
  display.println("MOHSEN-E");
  display.setTextSize(1);
  display.setCursor(22, 34);
  display.println("Initializing...");
  // progress bar
  display.drawRect(10, 46, 108, 8, SH110X_WHITE);
  display.display();
  for (int i = 0; i <= 104; i += 8) {
    display.fillRect(12, 48, i, 4, SH110X_WHITE);
    display.display();
    delay(25);
  }
}

// ── Idle screen ───────────────────────────────────────────────
void drawIdle() {
  display.clearDisplay();
  // header bar
  display.fillRect(0, 0, 128, 13, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(22, 3);
  display.println("MOHSEN-E  v5");
  // status
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(26, 22);
  display.println("IDLE");
  display.setTextSize(1);
  display.setCursor(14, 44);
  display.println("Waiting for AI...");
  display.display();
}

// ── Object detection screen ───────────────────────────────────


// ── Face screen ───────────────────────────────────────────────
void drawFaceScreen(const char* label, bool isFriend) {
  display.clearDisplay();

  // header
  display.fillRect(0, 0, 128, 13, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(30, 3);
  display.println("FACE MODE");

  // face circle
  display.setTextColor(SH110X_WHITE);
  display.drawCircle(64, 36, 16, SH110X_WHITE);

  if (isFriend) {
    // eyes filled
    display.fillCircle(57, 31, 2, SH110X_WHITE);
    display.fillCircle(71, 31, 2, SH110X_WHITE);
    // smile
    display.drawLine(56, 40, 59, 43, SH110X_WHITE);
    display.drawLine(59, 43, 69, 43, SH110X_WHITE);
    display.drawLine(69, 43, 72, 40, SH110X_WHITE);
  } else {
    // eyes hollow
    display.drawCircle(57, 31, 2, SH110X_WHITE);
    display.drawCircle(71, 31, 2, SH110X_WHITE);
    // frown
    display.drawLine(56, 44, 59, 41, SH110X_WHITE);
    display.drawLine(59, 41, 69, 41, SH110X_WHITE);
    display.drawLine(69, 41, 72, 44, SH110X_WHITE);
  }

  // label
  display.setTextSize(1);
  display.setCursor(isFriend ? 38 : 20, 56);
  display.println(label);

  // battery compact
  display.setCursor(80, 3);
  display.setTextColor(SH110X_BLACK);
  display.print(batteryLevel * 10);
  display.print("%");

  display.display();
}

// ── Alert screen ──────────────────────────────────────────────
void drawAlert() {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.drawRect(3, 3, 122, 58, SH110X_WHITE);
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(24, 16);
  display.println("ALERT!");
  display.setTextSize(1);
  display.setCursor(20, 40);
  display.println("!! WARNING !!");
  display.display();
}

// ── Full battery screen  (WALL-E style from uploaded file) ────
void drawBatteryScreen() {
  display.clearDisplay();
  drawBatteryBars();
  display.display();
}




void handleSound(String side, int angle) {
  if (side == "C") {
    playSound(SND_MOVING); 
   drawObjScreen("CENTERED", 0, 'C'); 
    moveForward();
  } else {
    playSound(SND_TURNING); 
    drawObjScreen(
      side == "R" ? "TURN RIGHT" : "TURN LEFT",
      angle,
      side.charAt(0)
    );
    moveForward();       
  }
}

// ═════════════════════════════════════════════════════════════
//  handleFace  —  cat = 0 (Friend) | 1 (Stranger)
// ═════════════════════════════════════════════════════════════
void handleFace(int cat) {

  bool reached = false,faceLocked = false;
  
  stopMoving();
  Expressions(3);
  sweep(&botNeck, 90, 120, 500);
  sweep(&topNeck, 90, 120, 500);
  
  unsigned long steerLast = millis();
  int steerAngle = 45;
  leftRight(steerAngle);

  mpu.update();
  float targetHeading = mpu.getAngleZ(); // Lock current heading
  float Kp = 4.5; // Precision correction factor
  
  while (!faceLocked) {
    if (millis() - steerLast >= 2500) {
      steerLast = millis();
      steerAngle = (steerAngle == 45) ? 135 : 45;
      leftRight(steerAngle);
    }
    if (Serial.available()) {
      if (cmd == 'L') faceLocked = true;
    }
  }
  if(faceLocked){
    Expressions(2);
    while (!reached) {
      distance = getDistance();
      mpu.update();
      float error = targetHeading - mpu.getAngleZ();
      int correction = (int)(error * Kp);

    if (distance <= 6 && distance > 0) {
      stopMoving();
      reached = true;
    } 
    else {
      // Speed logic
      int baseSpeed = (distance > 16) ? MID_SPEED : 120;
      
      // Apply MPU precision steering
      analogWrite(EN_A, constrain(baseSpeed + correction, 0, 255)); 
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      
      analogWrite(EN_B, constrain(baseSpeed - correction, 0, 255)); 
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    }
  }
  delay(30); 

  if (cat == 0) {
    playSound(SND_FRIEND);
    drawFaceScreen("FRIEND", true);

    leftRight(90);
    playWalleSound();
    sweep(&leftEye, 90, 60, 400);
    sweep(&rightEye, 90, 60, 400);
  
    for (int i = 0; i < 4; i++) {
      leftGrip(180); delay(300); 
      leftGrip(0);   delay(300);
    }
  
    delay(5000); // Wait 5 seconds to "interact"
    Expressions(0);
    delay(10);
    neutral();
  } 
  
  else {
    playSound(SND_STRANGER);
    drawFaceScreen("STRANGER", false);
   leftRight(90);
    playWalleSound();
    sweep(&leftEye, 90, 130, 400);
    sweep(&rightEye, 90, 130, 400);
  
  for (int i = 0; i < 2; i++) {
        leftRight(70); delay(100);
        leftRight(110); delay(100);
    }
    leftRight(90);
  
    delay(5000); // Wait 5 seconds to "interact"
    Expressions(0);
    delay(10);
    neutral();
  }     

  }
  }

