/// @dir bandgap
/// Try reading the bandgap reference voltage to measure current VCC voltage.
/// @see http://jeelabs.org/2012/05/12/improved-vcc-measurement/
// 2012-04-22 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <avr/sleep.h>

#define BATT_SENSE_PORT 2		// sense battery voltage on this port

volatile bool adcDone;

Port battPort (BATT_SENSE_PORT);

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

ISR(ADC_vect) { adcDone = true; }

static byte vccRead (byte count =4) {
  set_sleep_mode(SLEEP_MODE_ADC);
  ADMUX = bit(REFS0) | 14; // use VCC and internal bandgap
  bitSet(ADCSRA, ADIE);
  while (count-- > 0) {
    adcDone = false;
    while (!adcDone)
      sleep_mode();
  }
  bitClear(ADCSRA, ADIE);  
  // convert ADC readings to fit in one byte, i.e. 20 mV steps:
  //  1.0V = 0, 1.8V = 40, 3.3V = 115, 5.0V = 200, 6.0V = 250
  return (55U * 1023U) / (ADC + 1) - 50;
}

static byte readBat() {
	byte count = 4;
	int value;
	while (count-- > 0) {
		value = battPort.anaRead();
	}
    return  (byte) map(value,0,1013,0,330);
}

void setup() {
  rf12_initialize(17, RF12_915MHZ, 212);
}

void loop() {  
  byte x[2];
  x[0] = vccRead();
  Sleepy::loseSomeTime(1024);
  x[1] = readBat();

  Sleepy::loseSomeTime(16);

  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &x, 2);
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);

  Sleepy::loseSomeTime(1024);
}
