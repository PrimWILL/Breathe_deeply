#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>



#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PORT 8204
#define LISTEN_QUEUE_SIZE 3
#define PIN 20
#define POUT 21

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
      return(-1);
   }
   
   bytes_written  = snprintf(buffer, BUFFER_MAX, "%d", pin);
   write(fd, buffer, bytes_written);
   close(fd);
   return(0);
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
      return(-1);
   }
   
   if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
   {
      fprintf(stderr, "Failed to set direction!\n");
      return(-1);
   }
   
   close(fd);
   return(0);
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
      return(-1);
   }
   
   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
   write(fd, buffer, bytes_written);
   close(fd);
   return(0);
}

static int prepare(int fd) 
{
   if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) 
   {
       perror("Can't set MODE");
       return -1;
    }

   if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) 
   {
       perror("Can't set number of BITS");
       return -1;
    }

   if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) 
   {
       perror("Can't set write CLOCK");
       return -1;
    }

   if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) 
   {
       perror("Can't set read CLOCK");
       return -1;
    }

   return 0;
}

void error_handling(char *message)
{
   fputs(message,stderr);
   fputc('\n',stderr);
   exit(1);
}

static int GPIORead(int pin)
{
#define VALUE_MAX 30
   char path[VALUE_MAX];
   char value_str[3];
   int fd;
   
   snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
   fd = open(path, O_RDONLY);
   if (-1 == fd)
   {
      fprintf(stderr, "Failed to open gpio value for reading!\n");
      return(-1);
   }
   
   if (-1 == read(fd, value_str, 3))
   {
      fprintf(stderr, "Failed to read value!\n");
      return(-1);
   }
   
   close(fd);
   return(atoi(value_str));
}

static int GPIOWrite(int pin, int value)
{
   static const char s_values_str[] = "01";
   
   char path[VALUE_MAX];
   int fd;
   
   snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
   fd = open(path, O_WRONLY);
   if (-1 == fd)
   {
      fprintf(stderr, "Failed to open gpio value for writing!\n");
      
   }
   
   if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
   {
      fprintf(stderr, "Failed to write value!\n");
      close(fd);
      return(0);
   }
}


uint8_t control_bits_differential(uint8_t channel) 
{
   return (channel & 7) << 4;
}

uint8_t control_bits(uint8_t channel) 
{
   return 0x8 | control_bits_differential(channel);
}

int readadc(int fd, uint8_t channel) {
  uint8_t tx[] = {1, control_bits(channel), 0};
  uint8_t rx[3];

  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)tx,
    .rx_buf = (unsigned long)rx,
    .len = ARRAY_SIZE(tx),
    .delay_usecs = DELAY,
    .speed_hz = CLOCK,
    .bits_per_word = BITS,
  };

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) 
  {
    perror("IO Error");
    abort();
  }

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) 
  {
    perror("IO Error");
    abort();
  }

   return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

