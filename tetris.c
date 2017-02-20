#include "Adafruit_HT1632.h"
#include "lcdpcf8574.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>  
#include <stdlib.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <string.h>
   
#define HT_DATA PD5
#define HT_WR PD6
#define HT_CS PD7
#define RGT_ROT_BUT PC0
#define LFT_ROT_BUT PC1
#define FALL_BUT PC2
#define RST_BUT PC3
#define LFT_BUT PD2
#define RGT_BUT PD3
#define UART_BAUD_RATE 2400

static volatile int leftPressed;
static volatile int rightPressed;
static volatile int fallFast;
static volatile int restart;
static volatile int rotateLeftPressed;
static volatile int rotateRightPressed;
static uint32_t highScore1, highScore2, highScore3;
static char highScoreName1[4], highScoreName2[4], highScoreName3[4];

/*
Helps getCorrectPosition get the correct coordinate
*/
int coordinateHelper(int rowWidth, int val, int start, int end, int objective){
	int coord = -1;
	int value = val;
	for (int j = start; j < end; j+=16){
		if (objective >= j && objective < j+8){
			coord = value - (rowWidth*(objective-j));
		}
		value--;
	}
	
	return coord;
}

/*
Takes in a reasonable coordinate and returns the stupid coordinate that the matrix needs
*/
int getCorrectPosition(int i){
	int correctCoord = -1;
	int rowWidth = 16;
	
	if (i < 0 || i >= 384){correctCoord = i;}
	if (correctCoord == -1){
		correctCoord = coordinateHelper(rowWidth, 127, 0, 120, i);
	}
	if (correctCoord == -1){
		correctCoord = coordinateHelper(rowWidth, 119, 8, 128, i);
	}
	if (correctCoord == -1){
		correctCoord = coordinateHelper(rowWidth, 255, 128, 248, i);
	}
	if (correctCoord == -1){
		correctCoord = coordinateHelper(rowWidth, 247, 136, 256, i);
	}
	if (correctCoord == -1){
		correctCoord = coordinateHelper(rowWidth, 383, 256, 376, i);
	}
	if (correctCoord == -1){
		correctCoord = coordinateHelper(rowWidth, 375, 264, 384, i);
	}
	return correctCoord;
}

/*
Draws the border on the right side of the matrix display
*/
void drawBorders(){
	for (int i = 6; i < 24; i++){
		setMatrixPixel(getCorrectPosition(11+(16*i)));
		setMatrixPixel(getCorrectPosition(12+(16*i)));
		setMatrixPixel(getCorrectPosition(13+(16*i)));
		setMatrixPixel(getCorrectPosition(14+(16*i)));
		writeMatrixScreen();
	}
	
	for (int i = 0; i < 24; i++){
		setMatrixPixel(getCorrectPosition(10+(16*i)));
		setMatrixPixel(getCorrectPosition(15+(16*i)));
		writeMatrixScreen();
	}
	
	for (int i = 11; i < 15; i++){
		setMatrixPixel(getCorrectPosition(i));
		writeMatrixScreen();
	}
	
}

/*
Seeds the random number generator for the block spawning using an incremented counter stored in the EEPROM
*/
void initRand(){
	uint16_t state = eeprom_read_word((uint16_t*)0);
	
	//if nothing written to EEPROM
	if (state == 0xFFFF){
		state = 0;
	}
	srand(state);
	eeprom_write_word((uint16_t*)0, ++state);
}

/*
Given a start address and a character pointer, this function
reads in a sequence of 3 characters and puts them in the character pointer
*/
void readInHighScoreName(int startingAdd, char* hsNameArray){
	int counter = 0;
	for (int i = startingAdd; i <= (startingAdd+16); i+=8){
		uint8_t temp = eeprom_read_byte((uint8_t*)i);
		if (temp >= 65 && temp <= 90){
			hsNameArray[counter] = temp;
		}else{
			hsNameArray[counter] = 'A';
		}
		counter++;
	}
	hsNameArray[3] = '\0';
}

/*
Given an address, this method reads in an integer and stores it in hsValue
*/
void readInHighScoreValue(int address, uint32_t* hsValue){
	*hsValue = eeprom_read_dword((uint32_t*)address);
	if (*hsValue > 999999999){
		*hsValue = 0;
	}
}

