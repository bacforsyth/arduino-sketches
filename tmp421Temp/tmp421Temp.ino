/****************************
 * GetTwoTemperatures
 *  by Ted Hayes 2011
 *  www.tedbot.com
 *  modified by Ben Forsyth February 2014 to read a single temp
 *
 *  An example sketch that prints the
 *  temperatures from two sensors to the PC's serial port
 *
 *  Tested with two TMP421-Breakout
 *  Temperature Sensors from Modern Device
 *****************************/
#include "Wire.h"
#include <LibTemperature2.h>

LibTemperature2 temp = LibTemperature2(0x2A);
//LibTemperature2 temp2 = LibTemperature2(0x1C);

void setup() {
  Serial.begin(57600);
//  pinMode(9,OUTPUT);
//  digitalWrite(9,HIGH);	// just turns on an "I'm alive" LED on pin 9, optional
}

void loop() {
 Serial.println(temp.GetTemperature());
// Serial.print(" / ");
// Serial.println(temp2.GetTemperature());
 delay(100);
}
