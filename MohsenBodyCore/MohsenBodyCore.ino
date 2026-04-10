#include <Servo.h>

Servo LeftA;
Servo RightA;
Servo HandL;
Servo HandR;

Servo Door;
#define EN_A  9    
#define IN1   8   
#define IN2   7  

#define EN_B  11    
#define IN3   5    
#define IN4   4    

#define FULL_SPEED  255
#define MID_SPEED   180

int Left,Right;
int distance;
int Target = 0;
 
void setup() {
  Serial.begin(9600);
  
  pinMode(EN_A, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(EN_B, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  
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
    
    sscanf(Buffer,"%d,%d,%d,%d",&distance,&Left,&Right,&Target);
  }
  
  if(distance > 20 && Target == 0){
  	moveForward();
  }if(Right){
    moveBackward();
    delay(100);
  	turnRight(Right);
  }if(Left){
    moveBackward();
    delay(100);
    turnLeft(Left);
  }if(Target && distance < 20){
  	stopRobot();
    if(Right){
    	RightA.write(180);
      	HandR.write(180);
      	RightA.write(140);
      	Door.write(180);
      	HandR.write(0);
      	RightA.write(90);
      	Door.write(0);
    }else if(Left){
      	LeftA.write(180);
      	HandL.write(180);
      	LeftA.write(140);
      	Door.write(180);
      	HandL.write(0);
      	LeftA.write(90);
      	Door.write(0);
    }else{
    	Door.write(180);
      	moveForward();
      	delay(100);
      	stopRobot();
      	Door.write(0);
    }
  }
}

void Servo_Base(){
  LeftA.write(90);
  RightA.write(90);
  Door.write(0);
  HandR.write(0);
  HandL.write(0);
}

void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(EN_A, MID_SPEED);
  analogWrite(EN_B, MID_SPEED);
}

void moveBackward() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(EN_A, MID_SPEED);
  analogWrite(EN_B, MID_SPEED);
}

void turnRight(int time) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(EN_A, MID_SPEED);
  analogWrite(EN_B, MID_SPEED);
  delay(time);
}
void turnLeft(int time){
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);  digitalWrite(IN4, LOW);
  analogWrite(EN_A, MID_SPEED);
  analogWrite(EN_B, MID_SPEED);
  delay(time);
}

void stopRobot() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(EN_A, 0);
  analogWrite(EN_B, 0);
}
