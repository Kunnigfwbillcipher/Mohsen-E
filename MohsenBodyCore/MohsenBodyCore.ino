#include <Servo.h>

Servo LeftA;
Servo RightA;
Servo HandL;
Servo HandR;

Servo Door;


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

char direction,mode,category;
int angle;
float distance;



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
  
  
  LeftA.attach(A0);
  RightA.attach(A2);
  Door.attach(A1);
  HandR.attach(A3);
  HandL.attach(A4);
  Servo_Base();
}

void loop() {
 
  if(Serial.available()){
	char Buffer[50];
    Serial.readBytesUntil('\n',Buffer,sizeof(Buffer));
    
    sscanf(Buffer,"%c,%c,%c,%d",&mode,&direction,&category,&angle);
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
  }else if(mode = 'F'){
    switch(category){
      case '0':{
        
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
void Servo_Base(){
  
  LeftA.write(90);
  RightA.write(90);
  
  Door.write(0);
  HandR.write(0);
  HandL.write(0);
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
void stopRobot() {
  
  analogWrite(EN_A, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  
  
  analogWrite(EN_B, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
   
}
