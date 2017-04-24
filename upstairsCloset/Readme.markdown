# Jeenode living in the mom cave

Inspired by tempNode from [http://jeelabs.net/boards/7/topics/3898?r=3917#message-3917]()

* 2x ds18b20 temp sensors attached to port 1
* Battery level on port 4
* group: 212
* node: 3

# Packet format:

        struct {
            uint32_t packetseq;   	// packet serial number 32 bits 1..4294967295
            int sensor1;  		// temperature * 10 as 2 bytes
            int sensor2;             // temperature * 10 as 2 bytes 
            byte battVolts;  		// battery voltage in 100ths of a volt. max V = 3.33. Designed for use with AA board
            // Could use bandgap voltage as described here for other setups http://jeelabs.org/2012/05/04/measuring-vcc-via-the-bandgap/
        } payload;


#Hardware
* standard board stacked with carrier board. AA board with 6 wire connector, could have stacked if thought a bit more a head of time. Since the PWR pin is set to be battery voltage on the AA board, need to make sure to wire up the +3V pin from it to the carrier board.
* DS18B20s in normal mode
* HugoTemp port closest to resistor
