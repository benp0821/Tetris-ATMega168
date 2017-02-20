/*
pcf8574 lib 0x02

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "pcf8574.h"

//path to i2c fleury lib
#include "i2cmaster.h"

/*
 * initialize
 */
void pcf8574_init() {
	#if PCF8574_I2CINIT == 1
	//init i2c
	i2c_init();
	_delay_us(10);
	#endif

	//reset the pin status
	uint8_t i = 0;
	for(i=0; i<PCF8574_MAXDEVICES; i++)
		pcf8574_pinstatus[i] = 0;

}

/*
 * get output pin status
 */
int8_t pcf8574_getoutputpin(uint8_t deviceid, uint8_t pin) {
	int8_t data = -1;
	if((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES) && (pin >= 0 && pin < PCF8574_MAXPINS)) {
		data = pcf8574_pinstatus[deviceid];
		data = (data >> pin) & 0b00000001;
	}
	return data;
}

/*
 * set output pins
 */
int8_t pcf8574_setoutput(uint8_t deviceid, uint8_t data) {
	if((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES)) {
		pcf8574_pinstatus[deviceid] = data;
		i2c_start(((PCF8574_ADDRBASE+deviceid)<<1) | I2C_WRITE);
		i2c_write(data);
		i2c_stop();
		return 0;
	}
	return -1;
}


/*
 * set output pin
 */
int8_t pcf8574_setoutputpin(uint8_t deviceid, uint8_t pin, uint8_t data) {
	if((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES) && (pin >= 0 && pin < PCF8574_MAXPINS)) {
	    uint8_t b = 0;
	    b = pcf8574_pinstatus[deviceid];
	    b = (data != 0) ? (b | (1 << pin)) : (b & ~(1 << pin));
	    pcf8574_pinstatus[deviceid] = b;
	    //update device
		i2c_start(((PCF8574_ADDRBASE+deviceid)<<1) | I2C_WRITE);
		i2c_write(b);
		i2c_stop();
		return 0;
	}
	return -1;
}

/*
 * set output pin high
 */
int8_t pcf8574_setoutputpinhigh(uint8_t deviceid, uint8_t pin) {
	return pcf8574_setoutputpin(deviceid, pin, 1);
}

/*
 * set output pin low
 */
int8_t pcf8574_setoutputpinlow(uint8_t deviceid, uint8_t pin) {
	return pcf8574_setoutputpin(deviceid, pin, 0);
}