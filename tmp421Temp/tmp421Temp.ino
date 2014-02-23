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

/* Address (1 = VCC, 0 = GND, X = Floating)
 * A1 | A0 | Slave Address
 * X  | 0  | 001 1100 (0x1C)
 * X  | 1  | 001 1101 (0x1D)
 * 0  | X  | 001 1110 (0x1E)
 * 1  | X  | 001 1111 (0x1F)
 * X  | X  | 010 1010 (0x2A)
 * 0  | 0  | 100 1100 (0x4C)
 * 0  | 1  | 100 1101 (0x4D)
 * 1  | 0  | 100 1110 (0x4E)
 * 1  | 1  | 100 1111 (0x4F)
 */

#include "Wire.h"
#include <LibTemperature2.h>

LibTemperature2 temp = LibTemperature2(0x2A);
LibTemperature2 temp2 = LibTemperature2(0x1C);

void setup() {
  Serial.begin(57600);
//  pinMode(9,OUTPUT);
//  digitalWrite(9,HIGH);	// just turns on an "I'm alive" LED on pin 9, optional
}

void loop() {
 Serial.print(temp.GetTemperature());
 Serial.print(" / ");
 Serial.println(temp2.GetTemperature());
 delay(100);
}
