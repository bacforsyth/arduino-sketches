# Jeenode living in the mom cave

Inspired by tempNode from [http://jeelabs.net/boards/7/topics/3898?r=3917#message-3917]()

* ds18b20 temp sensor attached to port 4 measuring outside temperature
* tmp421, address 0x2A, attached to I2C port directly
* Battery level on port 2
* group: 212
* node: 2

packet format:

        struct {
            uint32_t packetseq;   	// packet serial number 32 bits 1..4294967295
            int tempDS1820B;  		// temperature * 16 as 2 bytes
            int tempTMP421;         // temperature * 16 as 2 bytes 
            byte battVolts;  		// battery voltage in 100ths of a volt. max V = 3.33
        } payload;


#Issues

- If i2c temp sensor isn't attached then it appears we hang forever when trying to read from it
