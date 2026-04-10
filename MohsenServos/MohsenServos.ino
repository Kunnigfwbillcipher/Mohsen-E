#include <Wire.h>
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
//sense i got different servos so i should tone every kind of the 3 kinds if there were any problem by changing the min and max pulse
void leftEye    (int angle)  { channel(0,  angle); }
void rightEye   (int angle)  { channel(1,  angle); }
void leftjoint  (int angle)  { channel(2,  angle); }
void rightjoint (int angle)  { channel(3,  angle); }
void leftRight  (int angle)  { channel(4,  angle); } // To look left and right and might be tilting so i dunno i would leavt it for now
void botNeck    (int angle)  { channel(5,  angle); }
void topNeck    (int angle)  { channel(6,  angle); }
void leftArm    (int angle)  { channel(7,  angle); }
void rightArm   (int angle)  { channel(8,  angle); }
void leftGrip   (int angle)  { channel(9,  angle); }
void rightGrip  (int angle)  { channel(10, angle); }
void door       (int angle)  { channel(11, angle); }
/* by using combination of those i should make him do 
1- sus
2- i got an idea 
3- intersting
4- afraid 
5- sad 
6- angry
*/
void setup() {


}

void loop() {

}
