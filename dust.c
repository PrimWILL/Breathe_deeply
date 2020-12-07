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

char port[20] = "/dev/ttyUSB0";

// int fd = 0xff;                   // filepointer
void sds011_init(int *fd)
{
   system("modprobe usbserial");
   system("modprobe ch341");

   *fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);

   if (*fd < 0) {
      fprintf(stderr, "could not open %s\n", port);
      restore_ser(*fd);
      exit(EXIT_FAILURE);
   }

   configure_interface(*fd, B9600);
   set_blocking(*fd, 0);

   usleep(100); 
   tcflush(*fd,TCIOFLUSH);
}

int ProcessResponse(const uint8_t *packet, fine_dust *ret)
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
    int num = 0;
    int count = loop;
    fine_dust data = {0,0};
    while (count) {
        read(fd, buf, sizeof(buf));

        if (ProcessResponse(buf, &data) == 1) {
            origin->pm25 += data.pm25;
            origin->pm10 += data.pm10;
            count--;
        }
    }
    origin->pm25 /= (float)10;
    origin->pm10 /= (float)10;

    return 1;
}
