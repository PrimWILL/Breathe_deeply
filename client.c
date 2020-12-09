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

#define LOW 0
#define HIGH 1
#define PIN 20
#define POUT 21
#define IN 0
#define OUT 1

static const char *DEVICE = "/dev/spidev0.0";
int fd = 0xff;                   // filepointer
fine_dust origin;
int lcd_file;

void error_handling( char *message){
 fputs(message,stderr);
 fputc( '\n',stderr);
 exit( 1);
}

static int GPIOExport(int pin)
{
#define BUFFER_MAX 3
   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;

   fd = open("/sys/class/gpio/export", O_WRONLY);
   if (-1 == fd)
   {
      fprintf(stderr, "Failed to open export for writing!\n");
      return (-1);
   }

   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
   write(fd, buffer, bytes_written);
   close(fd);
   return (0);
}

static int GPIODirection(int pin, int dir)
{
   static const char s_directions_str[] = "in\0out";

#define DIRECTION_MAX 35
   char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
   int fd;

   snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

   fd = open(path, O_WRONLY);
   if (-1 == fd)
   {
      fprintf(stderr, "Failed to open gpio direction for writing!\n");
      return (-1);
   }

   if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
   {
      fprintf(stderr, "Failed to set direction!\n");
      return (-1);
   }

   close(fd);
   return (0);
}

static int GPIOUnexport(int pin)
{
   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;

   fd = open("/sys/class/gpio/unexport", O_WRONLY);
   if (-1 == fd)
   {
      fprintf(stderr, "Failed to open unexport for writing!\n");
      return (-1);
   }

   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
   write(fd, buffer, bytes_written);
   close(fd);
   return (0);
}

static int GPIORead(int pin)
{
#define VALUE_MAX_GPIO 30
   char path[VALUE_MAX_GPIO];
   char value_str[3];
   int fd;

   snprintf(path, VALUE_MAX_GPIO, "/sys/class/gpio/gpio%d/value", pin);
   fd = open(path, O_RDONLY);

   if (-1 == fd)
   {
      fprintf(stderr, "Failed to open gpio value for reading!\n");
      return (-1);
   }

   if (-1 == read(fd, value_str, 3))
   {
      fprintf(stderr, "Failed to read value!\n");
      return (-1);
   }

   close(fd);
   return (atoi(value_str));
}

static int GPIOWrite(int pin, int value)
{
   static const char s_values_str[] = "01";

   char path[VALUE_MAX_GPIO];
   int fd;

   // printf("write value!\n");

   snprintf(path, VALUE_MAX_GPIO, "/sys/class/gpio/gpio%d/value", pin);
   fd = open(path, O_WRONLY);
   if (-1 == fd)
   {
      fprintf(stderr, "Failed to open gpio value for writing!\n");
   }

   if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
   {
      fprintf(stderr, "Failed to write value!\n");
      close(fd);
      return (0);
   }
   // printf("write value!!!!!!!!!!!\n");
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
   
   while(1) {
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

        printf("Receive message from Server : %s\n", msg);
        if (strncmp(on, msg, 1) == 0) {
            printf("message is 1\n");
            close(sock);
            
            int fd = open(DEVICE, O_RDWR);
            // measure wind
            for (int i = 0; i < 5; i++) {
                wind += ((float)readadc(fd, 0)/256.0) * 5.0 * 6.0;
            }
            wind = wind / 5;
            printf("wind: %f!\n", wind);
            
            // measure water
            water = readadc(fd, 1);
            if (water > 0) 
                snprintf(wind_rain_message, 16, "%02.1fm/s  rain  ", wind);
            else {
                snprintf(wind_rain_message, 16, "%02.1fm/s no rain", wind);
            }
			printf("%s\n", wind_rain_message);
            
            // measure temperature & humidity
            arduino(temp_humid_message);
                
            // measure fine dust
            sds011_init(&fd);
            fine_dust origin = {0, 0};
            read_sds(5, &origin, fd);
            snprintf(dust_message, 16, "pm10: %3.2f    ", origin.pm10);
            snprintf(finedust_message, 16, "pm2.5: %3.2f   ", origin.pm25);
            printf("PM2.5: %.2f, PM10: %.2f\n", origin.pm25, origin.pm10);
            
            // measure wind chill temperature
            // float wind_chill = 13.12 + 0.6215 * 
            
            // print LCD
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
                    lcd_byte(0x01, LCD_CMD);
                }
                
                if (buttonCount == 1) {
                    lcd_string(wind_rain_message, LCD_LINE_1);
                    lcd_string(temp_humid_message, LCD_LINE_2);					
                    buttonCount++;
                }
                else if (buttonCount == 3) {
                    lcd_string(dust_message, LCD_LINE_1);
                    lcd_string(finedust_message, LCD_LINE_2);						
                    buttonCount++;
                }			
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
            printf("message is 0\n");
        }
        close(sock);
   }
	   //Disable GPIO pins
   if (-1 == GPIOUnexport(POUT))
      return (4);
    return (0);
}
