#include <Wire.h>
#include <MPU6050.h>
#define led1 A3
#define led2 A2
#define led3 A1
#define led4 A0
MPU6050 mpu;
bool state = LOW;

int16_t ax,ay,az,gx,gy,gz;
float last,pitch,roll;
unsigned long print=0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mpu.initialize();
  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);
  pinMode(led3,OUTPUT);
  pinMode(led4,OUTPUT);
  if(mpu.testConnection())
  Serial.print("Ready");
  else Serial.print("Not detected");
  last=millis();
}
void low(){
  digitalWrite(led1,LOW);
  digitalWrite(led2,LOW);
  digitalWrite(led3,LOW);
  digitalWrite(led4,LOW);
}
void high(){
  digitalWrite(led1,HIGH);
  digitalWrite(led2,HIGH);
  digitalWrite(led3,HIGH);
  digitalWrite(led4,HIGH);
}
void loop() {

float dt =(millis()-last)/1000.0;
last =millis();


mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

float Ax = (float)ax;
float Ay = (float)ay;
float Az = (float)az;

float gyroRoll=gx/131.0;
float gyroPitch=gy/131.0;


float accPitch=atan2(-Ax,sqrt(Ay*Ay+Az*Az))*180/PI;
float accRoll = atan2(Ay, sqrt(Ax*Ax + Az*Az)) * 180.0 / PI;


pitch = 0.98*( pitch + gyroPitch *dt)+ 0.02*accPitch;
roll = 0.98*( roll + gyroRoll *dt)+ 0.02*accRoll;

if (abs(roll)>3&&abs(roll)<5){
  digitalWrite(led1,HIGH);
  digitalWrite(led2,LOW);
  digitalWrite(led3,LOW);
  digitalWrite(led4,LOW);
  state=0;
}
else if (abs(roll)>5&&abs(roll)<10){
  digitalWrite(led1,HIGH);
  digitalWrite(led2,HIGH);
  digitalWrite(led3,LOW);
  digitalWrite(led4,LOW);
  state=0;
}
else if (abs(roll)>10&&abs(roll)<15){
  digitalWrite(led1,HIGH);
  digitalWrite(led2,HIGH);
  digitalWrite(led3,HIGH);
  digitalWrite(led4,LOW);
  state=0;
}
else if (abs(roll)>15&&abs(roll)<40){

  if (state == 0){      // only when entering
    print = millis();   // start timer once
  }

  state = 1;
}
else{
  low();
  state=0;
}
if (state == 1){

  if (millis() - print > 200){
    print = millis();Serial.println("A7aaaa");
    
    if (digitalRead(led1) == HIGH)
      low();
    else
      high();
  }

}
Serial.println(roll);
}

