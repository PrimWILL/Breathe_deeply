#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "lcd.h"

int file_i2c;
static int LCD_BACKLIGHT = 0x08; // ON
static int ENABLE = 4;           // enable bit

static int E_PULSE = 500;
static int E_DELAY = 500;
char *filename = (char*)"/dev/i2c-1";

void lcd_toggle_enable(int bits)
{
    unsigned char buffer[1];
    buffer[0] = (bits | ENABLE);
    int length = 1;

    usleep(E_DELAY);
    if (write(file_i2c, buffer, length) != length) 
        printf("Error: lcd_byte(E_DELAY).\n");

    usleep(E_PULSE);
    buffer[0] = (bits & ~ENABLE);
    if (write(file_i2c, buffer, length) != length) 
        printf("Error: lcd_byte(E_PULSE).\n");
    usleep(E_DELAY);
}

void lcd_byte(int bits, int mode)
{
    unsigned char buffer[1];
    int bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    int bits_low = mode | ((bits<<4) & 0xF0) | LCD_BACKLIGHT;

    int length = 1;
    buffer[0] = bits_high;

    if (write(file_i2c, buffer, length) != length) 
        printf("Error: lcd_byte(bits_high).\n");
    lcd_toggle_enable(bits_high);

    buffer[0] = bits_low;
    if (write(file_i2c, buffer, length) != length) 
        printf("Error: lcd_byte(bits_low).\n");
    lcd_toggle_enable(bits_low);
}

void lcd_init()
{
    lcd_byte(0x33, LCD_CMD);    // 110011 initialize
    lcd_byte(0x32, LCD_CMD);    // 110010 initialize
    lcd_byte(0x06, LCD_CMD);    // 000110 cursor move direction
    lcd_byte(0x0C, LCD_CMD);    // 001100 display on, cursor off, blink off
    lcd_byte(0x28, LCD_CMD);    // 101000 data length, number of lines, font size
    lcd_byte(0x01, LCD_CMD);    // 000001 clear display
    usleep(E_DELAY);
    printf("LCD init complete\n");
}

/* send string to display */
void lcd_string(char *message, int line)
{
    lcd_byte(line, LCD_CMD);    // send command to line
    for(int i = 0; i < LCD_WIDTH && i < strlen(message); i++) {
        lcd_byte(message[i],LCD_CHR);   // sending data
    }
}

int lcd_open(int *file)
{
	//----- OPEN THE I2C BUS -----
	if ((*file = open(filename, O_RDWR)) < 0)
	{
		//ERROR HANDLING: you can check errno to see what went wrong
		printf("Failed to open the i2c bus");
		return -1;
	}
	
	int addr = 0x27;          // The I2C address of the slave
	if (ioctl(*file, I2C_SLAVE, addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n");
		//ERROR HANDLING; you can check errno to see what went wrong
		return -1;
	}
	file_i2c = *file;
    return 1;
}