/*
Gets the high scores and displays them on the LCD display
*/
void displayHighScores(){
	readInHighScoreValue(16, &highScore1);
	readInHighScoreValue(48, &highScore2);
	readInHighScoreValue(80, &highScore3);
	
	readInHighScoreName(112, highScoreName1);
	readInHighScoreName(136, highScoreName2);
	readInHighScoreName(160, highScoreName3);
	
	lcd_gotoxy(0, 1);
	lcd_puts(" High:");
	
	char buf[10];
	snprintf(buf, 10, "%ld", (long)highScore1);
	lcd_gotoxy(11, 1);
	lcd_puts(buf);
	
	snprintf(buf, 10, "%ld", (long)highScore2);
	lcd_gotoxy(11, 2);
	lcd_puts(buf);
	
	snprintf(buf, 10, "%ld", (long)highScore3);
	lcd_gotoxy(11, 3);
	lcd_puts(buf);
	
	lcd_gotoxy(7, 1);
	lcd_puts(highScoreName1);
	
	lcd_gotoxy(7, 2);
	lcd_puts(highScoreName2);
	
	lcd_gotoxy(7, 3);
	lcd_puts(highScoreName3);
}

/*
Displays the score on the LCD display
*/
void updateScoreDisp(uint32_t score){
	char buf[10];
	
	snprintf(buf, 10, "%ld", (long)score);
	
	lcd_gotoxy(0, 0);
	lcd_puts("Score:              ");
	lcd_gotoxy(7, 0);
	lcd_puts(buf);
}

/*
Displays the level on the LCD display
*/
void updateLevelDisp(int level){
	char buf[4];
	
	snprintf(buf, 4, "%d", level);
	
	lcd_gotoxy(0, 4);
	lcd_puts("LVL   ");
	lcd_gotoxy(4, 4);
	lcd_puts(buf);
}

/*
Initializes the LCD display for use
*/
void initScoreLCD(){
	lcd_init(LCD_DISP_ON);
	lcd_home();
	lcd_led(0);
}

/*
Initializes everything
*/
void setup() {
	leftPressed = 0;
	rightPressed = 0;
	fallFast = 0;
	rotateLeftPressed = 0;
	rotateRightPressed = 0;
	
	PORTD |= (1 << LFT_BUT);	//enables pull up resistor on PD2
	PORTD |= (1 << RGT_BUT);	//enables pull up resistor on PD3
	EIMSK |= (1 << INT0); //enable interrupt 0 (EIMSK reg controls whether interrupts are enabled)
	EIMSK |= (1 << INT1); //enable interrupt 1 (EIMSK reg controls whether interrupts are enabled)
	EICRA |= (1 << ISC00); //any logical change
	EICRA |= (1 << ISC10); //any logical change
	
	PORTC |= (1 << RST_BUT);	//enables pull up resistor on PC5
	PORTC |= (1 << FALL_BUT);	//enables pull up resistor on PC4
	PORTC |= (1 << LFT_ROT_BUT); //enables pull up resistor on PC3
	PORTC |= (1 << RGT_ROT_BUT); //enables pull up resistor on PC2
	PCICR |= (1 << PCIE1);      //enables pin change interrupt for C pins
	PCMSK1 |= (1 << RST_BUT);   //enable interrupt on PC5
	PCMSK1 |= (1 << FALL_BUT);  //enable interrupt on PC4
	PCMSK1 |= (1 << LFT_ROT_BUT);  //enable interrupt on PC3
	PCMSK1 |= (1 << RGT_ROT_BUT);  //enable interrupt on PC2
	
	//Set up matrix display
	beginMatrix(HT_DATA, HT_WR, HT_CS);
	setMatrixBrightness(1); 
	_delay_ms(100);
	clearMatrixScreen();
	drawBorders(); 
	
	//initialize lcd values
	updateLevelDisp(1);
	updateScoreDisp(0);
	displayHighScores();
	
	sei();
	
	initRand();
	
	restart = 0;
} 

