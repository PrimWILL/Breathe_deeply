#ifndef _LCD_H_
#define _LCD_H_

#define LCD_CHR 1 // Mode - Sending data
#define LCD_CMD 0 // Mode - Sending command
#define I2C_ADDR 0x27 // I2C device address
#define LCD_WIDTH 16 // Maximum characters per line 

#define LCD_LINE_1 0x80 // LCD RAM address for the 1st line
#define LCD_LINE_2 0xC0 // LCD RAM address for the 2nd line

void lcd_toggle_enable(int bits);

void lcd_byte(int bits, int mode);

void lcd_init();

void lcd_string(char *message, int line);

int lcd_open(int* file);

#endif /* _LCD_H_ */
