// This Arduino sketch reads DS18B20 "1-Wire" digital
// temperature sensors.
// Tutorial:
// http://www.hacktronics.com/Tutorials/arduino-1-wire-tutorial.html

#include <OneWire.h>
#include <DallasTemperature.h>
#include <PortsLCD.h>
#include <JeeLib.h>

PortI2C myI2C (4);
LiquidCrystalI2C lcd (myI2C);

#define screen_width 16
#define screen_height 2

// Data wire is plugged into pin 4 (jeenode DIO 1) on the Arduino
#define ONE_WIRE_BUS 4

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

struct {
    byte light;     // light sensor: 0..255
    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Assign the addresses of your 1-Wire temp sensors.
// See the tutorial on how to obtain these addresses:
// http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html

DeviceAddress insideThermometer = { 0x28, 0x1B, 0xE1, 0x22, 0x05, 0x00, 0x00, 0xE8 };
// DeviceAddress outsideThermometer = { 0x28, 0x6B, 0xDF, 0xDF, 0x02, 0x00, 0x00, 0xC0 };
//DeviceAddress dogHouseThermometer = { 0x28, 0x59, 0xBE, 0xDF, 0x02, 0x00, 0x00, 0x9F };

const int numReadings = 10;

float readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
float total = 0;                  // the running total
float average = 0;                // the average

void setup(void)
{
  // start serial port
  Serial.begin(57600);
  // Start up the library
  sensors.begin();
  // set the resolution to 10 bit (good enough?)
  sensors.setResolution(insideThermometer, 10);
 // sensors.setResolution(outsideThermometer, 10);
 // sensors.setResolution(dogHouseThermometer, 10);
 
 // set up the LCD's number of rows and columns: 
  lcd.begin(screen_width, screen_height);
  // Print a message to the LCD.
  lcd.clear();
  
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
  {
    readings[thisReading] = 0; 
  }
  
  // this is node 1 in net group 100 on the 868 MHz band
  rf12_initialize(1, RF12_915MHZ, 212);
}

void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  
  // subtract the last reading:
  total= total - readings[index];         
  // read from the sensor:  
  readings[index] = tempC; 
  // add the reading to the total:
  total= total + readings[index];       
  // advance to the next position in the array:  
  index = index + 1;                    

  // if we're at the end of the array...
  if (index >= numReadings)              
    // ...wrap around to the beginning: 
    index = 0;                           

  // calculate the average:
  average = total / numReadings;       
  
  if (tempC == -127.00) {
    Serial.print("Error getting temperature");
  } else {
    Serial.print("C: ");
    Serial.print(tempC);
    Serial.print(" F: ");
    Serial.print(DallasTemperature::toFahrenheit(tempC));
    Serial.print(" 10 sample Average: ");
    Serial.print(average);
    // todo: average value too
    lcd.setCursor(0,0);
    lcd.print(tempC);
    lcd.print("C");
    lcd.setCursor(0,1);
    lcd.print("10spl avg: ");
    lcd.print(average);
  }
  
  int tempTenths = tempC*10.0;
  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(0, &tempTenths, sizeof tempTenths);
  rf12_sendWait(RADIO_SYNC_MODE);
  rf12_sleep(RF12_SLEEP); 
}

void loop(void)
{   
  // Wait a minute between samples. This used to be once per second when I was using this to measure the lake temperature and something on the order of one second (maybe 5) would still be good for that appliction
  delay(60000);
  Serial.print("Getting temperatures...\n\r");
  sensors.requestTemperatures();
  
  Serial.print("Inside temperature is: ");
  printTemperature(insideThermometer);
  Serial.print("\n\r");
  /*
  Serial.print("Outside temperature is: ");
  printTemperature(outsideThermometer);
  Serial.print("\n\r");
  Serial.print("Dog House temperature is: ");
  printTemperature(dogHouseThermometer);
  Serial.print("\n\r\n\r");
  */
}

