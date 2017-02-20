# Tetris-ATMega168

The program running:
https://www.youtube.com/watch?v=YWVMb-ZXHz4

It is a game of Tetris that runs on the ATMega168 AVR chip. It uses the HT1632C 16x24 LED matrix panel from Adafruit, available here: https://www.adafruit.com/product/555

In order to run it, you need the above mentioned LED panel, a 4 line I2C LCD panel, an ATMega168 chip, 6 buttons, and a lot of jumper wires. The important pins are all available at the top of tetris.c to change. 

It has buttons for rotating blocks in both directions, moving left and right, speeding up the block, and restarting the game. It saves the top 3 highscores using the EEPROM memory on the chip, and keeps track of the score and level the player has reached as he is playing. EEPROM is also used to increment a seed that is used for seeding the random number generator for block creation at the beginning of every game. The score is determined by how many rows the player gets at a time: one row is 20 points, 2 rows is 100, 3 rows is 500, and 4 rows is 2500. The level is determined by how many blocks have fallen so far, and it controls how fast the block falls. There are ten levels.

When the user gets a highscore, a keyboard pops up on the LCD panel. The user can put in his initials by reusing the buttons used to rotate and move the blocks while playing the game. The initials will appear next to their score for as long as they are on the scoreboard.  