/*
Takes an integer representation of a block, a block position,
and a rotation. Returns an array with a starting position and 3 numbers
that are added to the start position to form the proper block shape
*/
int* getOutline(int block, int pos, int rotation){
	static int info[4];
	if (block == 0){ //O
		info[0] = pos;
		info[1] =  1;
		info[2] = 16;
		info[3] = 17;
	}else if (block == 1){ //I
		if (rotation == 0){
			info[0] = pos; info[1] =  16; info[2] = 32; info[3] = 48;
		}else if (rotation == 1){
			info[0] = pos+15; info[1] = 1; info[2] = 2; info[3] = 3;
		}else if (rotation == 2){
			info[0] = pos+1; info[1] = 16; info[2] = 32; info[3] = 48;
		}else {
			info[0] = pos+31; info[1] = 1; info[2] = 2; info[3] = 3;
		}
	}else if (block == 2){ //S
		if (rotation == 0){
			info[0] = pos+1; info[1] =  1; info[2] = 16; info[3] = 15;
		}else if (rotation == 1){
			info[0] = pos+1; info[1] = 17; info[2] = 16; info[3] = 33;
		}else if (rotation == 2){
			info[0] = pos+1+16; info[1] = 1; info[2] = 16; info[3] = 15;
		}else{
			info[0] = pos; info[1] = 17; info[2] = 16; info[3] = 33;
		}
	}else if (block == 3){ //Z
		if (rotation == 0){
			info[0] = pos; info[1] =  1; info[2] = 17; info[3] = 18;
		}else if (rotation == 1){
			info[0] = pos+2; info[1] =  15; info[2] = 16; info[3] = 31;
		}else if (rotation == 2){
			info[0] = pos+16; info[1] =  1; info[2] = 17; info[3] = 18;
		}else{
			info[0] = pos+1; info[1] =  15; info[2] = 16; info[3] = 31;
		}
	}else if (block == 4){ //L
		if (rotation == 0){
			info[0] = pos; info[1] =  16; info[2] = 32; info[3] = 33;
		}else if (rotation == 1){
			info[0] = pos+15; info[1] =  1; info[2] = 2; info[3] = 16;
		}else if (rotation == 2){
			info[0] = pos-1; info[1] =  1; info[2] = 17; info[3] = 33;
		}else{
			info[0] = pos+1; info[1] =  14; info[2] = 15; info[3] = 16;
		}
	}else if (block == 5){ //J
		if (rotation == 0){
			info[0] = pos+1; info[1] =  16; info[2] = 32; info[3] = 31;
		}else if (rotation == 1){
			info[0] = pos; info[1] =  16; info[2] = 17; info[3] = 18;
		}else if (rotation == 2){
			info[0] = pos+1; info[1] =  1; info[2] = 16; info[3] = 32;
		}else{
			info[0] = pos+16; info[1] =  1; info[2] = 2; info[3] = 18;
		}
	}else if (block == 6){ //T
		if (rotation == 0){
			info[0] = pos; info[1] =  1; info[2] = 2; info[3] = 17;
		}else if (rotation == 1){
			info[0] = pos-15; info[1] =  15; info[2] = 16; info[3] = 32;
		}else if (rotation == 2){
			info[0] = pos-15; info[1] =  15; info[2] = 16; info[3] = 17;
		}else{
			info[0] = pos-15; info[1] =  16; info[2] = 17; info[3] = 32;
		}
	}			
	return info;
}

/*
Checks for collision using block data to see if the block intersects another block or obstacle
*/
int checkCollision(int block, int pos, int rotation){
	int collision = 1;
	int change = 0;
	
	int* info = getOutline(block, pos, rotation);
	
	change = checkMatrixPixel(getCorrectPosition(info[0]));
	change += checkMatrixPixel(getCorrectPosition(info[0]+info[1]));
	change += checkMatrixPixel(getCorrectPosition(info[0]+info[2]));
	change += checkMatrixPixel(getCorrectPosition(info[0]+info[3]));
	
	if (change == 4){
		collision = 0;
	}
	return collision;
}

/*
Draws a block at a certain position and rotation.
Fails if there is a collision at the new position.
*/
int drawBlock(int block, int pos, int rotation){
	int* info = getOutline(block, pos, rotation);
	
	//finds max coordinate on block
	int max12 = (info[1] > info[2] ? info[1]:info[2]);
	int max = (max12 > info[3] ? max12:info[3]);
	
	int success = 0;
	
	if (!checkCollision(block, pos, rotation) && getCorrectPosition(info[0]+max) < 384){
		setMatrixPixel(getCorrectPosition(info[0]));
		setMatrixPixel(getCorrectPosition(info[0]+info[1]));
		setMatrixPixel(getCorrectPosition(info[0]+info[2]));
		setMatrixPixel(getCorrectPosition(info[0]+info[3]));
		success = 1;
	} 
	return success;
}