int main(int argc, char **argv)
{
   time_t rawTime;
   struct tm* pTimeInfo;
   int earlyAlarmHour = 8; //이른 알람 시각 설정
   int earlyAlarmMin = 30; 
   int laterAlarmHour = 12; //늦은 알람 시각 설정
   int laterAlarmMin = 0;
   int realAlarmHour = earlyAlarmHour; //가장 처음에는 이른 알람 시각으로 우선 설정해 둠 (사용자의 선택에 따라 늦은 시각도 가능)
   int realAlarmMin = earlyAlarmMin;
   int hourSub = 0; //시간을 초로 변환할 때 쓰임
   int minSub = 0; //시간을 초로 변환할 때 쓰임
   
   int state = 1;
   int prev_state = 1;
   int light = 0;
   int alcoholValue = 0;
   int serv_sock,clnt_sock = -1;
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size;
   char msg[2];
   
   //Enable and Disable GPIO pins
   if (-1 == GPIOExport(PIN) || -1 == GPIOExport(POUT))
      return(1);
   
   //Set GPIO directions
   if (-1 == GPIODirection(PIN, IN) || -1 == GPIODirection(POUT,OUT))
      return(2);
   
   //Write GPIO value
   if ( -1 == GPIOWrite(POUT,1))
      return(3);
      
   if(argc!=2)
   {
      printf("Usage : %s <port>\n",argv[0]);
   }
 
   int fd = open(DEVICE, O_RDWR);
   
   //fd가 제대로 열렸는지 확인
   if (fd <= 0 || prepare(fd) == 1)
   {
      return -1;
   }
   
   //서버 열기
   serv_sock = socket(PF_INET, SOCK_STREAM, 0);
   if(serv_sock == -1)
      error_handling("socket() error");
      
   memset(&serv_addr, 0 , sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons(atoi(argv[1]));

   if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
      error_handling("bind() error");
      
   while(1)
   {
      
   rawTime = time(NULL);
   pTimeInfo = localtime(&rawTime);
      if(clnt_sock<0)
      {
         if(listen(serv_sock,5) == -1) {
            error_handling("listen() error");
         }

         clnt_addr_size = sizeof(clnt_addr);
         clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
         if(clnt_sock == -1) {
            error_handling("accept() error");
         }
      }
      state = GPIORead(PIN);

      //현재 시각을 출력함
      printf("pTimeInfohour: %d min: %d sec: %d\n", pTimeInfo->tm_hour, pTimeInfo->tm_min, pTimeInfo->tm_sec);

      //NO.1 time (알람 시각이 된 순간)
      if ((pTimeInfo->tm_hour == realAlarmHour) && (pTimeInfo->tm_min == realAlarmMin) && pTimeInfo->tm_sec == 0)
      {
         //알람을 작동하기 위해 클라이언트에게 1을 전송함
         snprintf(msg,2,"1"); 
         write(clnt_sock, msg, sizeof(msg));
         printf("msg = %s\n",msg);
         close(clnt_sock);
         clnt_sock = -1;
      }

      //NO.2 time (알람 시각에서 0초가 지나고 1초로 넘어가고부터 알람 시각 한 시간 후까지)
      else if(((pTimeInfo->tm_hour == realAlarmHour) && (pTimeInfo->tm_min == realAlarmMin) && (pTimeInfo->tm_sec >= 1)) 
      || ((pTimeInfo->tm_hour == realAlarmHour) && (pTimeInfo->tm_min > realAlarmMin))
      || ((pTimeInfo->tm_hour == realAlarmHour + 1) && (pTimeInfo->tm_hour < realAlarmMin)))
      {
         printf("prev: %d, st: %d\n", prev_state, state); 
         if (prev_state == 0 && state == 1) //버튼을 눌렀을 때
         {
            //이때 알코올 측정을 한 번 더 하는 것은 도전 과제인 알코올 분해 농도를 구하기 위함
            alcoholValue = readadc(fd, 0); //ADC를 이용해 아날로그 값을 읽어옴
            printf("alcohol: %d\n", alcoholValue); //Alcohol value 값을 출력함
            printf("Send 0 to client\n"); //클라이언트에게 "0"을 보냈음을 출력함

            //알람을 끄기 위해 클라이언트에게 "0"을 보냄
            snprintf(msg,2,"0");
            write(clnt_sock, msg, sizeof(msg));
            close(clnt_sock);
            clnt_sock = -1;
            pTimeInfo = localtime(&rawTime);
            hourSub = realAlarmHour + 1 - pTimeInfo->tm_hour;
            minSub = realAlarmMin - pTimeInfo->tm_min;

            sleep(60 * ((60 * hourSub) + minSub)); //NO.3 시간까지 기다림
         }
         prev_state = state;
      }

      //NO.3 time (알람 시각 한 시간 후부터 다음날 알람 시각 한 시간 전까지)
      //자정이 되면 24시가 아니라 0시로 초기화되기 때문에 자정을 기준으로 나누었음
      else if (((realAlarmHour + 1 == pTimeInfo->tm_hour) && (realAlarmMin <= pTimeInfo->tm_min)) || 
      ((realAlarmHour + 1 < pTimeInfo->tm_hour) && ((23 >= pTimeInfo->tm_hour) && (59 >= pTimeInfo->tm_min)) ||
      ((0 <= pTimeInfo->tm_hour) && ((earlyAlarmHour - 1 < pTimeInfo->tm_hour) && (earlyAlarmMin >= pTimeInfo->tm_min)))
      {
         printf("else if - prev: %d, st: %d\n", prev_state, state);
         if (prev_state == 0 && state == 1) //버튼을 눌렀을 때
         {
            alcoholValue = readadc(fd, 0); //ADC를 통해 아날로그 값을 읽어옴

            if (alcoholValue > 500) //500 이상이면 과음으로 판별
            {
               printf("%d: You have to wake up earlier!!!\n", alcoholValue); //Alcohol value와 함께 메시지 출력
               realAlarmHour = earlyAlarmHour; //과음 상태이니 알람 시각을 이른 시각으로 설정
               realAlarmMin = earlyAlarmMin;
               pTimeInfo = localtime(&rawTime);
               hourSub = realAlarmHour - pTimeInfo->tm_hour;
               minSub = realAlarmMin - pTimeInfo->tm_min;

               sleep(60 * ((60 * hourSub) + minSub)); //알람 시각까지 기다림
            }

            else //과음 상태가 아닌 경우
            {
               printf("%d: You can drive!\n", alcoholValue); //Alcohol value와 함께 메시지 출력
               realAlarmHour = laterAlarmHour; //과음 상태가 아니니 알람 시각을 늦은 시각으로 설정
               realAlarmMin = laterAlarmMin;
               pTimeInfo = localtime(&rawTime);
               hourSub = realAlarmHour - pTimeInfo->tm_hour;
               minSub = realAlarmMin - pTimeInfo->tm_min;

               sleep(60 * ((60 * hourSub) + minSub)); //알람 시각까지 기다림
            }
         }
         
         prev_state = state;
         
      }

      //NO.4 time (이른 시각 두 시간 전에도 알코올이 측정되지 않았을 때)
      else if (((earlyAlarmHour - 2) == pTimeInfo->tm_hour) && ((earlyAlarmMin + 1) == pTimeInfo->tm_min))
      {
         realAlarmHour = earlyAlarmHour; //측정하지 않고 잔 것으로 판단하고 이른 시각으로 알람 설정
         realAlarmMin = earlyAlarmMin;
         pTimeInfo = localtime(&rawTime);
         hourSub = realAlarmHour - pTimeInfo->tm_hour;
         minSub = realAlarmMin - pTimeInfo->tm_min;

         sleep(60 * ((60 * hourSub) + minSub)); //알람 시각까지 기다림
      }    
      sleep(1);
   }
   
   close(serv_sock);
   close(fd);
   return 0;
}
