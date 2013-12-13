// Controlling a servo position using a potentiometer (variable resistor) 
// by Michal Rinott <http://people.interaction-ivrea.it/m.rinott> 

#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
Servo myservo2;  // create servo object to control a servo 
 
int potpin = 1;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog pin 
 
void setup() 
{ 
  myservo.attach(5);  // attaches the servo to DIO port 2
  myservo2.attach(6);  // attaches the servo to DIO port 2
  Serial.begin(57600);
  Serial.println("Ready");
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);  
} 
 
void loop() 
{ 
  val = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023) 
  val = map(val, 0, 1023, 0, 90);     // scale it to use it with the servo (value between 0 and 180) 
  if (val > 90)
    val = 90;
  Serial.println(val);
  myservo.write(val);                  // sets the servo position according to the scaled value 
  myservo2.write(180-val);                  // sets the servo position according to the scaled value 
  delay(15);                           // waits for the servo to get there 
} 