/*
Clears a block from the matrix given the block data
*/
void clearBlock(int block, int pos, int rotation){
	int* info = getOutline(block, pos, rotation);
	
	clrMatrixPixel(getCorrectPosition(info[0]));
	clrMatrixPixel(getCorrectPosition(info[0]+info[1]));
	clrMatrixPixel(getCorrectPosition(info[0]+info[2]));
	clrMatrixPixel(getCorrectPosition(info[0]+info[3]));
}

/*
Draws the next shape in the preview box
*/
void drawPreview(int nextShape){
	int* info = getOutline(nextShape, 44, 0);
	
	if (nextShape == 2 || nextShape == 3 || nextShape == 6){
		info[0]-=1;
	}
	setMatrixPixel(getCorrectPosition(info[0]));
	setMatrixPixel(getCorrectPosition(info[0]+info[1]));
	setMatrixPixel(getCorrectPosition(info[0]+info[2]));
	setMatrixPixel(getCorrectPosition(info[0]+info[3]));
}

/*
Clears the block from the preview box
*/
void clearPreview(){
	for (int i = 1; i < 6; i++){
		clrMatrixPixel(getCorrectPosition(11+(i*16)));
		clrMatrixPixel(getCorrectPosition(12+(i*16)));
		clrMatrixPixel(getCorrectPosition(13+(i*16)));
		clrMatrixPixel(getCorrectPosition(14+(i*16)));
	}
}

/*
Interrupt sets the flag for left movement
*/
ISR(INT0_vect){
	if ((PIND & (1 << LFT_BUT)) == 0){
		leftPressed = 1;
	}
}

/*
Interrupt sets the flag for right movement
*/
ISR(INT1_vect){
	if ((PIND & (1 << RGT_BUT)) == 0){
		rightPressed = 1;
	}
}

/*
Interrupt sets the flags for fast fall, restart, rotate left, and rotate right movement
*/
ISR(PCINT1_vect){
	if ((PINC & (1 << FALL_BUT)) == 0){
		fallFast = 1;
	}
	if ((PINC & (1 << RST_BUT)) == 0){
		restart = 1;
	}
	if ((PINC & (1 << LFT_ROT_BUT)) == 0){
		rotateLeftPressed = 1;
	}
	if ((PINC & (1 << RGT_ROT_BUT)) == 0){
		rotateRightPressed = 1;
	}
}

/*
Attempts to increment pos by 16, returns 1 if there was a collision (moving down failed)
*/
int moveDown(int block, int rotation, int *pos){
	int collision = 0;
	clearBlock(block, *pos, rotation);
	*pos += 16;
	if (!drawBlock(block, *pos, rotation)){
		*pos -= 16;
		drawBlock(block, *pos, rotation);
		collision = 1;
	}
	return collision;
}

/*
Attempts to move block left
*/
void moveLeft(int block, int rotation, int *pos){
	int leftEdge = 0;
	while (leftPressed && !leftEdge && *pos > 0){
		clearBlock(block, *pos, rotation);
		(*pos)--;
		if (!drawBlock(block, *pos, rotation)){
			(*pos)++;
			leftEdge = 1;
		}
		drawBlock(block, *pos, rotation);
		leftPressed = 0;
		if (((PIND & (1 << LFT_BUT)) == 0) && !leftEdge){
			leftPressed = 1;
			_delay_ms(60);
		}
		writeMatrixScreen();
	}
}

/*
Attempts to move block right
*/
void moveRight(int block, int rotation, int *pos){
	int rightEdge = 0;
	while (rightPressed && !rightEdge && *pos > 0){
		clearBlock(block, *pos, rotation);
		(*pos)++;
		if (!drawBlock(block, *pos, rotation)){
			(*pos)--;
			rightEdge = 1;
		}
		drawBlock(block, *pos, rotation);
		rightPressed = 0;
		if (((PIND & (1 << RGT_BUT)) == 0) && !rightEdge){
			rightPressed = 1;
			_delay_ms(60);
		}
		writeMatrixScreen();
	}
}

/*
makes the block fall fast
*/
void quickFall(int block, int rotation, int *pos){
	if (fallFast){
		while (!moveDown(block, rotation, pos) && fallFast){
			writeMatrixScreen();
			if ((PINC & (1 << FALL_BUT)) != 0){
				fallFast = 0;
			}
		}
		fallFast = 0;
		writeMatrixScreen();
		_delay_ms(100);
	}
}

