#include <Servo.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <MPU6050_light.h> 

// 1. التعريفات الأساسية (Global Variables)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
MPU6050 mpu(Wire);

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

const int TARGET_DIST = 6; 
const int DIST_THRESHOLD = 1;
int distance;
int Target = 0;
char cmd;


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

int getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void alignToObject() {
  stopMoving();
  while (true) {
    distance = getDistance();
    if (distance > (TARGET_DIST + DIST_THRESHOLD)) {
      analogWrite(EN_A, 120); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      analogWrite(EN_B, 120); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    } 
    else if (distance < (TARGET_DIST - DIST_THRESHOLD) && currentDist > 0) {
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
  Expression(1);
  alignToObject();
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
  bool reached = false,faceLocked = false;
  stopMoving();
  Expression(3);
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
    Expression(2);
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
  // --- PHASE 3: Expressive Reaction ---
  if(faceLocked && reached){
    leftRight(90);
    playWalleSound();
    sweep(&leftEye, 90, 60, 400);
    sweep(&rightEye, 90, 60, 400);
  
    for (int i = 0; i < 4; i++) {
      leftGrip(180); delay(300); 
      leftGrip(0);   delay(300);
    }
  
    delay(5000); // Wait 5 seconds to "interact"
    Expression(0);
    delay(10);1
    neutral();
  }
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
    cmd = Serial.read();
    if (cmd == 'O') objectDetectionSequence();
    else if (cmd == 'F') faceDetectionSequence();
  }
}
