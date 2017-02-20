#ifndef _I2CMASTER_H
#define _I2CMASTER_H   1
/************************************************************************* 
* Title:    C include file for the I2C master interface 
*           (i2cmaster.S or twimaster.c)
* Author:   Peter Fleury <pfleury@gmx.ch>  http://jump.to/fleury
* File:     $Id: i2cmaster.h,v 1.10 2005/03/06 22:39:57 Peter Exp $
* Software: AVR-GCC 3.4.3 / avr-libc 1.2.3
* Target:   any AVR device
* Usage:    see Doxygen manual
**************************************************************************/

/**
 @defgroup pfleury_ic2master I2C Master library
 @code #include <i2cmaster.h> @endcode
  
 @brief I2C (TWI) Master Software Library
 
 Basic routines for communicating with I2C slave devices. This single master 
 implementation is limited to one bus master on the I2C bus. 

 This I2c library is implemented as a compact assembler software implementation of the I2C protocol 
 which runs on any AVR (i2cmaster.S) and as a TWI hardware interface for all AVR with built-in TWI hardware (twimaster.c).
 Since the API for these two implementations is exactly the same, an application can be linked either against the
 software I2C implementation or the hardware I2C implementation.

 Use 4.7k pull-up resistor on the SDA and SCL pin.
 
 Adapt the SCL and SDA port and pin definitions and eventually the delay routine in the module 
 i2cmaster.S to your target when using the software I2C implementation ! 
 
 Adjust the  CPU clock frequence F_CPU in twimaster.c or in the Makfile when using the TWI hardware implementaion.

 @note 
    The module i2cmaster.S is based on the Atmel Application Note AVR300, corrected and adapted 
    to GNU assembler and AVR-GCC C call interface.
    Replaced the incorrect quarter period delays found in AVR300 with 
    half period delays. 
    
 @author Peter Fleury pfleury@gmx.ch  http://jump.to/fleury

*/

#include <avr/io.h>

/** defines the data direction (reading from I2C device) in i2c_start(),i2c_rep_start() */
#define I2C_READ    1

/** defines the data direction (writing to I2C device) in i2c_start(),i2c_rep_start() */
#define I2C_WRITE   0

extern void i2c_init(void);
extern void i2c_stop(void);
extern unsigned char i2c_start(unsigned char addr);
extern unsigned char i2c_write(unsigned char data);

#endif