/*
Attempts to rotate block right
*/
void rotateRight(int block, int pos, int *rotation){
	if (rotateRightPressed){
		int tempRot = *rotation;
		clearBlock(block, pos, *rotation);
		if (*rotation <= 0){
			*rotation = 3;
		}else{
			(*rotation)--;
		}
		
		if (!drawBlock(block, pos, *rotation)){
			*rotation = tempRot;
		}
		rotateRightPressed = 0;
		_delay_ms(100);
	}
}

/*
Attempts to rotate block left
*/
void rotateLeft(int block, int pos, int *rotation){
	if (rotateLeftPressed){
		int tempRot = *rotation;
		clearBlock(block, pos, *rotation);
		if (*rotation >= 3){
			*rotation = 0;
		}else{
			(*rotation)++;
		}
		
		if (!drawBlock(block, pos, *rotation)){
			*rotation = tempRot;
		}
		rotateLeftPressed = 0;
		_delay_ms(100);
	}
}

/*
Controls all button presses except the reset button
*/
void checkButtonInput(int block, int *pos, int *rotation){
	moveLeft(block, *rotation, pos);
	moveRight(block, *rotation, pos);
	quickFall(block, *rotation, pos);
	rotateRight(block, *pos, rotation);
	rotateLeft(block, *pos, rotation);
}

/*
Checks for and removes completely filled in rows,
moving the above blocks down by the number of rows removed.
Also updates the score based on the number of rows removed at once
*/
uint32_t handleFullRow(){
	uint32_t newScore = 0;
	for (int j = 0; j < 24; j++){
		int counter = 0;
		for (int i = 0; i < 10; i++){
			counter += checkMatrixPixel(getCorrectPosition((16*j)+i));
		}
		if (counter == 0){
			for (int i = 0; i < 10; i++){
				clrMatrixPixel(getCorrectPosition((16*j)+i));
				for (int k = j-1; k > 0; k--){
					if (checkMatrixPixel(getCorrectPosition((16*k)+i)) == 0){
						clrMatrixPixel(getCorrectPosition((16*k)+i));
						setMatrixPixel(getCorrectPosition((16*k)+i+16));
					}
				}
			}
			
			if (newScore == 0){
				newScore = 20;
			}else{
				newScore = newScore+(newScore*4);
			}
		}
	}
	return newScore;
}

/*
Checks if the game is over by seeing if there are any blocks in the top row
*/
int isGameLost(){
	int lost = 0;
	int counter = 0;
	for (int i = 0; i < 10; i++){
		counter += checkMatrixPixel(getCorrectPosition(i));
	}
	if (counter < 10){
		lost = 1;
	}
	return lost;
}

/*
Creates keyboard and controls keyboard input by reusing button flags
*/
void enterName(char *name){
	int x = 0; int y = 0;
	volatile int enteringName = 1;
	lcd_clrscr();
	char* atoj = "A B C D E F G H I J";
	lcd_puts(atoj);
	x = 0; y = 1;
	lcd_gotoxy(x, y);
	char* ktot = "K L M N O P Q R S T";
	lcd_puts(ktot);
	x = 0; y = 2;
	lcd_gotoxy(x, y);
	char* utoz = "  U V W X Y Z < >";
	lcd_puts(utoz);
	x = 5; y = 3;
	lcd_gotoxy(x, y);
	lcd_puts("Name: ___");
	x = 0; y = 0;
	lcd_gotoxy(x,y);
	lcd_command(LCD_DISP_ON_BLINK);
	int pos = 0;
	while (enteringName){
		if (leftPressed){
			if (x == 2 && y == 2){
				x = 16;
			}else if (x == 0){
				x = 18;
			}else{
				x-=2;
			}
			lcd_gotoxy(x,y);
			leftPressed = 0;
		}
		if (rightPressed){
			if (x == 16 && y == 2){
				x = 2;
			}else if (x == 18){
				x = 0;
			}else{
				x+=2;
			}
			lcd_gotoxy(x,y);
			rightPressed = 0;
		}
		if (rotateLeftPressed){
			if (y == 2 || (y == 1 && (x == 0 || x == 18))){
				y = 0;
			}else{
				y++;
			}
			lcd_gotoxy(x,y);
			rotateLeftPressed = 0;
		}
		if (rotateRightPressed){
			if (y == 0 && (x == 18 || x == 0)){
				y = 1;
			}else if (y==0){
				y = 2;
			}else{
				y--;
			}
			lcd_gotoxy(x,y);
			rotateRightPressed = 0;
		}
		
		if (fallFast){
			lcd_gotoxy(pos+11, 3);
			char val[2];
			if (y == 0 && pos < 3){ //if cursor on top row and space left in name
				val[0] = atoj[x];
				val[1] = '\0';
				lcd_puts(val);
				name[pos] = val[0];
				pos++;
			}else if (y == 1 && pos < 3){ //if cursor on second row and space left in name
				val[0] = ktot[x];
				val[1] = '\0';
				lcd_puts(val);
				name[pos] = val[0];
				pos++;
			}else if (x == 14 && pos > 0 && y == 2){ //if backspace selected
				pos--;
				lcd_gotoxy(pos+11, 3);
				lcd_puts("_");
			}else if (x == 16 && pos == 3 && y == 2){ //if finished selected
				enteringName = 0;
			}else if (pos < 3 && x != 14 && x != 16){ //if cursor on third row and space left in name
				val[0] = utoz[x];
				val[1] = '\0';
				lcd_puts(val);
				name[pos] = val[0];
				pos++;
			}
			lcd_gotoxy(x,y);
			fallFast = 0;
		}
		
		_delay_ms(200);
	}
	lcd_clrscr();
	lcd_command(LCD_DISP_ON);
}

