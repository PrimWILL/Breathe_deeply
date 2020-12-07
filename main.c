#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "arduino.h"
#include "lcd.h"



static const char *DEVICE = "/dev/spidev0.0";
int fd = 0xff;                   // filepointer
int lcd_file;

int main(int argc, char **argv) {	
		char temp_humid_message[16];
	
		// measure temperature & humidity
		
		arduino(temp_humid_message);
		
		// print LCD
			if (lcd_open(&lcd_file) < 0) {
				exit(1);
			}
			lcd_init();

			while(1) {
				lcd_string(temp_humid_message, LCD_LINE_1);
				lcd_string("SystemSoftware!", LCD_LINE_2);				
				sleep(3);
				
				// sleep(3);
							lcd_byte(0x01, LCD_CMD);
			}
			

	return 0;
}

