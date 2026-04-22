#include <Servo.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 1. التعريفات الأساسية (Global Variables)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

Servo Door; // سيرفو الباب موصل مباشرة بالأردوينو
#define SERVO_MIN 150
#define SERVO_MAX 600

#define TRIG_PIN 2
#define ECHO_PIN 12
#define EN_A 9
#define IN1 8
#define IN2 7
#define EN_B 11
#define IN3 5
#define IN4 4
#define MID_SPEED 180

int distance;
int Target = 0;


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

void stopMoving() {
  analogWrite(EN_A, 0); digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  analogWrite(EN_B, 0); digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void moveForward() {
  analogWrite(EN_A, MID_SPEED); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  analogWrite(EN_B, MID_SPEED); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void playWalleSound() { /* أضف كود الصوت هنا */ }

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

// 3. دوال الأطوار (Sequences)
void objectDetectionSequence() {
  stopMoving();
  delay(300);
  sweep(&leftArm, 90, 30, 800);
  sweep(&leftJoint, 0, 45, 600);
  leftGrip(180);
  delay(400);
  sweep(&leftArm, 30, 25, 300);
  leftGrip(0);
  delay(500);
  sweep(&leftJoint, 45, 0, 600);
  door(0);
  delay(400);
  sweep(&leftJoint, 0, 60, 600);
  leftGrip(180);
  delay(400);
  sweep(&leftJoint, 60, 0, 500);
  sweep(&leftArm, 25, 90, 600);
  leftGrip(90);
  door(90);
  neutral();
  moveForward();
}

void faceDetectionSequence() {
  sweep(&botNeck, 90, 120, 500);
  sweep(&topNeck, 90, 120, 500);
  
  bool faceLocked = false;
  unsigned long steerLast = millis();
  int steerAngle = 45;
  leftRight(steerAngle);

  while (!faceLocked) {
    if (millis() - steerLast >= 2500) {
      steerLast = millis();
      steerAngle = (steerAngle == 45) ? 135 : 45;
      leftRight(steerAngle);
    }
    if (Serial.available()) {
      char sig = Serial.read();
      if (sig == 'L') faceLocked = true;
    }
  }
  leftRight(90);
  playWalleSound();
  sweep(&leftEye, 90, 60, 400);
  sweep(&rightEye, 90, 60, 400);
  for (int i = 0; i < 4; i++) {
    leftGrip(180); delay(300); leftGrip(0); delay(300);
  }
  delay(10000);
  neutral();
  moveForward();
}

// 4. الـ Setup والـ Loop
void setup() {
  Serial.begin(9600);
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);
  
  pinMode(EN_A, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(EN_B, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  
  Door.attach(A1);
  neutral();
  delay(1000);
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'O') objectDetectionSequence();
    else if (cmd == 'F') faceDetectionSequence();
  }
}