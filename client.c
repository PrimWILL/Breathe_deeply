#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "adc.h"
#include "lcd.h"
#include "serial.h"
#include "dust.h"
#include "arduino.h"
#include "mypwm.h"

static const char *DEVICE = "/dev/spidev0.0";
int fd = 0xff;                   // filepointer
fine_dust origin;
int lcd_file;

void error_handling( char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char msg[2];
    char on[2] = "1";
    int str_len;
	
	float wind = 0;
	int water = 0;
	
	char dust_message[16];
	char finedust_message[16];
	char wind_rain_message[16];
	char temp_humid_message[16];

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    //Enable GPIO pins
    if (-1 == GPIOExport(POUT))
        return (1);

    //Set GPIO directions
    if (-1 == GPIODirection(POUT, OUT))
        return (2);

    // start socket communication with server
    while (1)
    {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1)
            error_handling("socket() error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
        serv_addr.sin_port = htons(atoi(argv[2]));

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
            error_handling("connect() error");

        str_len = read(sock, msg, sizeof(msg));
        if (str_len == -1)
            error_handling("read() error");

        printf("Receive message from Server : %s\n", msg); // for checking in SHELL]

        // when client receive message "1" from server
        if (strncmp(on, msg, 1) == 0) { 
            int fd = open(DEVICE, O_RDWR);

            /* MEASURE WIND */
            for (int i = 0; i < 5; i++) {
                wind += ((float)readadc(fd, 0)/256.0) * 5.0 * 6.0;
            }
            wind = wind / 5; // 5번 측정한 풍속값의 평균
            
            /* MEASURE WATER */
            water = readadc(fd, 1);
            if (water > 0) 
                snprintf(wind_rain_message, 16, "%02.1fm/s  rain  ", wind);
            else {
                snprintf(wind_rain_message, 16, "%02.1fm/s no rain", wind);
            }
			printf("%s\n", wind_rain_message); // for checking in SHELL
            
            /* MEASURE TEMPERATURE & HUMIDITY */
            arduino(temp_humid_message);
                
            /* MEASURE FINE DUST */
            sds011_init(&fd);
            fine_dust origin = {0, 0};
            read_sds(5, &origin, fd);
            snprintf(dust_message, 16, "pm10: %3.2f    ", origin.pm10);
            snprintf(finedust_message, 16, "pm2.5: %3.2f   ", origin.pm25);
            printf("PM2.5: %.2f, PM10: %.2f\n", origin.pm25, origin.pm10);
            
            /* PRINT LCD */
            if (lcd_open(&lcd_file) < 0) {
                exit(1);
            }
            lcd_init();

            int buttonStatus = 0;
            int prevButtonStatus = 1;
            int buttonCount = 0;

            if (-1 == GPIOWrite(POUT, 1))
                return (3);
            
            while(1) {
                if (prevButtonStatus == 0 && (buttonStatus != GPIORead(PIN))) {
                    buttonCount++;
                    lcd_byte(0x01, LCD_CMD);    // clear LCD
                }
                
                // when user press button once: display rain, wind, temperature, and humidity
                if (buttonCount == 1) { 
                    lcd_string(wind_rain_message, LCD_LINE_1);
                    lcd_string(temp_humid_message, LCD_LINE_2);					
                    buttonCount++;
                }

                // when user press button twice: display pm10 & pm2.5
                else if (buttonCount == 3) {
                    lcd_string(dust_message, LCD_LINE_1);
                    lcd_string(finedust_message, LCD_LINE_2);						
                    buttonCount++;
                }			

                // when user press button triple: clear LCD
                else if (buttonCount == 5) {
                    lcd_byte(0x01, LCD_CMD);
                    prevButtonStatus = 1;
                    buttonStatus = 1;
                    break;
                }

                buttonStatus = GPIORead(PIN);
                prevButtonStatus = buttonStatus;
                usleep(50000);
            }
        }
        else {
            printf("message is 0\n"); // for checking in SHELL
        }
        close(sock);
    }

    //Disable GPIO pins
    if (-1 == GPIOUnexport(POUT))
       return (4);
    return 0;
}