/*
Saves a name to the EEPROM at the startAddr
*/
void saveHighScoreName(int startAddr, char *name){
	int counter = 0;
	for (int i = startAddr; i <= (startAddr+16); i+=8){
		eeprom_write_byte((uint8_t*)i, name[counter++]);
	}
}

/*
Updates the high scores and writes them to the EEPROM
*/
int updateHighScores(uint32_t score){
	char name[3];
	int newHighScore = 0;
	
	if (score > highScore1){
		strncpy(highScoreName3, highScoreName2, 3);
		highScore3 = highScore2;
		strncpy(highScoreName2, highScoreName1, 3);
		highScore2 = highScore1;
		enterName(name);
		strncpy(highScoreName1, name, 3);
		highScore1 = score;
		newHighScore = 1;
	}else if (score > highScore2){
		strncpy(highScoreName3, highScoreName2, 3);
		highScore3 = highScore2;
		enterName(name);
		strncpy(highScoreName2, name, 3);
		highScore2 = score;
		newHighScore = 1;
	}else if (score > highScore3){
		enterName(name);
		strncpy(highScoreName3, name, 3);
		highScore3 = score;
		newHighScore = 1;
	}
	
	eeprom_write_dword((uint32_t*)16, highScore1);
	eeprom_write_dword((uint32_t*)48, highScore2);
	eeprom_write_dword((uint32_t*)80, highScore3);
	
	saveHighScoreName(112, highScoreName1);
	saveHighScoreName(136, highScoreName2);
	saveHighScoreName(160, highScoreName3);
	
	displayHighScores();
	
	return newHighScore;
}

/*
Main game loop
*/
void loop() { 
  uint32_t score = 0;
  int gameOver = 0;
  int level = 1;
  int levelCounter = 0;
  int currentBlock = rand() % 7;
  while(!gameOver && !restart){
	int nextBlock = rand() % 7;
	int fallingBlockPos = -28;
	int collision = 0;
	int rotation = 0;
	clearPreview();
	drawPreview(nextBlock);
	while (!collision && !restart){
		checkButtonInput(currentBlock, &fallingBlockPos, &rotation);
		if (moveDown(currentBlock, rotation, &fallingBlockPos)){
			collision = 1;
			int prevScore = score;
			score += handleFullRow();
			if (score > 999999999){
				score = 999999999;
			}
			if (prevScore != score){
				updateScoreDisp(score);
			}
			gameOver = isGameLost();
		}
		writeMatrixScreen();
		_delay_ms(500 - ((level-1)*50));
	}
	levelCounter++;
	if (levelCounter > (level*5)+(level*level) && level < 10){
		level++;
		updateLevelDisp(level);
		levelCounter = 0;
	}
	currentBlock = nextBlock;
  }
  
  if (gameOver){
	if (updateHighScores(score)){
		updateScoreDisp(score);
		updateLevelDisp(level);
	}
	restart = 0;
  }
  fillMatrixScreen();
  _delay_ms(1000);
  clearMatrixScreen();
  while (restart == 0){}; 
  setup();
  loop();
}

/*
Maim method. Initializes LCD panel, calls setup, and starts main loop
*/
int main(void){
	initScoreLCD();
	setup();
	loop();
	return 0;
}