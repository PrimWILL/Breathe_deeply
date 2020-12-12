#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdarg.h>

#include "serial.h"
#include "dust.h"
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

char port[20] = "/dev/ttyUSB0";     // sds011이 연결된 위치

void sds011_init(int *fd)
{
   system("modprobe usbserial");
   system("modprobe ch341");

   *fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);

   if (*fd < 0) {
      fprintf(stderr, "could not open %s\n", port);
      exit(EXIT_FAILURE);
   }

   set_UART(*fd); // set UART communication

   usleep(100); // need some time for preparing flush
   tcflush(*fd,TCIOFLUSH); // 송수신 데이터를 버린다 (TCIOFLUSH: 입력을 비우고, 출력이 완료되지 않는 것도 비움)
}

int read_finedust(const uint8_t *packet, fine_dust *ret)
{
    if (packet[1] == 0xC0) {
        ret->pm25 = (float)(((packet[3] * 256) + packet[2]) / 10.0);
        ret->pm10 = (float)(((packet[5] * 256) + packet[4]) / 10.0);
        return 1;
    }
    return -1;
}

int read_sds(int loop, fine_dust *origin, int fd) {
    uint8_t buf[20];
    int count = loop;
    fine_dust data = {0,0}; // pm2.5, pm10의 값을 저장하기 위한 구조체 선언
    
    while (count) { // 5번 값을 측정
        read(fd, buf, sizeof(buf));

        if (read_finedust(buf, &data) == 1) {
            origin->pm25 += data.pm25;
            origin->pm10 += data.pm10;
            count--;
        }
    }
    // 5번 측정한 값의 평균을 저장
    origin->pm25 /= (float)10;
    origin->pm10 /= (float)10;

    return 1;
}
