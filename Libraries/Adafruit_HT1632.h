#include <stdint.h>
#include <avr/io.h>

#ifndef ADA_HT1632_H_
#define ADA_HT1632_H_

#define ADA_HT1632_READ  0x6
#define ADA_HT1632_WRITE 0x5
#define ADA_HT1632_COMMAND 0x4

#define ADA_HT1632_SYS_DIS 0x00
#define ADA_HT1632_SYS_EN 0x01
#define ADA_HT1632_LED_OFF 0x02
#define ADA_HT1632_LED_ON 0x03
#define ADA_HT1632_BLINK_OFF 0x08
#define ADA_HT1632_BLINK_ON 0x09
#define ADA_HT1632_SLAVE_MODE 0x10
#define ADA_HT1632_MASTER_MODE 0x14
#define ADA_HT1632_INT_RC 0x18
#define ADA_HT1632_EXT_CLK 0x1C
#define ADA_HT1632_PWM_CONTROL 0xA0

#define ADA_HT1632_COMMON_8NMOS  0x20
#define ADA_HT1632_COMMON_16NMOS  0x24
#define ADA_HT1632_COMMON_8PMOS  0x28
#define ADA_HT1632_COMMON_16PMOS  0x2C

int8_t _data, _cs, _wr;
uint8_t ledmatrix[48]; //48 because (24*16)/8 = 48. Each 8 bit entry in the array controls 8 LEDs (half a row)

void writeMatrixData(uint16_t d, uint8_t bits);
void sendMatrixCommand(uint8_t c);
void beginMatrix(int8_t data, int8_t wr, int8_t cs);
void setMatrixBrightness(uint8_t pwm);
void setMatrixPixel(int16_t i);
int checkMatrixPixel(int16_t i);
void clrMatrixPixel(int16_t i);
void writeMatrixScreen();
void clearMatrixScreen();
void fillMatrixScreen();   

#endif /* Adafruit HT1632_H_ */
