#include <Servo.h>

#include <Adafruit_PWMServoDriver.h>

#define min 150  
#define max 600 // adjust if the servo isn't reaching its peak angle

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

//use this function to control servos

void channel(int servo ,int angle)
{
  int pulse = map(angle,0,180,min,max); 
  pwm.setPWM(servo,0,pulse);
} 
//channel(3,90); as example
//since i got different servos so i should tone every kind of the 3 kinds if there were any problem by changing the min and max pulse
void leftEye    (int angle)  { channel(7,  angle); }
void rightEye   (int angle)  { channel(8,  angle); }
void leftJoint  (int angle)  { channel(6,  angle); }
void rightJoint (int angle)  { channel(3,  angle); }
void leftRight  (int angle)  { channel(4,  angle); } // To look left and right and might be tilting so i dunno i would leavt it for now
void botNeck    (int angle)  { channel(9,  angle); }
void topNeck    (int angle)  { channel(10,  angle); }
void leftArm    (int angle)  { channel(5,  angle); }
void rightArm   (int angle)  { channel(2,  angle); }
void leftGrip   (int angle)  { channel(4,  angle); }
void rightGrip  (int angle)  { channel(1, angle); }
void Door       (int angle)  { channel(0, angle); }


#define TRIG_PIN 2
#define ECHO_PIN 12


#define EN_A  9    
#define IN1   8   
#define IN2   7  

#define EN_B  11    
#define IN3   5    
#define IN4   4    

//#define FULL_SPEED  255
#define MID_SPEED   180

char direction,mode,category,object;
int angle;
float distance;
int time = millis();


float setPoint = 20.0;

// PID constants
float Kp = 8.0;
float Ki = 0.05;
float Kd = 3.0;

// error variables
float error = 0;
float previousError = 0;
float integral = 0;
float derivative = 0;

unsigned long previousTime = 0;

 
void setup() {
  Serial.begin(9600);
  
  pinMode(EN_A, OUTPUT); 
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  
  
  pinMode(EN_B, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
 
  if(Serial.available()){
	char Buffer[50];
    Serial.readBytesUntil('\n',Buffer,sizeof(Buffer));
    
    sscanf(Buffer,"%c,%c,%c,%c,%d",&mode,&direction,&category,&object,&angle);
  }
  
  distance = getDistance();

  if(mode == 'O'){
    if(distance > 20){
      if(direction == 'R'){
        turnRight(angle);
  	    moveForward(calculatePID_speed());
      }if(direction == 'L'){
        turnLeft(angle);
  	    moveForward(calculatePID_speed());
      }
    }
    if(distance < 20){
       stop();
      if(object == 'G'){
        turnRight(10);
        leftArm(0);
        leftGrip(0);
        leftArm(90);
        Door(180);
        leftJoint(180);
        leftGrip(180);
        leftJoint(180);
        Door(0);
      }else if(object == 'T'){
        turnLeft(10);
        rightArm(0);
        rightGrip(0);
        rightArm(90);
        Door(180);
        rightJoint(180);
        rightGrip(180);
        rightJoint(180);
        Door(0);
      }
    }
  }else if(mode == 'F'){
    switch(category){
      case '0':{
         moveForward(calculatePID_speed());
         if(distance < 20){
          stop();
          rightArm(180);
         }
         if(millis() - time < 5000){
           rightGrip(0);
           if((millis() - time) % 2000 < 1000)
             rightGrip(180);
         }
        }
        break;
      } 
      case '1':{

        break;
      }
      case '2':{
      
        break;
      }
    }
  }
  if(millis() - time > 5000){
    time = millis();
  }
}
float getDistance()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2.0;

    return distance;
}
int calculatePID_speed()
{
  	
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - previousTime) / 1000.0;

    if (deltaTime <= 0)
        return 0;

    float distance = getDistance();

    error = setPoint - distance;

    integral += error * deltaTime;
    derivative = (error - previousError) / deltaTime;

    float output = (Kp * error) + (Ki * integral) + (Kd * derivative);

  	float motorspeed;
    float basespeed=150.0;
    float motorSpeed =basespeed-output;

    motorSpeed = constrain(motorSpeed, -255, 255);

    previousError = error;
    previousTime = currentTime;

    return motorSpeed;
}
void moveForward(int speed ) {
  
  analogWrite(EN_A, speed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  analogWrite(EN_B, speed);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  
}
void moveBackward() {
  
  analogWrite(EN_A, MID_SPEED);
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  
  analogWrite(EN_B, MID_SPEED);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
 
  
}
void turnRight(int time) {
  
  analogWrite(EN_A, MID_SPEED);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  analogWrite(EN_B, MID_SPEED);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
 
 
  delay(time);
}
void turnLeft(int time){
  
  analogWrite(EN_A, MID_SPEED);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  analogWrite(EN_B, MID_SPEED);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  
  delay(time);
  
}
void stop() {
  
  analogWrite(EN_A, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  
  
  analogWrite(EN_B, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
   
}
