// from http://jeelabs.net/boards/7/topics/3898?r=3917#message-3917
// tempNode V4
// Simple Temperature Node, using a single DS18B20
// assumes powered by AA power board 
// Damon Bound 2012-08-14

// TODO(bf)
// use DallasTemperature library, see WaitForConversion examples for async reading
// Read from i2c for the TMP421. Bit bang with jeelib i2c or use proper i2c pins?

#include <JeeLib.h>
#include <avr/sleep.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Wire.h"
#include <LibTemperature2.h>


#define DEBUG   0   			// set to 1 to trace activity via serial console

#define SET_NODE   		2  		// wireless node ID 
#define SET_GROUP  		212   	// wireless net group 
#define SEND_MODE  		2   	// set to 3 if fuses are e=06/h=DE/l=CE, else set to 2

#define ONEWIRE_PIN 	4   	// on arduino digital pin 4 = jeenode port 1 DIO line
#define BATT_SENSE_PORT 2		// sense battery voltage on this port

#define PREPTEMP_DELAY  8    	// how long to wait for DS18B20 sensor to stabilise, in tenths of seconds
#define MEASURE_INTERVAL  592 	// how long between measurements, in tenths of seconds
								// 592 + 8 = 600 i.e. 1 minute

DeviceAddress DS18B20_ADDR = { 0x28, 0x1B, 0xE1, 0x22, 0x05, 0x00, 0x00, 0xE8 };

#if DEBUG
static const char signature[14] = "[tempNodeV4]";
#endif
								
// define OneWire object for our DS18B20 temperature sensor
OneWire ds(ONEWIRE_PIN);  
DallasTemperature sensors(&ds);

// define JeeLib Port for the battery reading
Port battPort (BATT_SENSE_PORT);

// TMP421 sensor attached to I2C pins directly
LibTemperature2 tmp421 = LibTemperature2(0x2A);


// The JeeLib scheduler makes it easy to perform various tasks at various times:
enum { PREPTEMP, MEASUREALL, TASK_END };
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// This defines the structure of the packets which get sent out by wireless:
struct {
    uint32_t packetseq;   	// packet serial number 32 bits 1..4294967295
    int tempDS1820B;  		// temperature * 16 as 2 bytes
    int tempTMP421;             // temperature * 16 as 2 bytes 
    byte battVolts;  		// battery voltage in 100ths of a volt. max V = 3.33. Designed for use with AA board
    // Could use bandgap voltage as described here for other setups http://jeelabs.org/2012/05/04/measuring-vcc-via-the-bandgap/
} payload;

// because we're using the watchdog timer for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// send out a radio packet with our data payload
static byte sendPayload () {
  ++payload.packetseq;

  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload);
  rf12_sendWait(SEND_MODE);
  rf12_sleep(RF12_SLEEP);
}

// prepare to read out temperature sensor
static void prepTemp() {
	// starts a temperature measurement cycle
	// then there needs to be a delay for DS18B20 to do its thing 
	// at least 750ms @ 12-bit resolution, we actually wait 8 tenths = 800 ms
	// the DS18B20 automatically goes into low power standby after its conversion is done
    sensors.requestTemperatures();
}

static int readTemp() {
    // read the temperature back from the DS18B20 that we kicked off a little while ago
    // we are using 12 bit resolution, which means the sensor returns temp in 1/16ths but the DallasTemperature library has already converted that for us to actual degrees C. This code had assumed it was still in 16ths, keep it that way for now.
    return (int)sensors.getTempC(DS18B20_ADDR)*16;
}

static byte readBatt() {
	byte count = 4;
	int value;
	while (count-- > 0) {
		value = battPort.anaRead();
	}
    return  (byte) map(value,0,1013,0,330);
}

// readout all the sensors and other values
static void doMeasure() {
    
  payload.tempDS1820B = readTemp();
  payload.tempTMP421 = tmp421.GetTemperature() * 16;
  payload.battVolts = readBatt();
  sendPayload();

    #if SERIAL
        dumpPayload("doMeasure()");
    #endif
}

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
		delay(2); // make sure tx buf is empty before going back to sleep
    #endif  
}

void setup() {
	rf12_initialize(SET_NODE, RF12_915MHZ, SET_GROUP);
	rf12_sleep(RF12_SLEEP);
	
    #if DEBUG
        Serial.begin(57600);
        Serial.print(signature);
        serialFlush();
    #endif
	
    sensors.setWaitForConversion(false);  // makes it async
    sensors.setResolution(DS18B20_ADDR, 10);
    scheduler.timer(PREPTEMP, 0);    	// start the measurement loop going
}

void loop() {

	// call to scheduler.pollWaiting() waits in sleep mode until
	// the next event is due
    switch (scheduler.pollWaiting()) {

        case PREPTEMP:
           #if DEBUG
               Serial.println("action=prep temp");
               serialFlush();
           #endif

           prepTemp();

           // schedule next action after the appropriate delay
           scheduler.timer(MEASUREALL, PREPTEMP_DELAY);

           break;

        case MEASUREALL:
            #if DEBUG
                Serial.println("action=measureall");
                serialFlush();
            #endif
    
            doMeasure();

            // schedule next measurement periodically
            scheduler.timer(PREPTEMP, MEASURE_INTERVAL);

            break;
		
	}	
}
