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
    int tempDS1820B;  		// temperature * 10 as 2 bytes
    int tempTMP421;             // temperature * 10 as 2 bytes 
    byte battVolts;  		// battery voltage in 100ths of a volt. max V = 3.33. Designed for use with AA board
    // Could use bandgap voltage as described here for other setups http://jeelabs.org/2012/05/04/measuring-vcc-via-the-bandgap/
} payload;

// because we're using the watchdog timer for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// send out a radio packet with our data payload
static byte sendPayload () {
  ++payload.packetseq;

  // not done until we get an ack or we have 8 unsuccessful resends
  bool ACKd = false;
  int resendCount = 8;
  while (!ACKd && resendCount > 0)
  {
      rf12_sleep(RF12_WAKEUP);
      while (!rf12_canSend())
      {
        rf12_recvDone();
      }
      // Ask for an ACK
#if DEBUG
      Serial.print("Sending, attempt ");
      Serial.println(9-resendCount);
      serialFlush();
#endif
      rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload);
      // This sendWait was here before we started waiting for ACKs. It would prevent us putting the radio to sleep before the send finished. That shouldn't ever happen now and it was preventing us from receiving ACKs.
      //rf12_sendWait(SEND_MODE);
      resendCount--;
#if DEBUG
      Serial.println("Waiting for ACK...");
      serialFlush();
#endif
      // poll for 100ms to wait for an ACK
      MilliTimer t;
      while (!t.poll(100)) {
        // got an empty packet intended for us 
        if (rf12_recvDone() && rf12_crc == 0 && rf12_len == 0) {
#if DEBUG 
              Serial.println("Possible ACK received");
              serialFlush();
#endif
          byte myAddr = SET_NODE & RF12_HDR_MASK;
          if (rf12_hdr == (RF12_HDR_CTL | RF12_HDR_DST | myAddr)) {
#if DEBUG 
              Serial.println("ACK received");
              serialFlush();
#endif
              ACKd = true;
          } else {
#if DEBUG 
              Serial.println("NO ACK");
              serialFlush();
#endif
          }
        }
        // TODO: put a 1ms or 2ms delay here to avoid burning power polling for 100ms?
      }
      // Go back into low power radio mode
      rf12_sleep(RF12_SLEEP);
      if (!ACKd)
      {
        // Wait 1s before sending again
        delay(1000);
      }
  }
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
    // we are using 12 bit resolution, which means the sensor returns temp in 1/16ths but the DallasTemperature library has already converted that for us to actual degrees C. 
    return (int)sensors.getTempC(DS18B20_ADDR)*10;
}

// Returns battery voltage as measured on analog pin of battPort. Assuming 3.3V VCC, this maps [0,3.3]V to a return value in [0,255]
static byte readBatt() {
	byte count = 4;
	int value;
	while (count-- > 0) {
		value = battPort.anaRead();
	}

    return  (byte) map(value,0,1023,0,330);
}

// readout all the sensors and other values
static void doMeasure() {
    
  payload.tempDS1820B = readTemp();
  payload.tempTMP421 = tmp421.GetTemperature() * 10;
  payload.battVolts = readBatt();
  sendPayload();
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
    sensors.setResolution(DS18B20_ADDR, 12);
    scheduler.timer(PREPTEMP, 0);    	// start the measurement loop going
}

void printPayload() {
    Serial.print("seq: ");
    Serial.println(payload.packetseq);
    Serial.print("DS1820B: ");
    Serial.println(payload.tempDS1820B);
    Serial.print("TMP421: ");
    Serial.println(payload.tempTMP421);
    Serial.print("Battery: ");
    Serial.println(payload.battVolts);
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

            #if DEBUG
                printPayload();
                serialFlush();
            #endif

            // schedule next measurement periodically
            scheduler.timer(PREPTEMP, MEASURE_INTERVAL);

            break;
		
	}	
}
