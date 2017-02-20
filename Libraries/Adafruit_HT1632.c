#include "Adafruit_HT1632.h"
#define ht1632swap(a, b) { uint16_t t = a; a = b; b = t; }

void writeMatrixData(uint16_t d, uint8_t bits) {
  DDRD |= (1 << _data);//pinMode(_data, OUTPUT);
  for (uint8_t i=bits; i > 0; i--) {
    PORTD &= ~(1 << _wr);//digitalWrite(_wr, LOW);
   if (d & _BV(i-1)) {
     PORTD |= (1 << _data);//digitalWrite(_data, HIGH);
   } else {
     PORTD &= ~(1 << _data);//digitalWrite(_data, LOW);
   }
  PORTD |= (1 << _wr);//digitalWrite(_wr, HIGH);
  }
  DDRD &= ~(1 << _data);//pinMode(_data, INPUT);
}

void sendMatrixCommand(uint8_t cmd) {
  uint16_t data = 0;
  data = ADA_HT1632_COMMAND;
  data <<= 8;
  data |= cmd;
  data <<= 1;
  
  PORTD &= ~(1 << _cs);//digitalWrite(_cs, LOW);
  writeMatrixData(data, 12);
  PORTD |= (1 << _cs);//digitalWrite(_cs, HIGH); 
}

void beginMatrix(int8_t data, int8_t wr, int8_t cs) {
  _data = data;
  _wr = wr;
  _cs = cs;

  for (uint8_t i=0; i<48; i++) {
    ledmatrix[i] = 0;
  }
  
  DDRD |= (1 << _cs);//pinMode(_cs, OUTPUT);
  PORTD |= (1 << _cs);//digitalWrite(_cs, HIGH);
  DDRD |= (1 << _wr);//pinMode(_wr, OUTPUT);
  PORTD |= (1 << _wr);//digitalWrite(_wr, HIGH);
  DDRD |= (1 << _data);//pinMode(_data, OUTPUT);

  sendMatrixCommand(ADA_HT1632_SYS_EN);
  sendMatrixCommand(ADA_HT1632_LED_ON);
  sendMatrixCommand(ADA_HT1632_BLINK_OFF);
  sendMatrixCommand(ADA_HT1632_MASTER_MODE); 
  sendMatrixCommand(ADA_HT1632_INT_RC);
  sendMatrixCommand(ADA_HT1632_COMMON_16NMOS);
  sendMatrixCommand(ADA_HT1632_PWM_CONTROL | 0xF);
}

void setMatrixBrightness(uint8_t pwm) {
  if (pwm > 15) pwm = 15;
  sendMatrixCommand(ADA_HT1632_PWM_CONTROL | pwm);
}

void setMatrixPixel(int16_t i) {
  if (i >= 0)
	ledmatrix[i/8] |= _BV(i%8); 
}

int checkMatrixPixel(int16_t i) {
  int change = 0;
  int val = 0; 
  if (i >= 0)
	val = ledmatrix[i/8] | _BV(i%8); 
  if (val != ledmatrix[i/8]){
	  change = 1;
  }
  return change;
}

void clrMatrixPixel(int16_t i) {
  if (i >= 0)
    ledmatrix[i/8] &= ~_BV(i%8); 
}

void writeMatrixScreen() {
  PORTD &= ~(1 << _cs);//digitalWrite(_cs, LOW);

  writeMatrixData(ADA_HT1632_WRITE, 3);
  // send with address 0
  writeMatrixData(0, 7);

  for (uint16_t i=0; i<(24*16/8); i+=2) {
    uint16_t d = ledmatrix[i];
    d <<= 8;
    d |= ledmatrix[i+1];

    writeMatrixData(d, 16);
  }
  PORTD |= (1 << _cs);//digitalWrite(_cs, HIGH);
}


void clearMatrixScreen() {
  for (uint8_t i=0; i<(24*16/8); i++) {
    ledmatrix[i] = 0;
  }
  writeMatrixScreen();
}

void fillMatrixScreen() {
  for (uint8_t i=0; i<(24*16/8); i++) {
    ledmatrix[i] = 0xFF;
  }
  writeMatrixScreen();
}