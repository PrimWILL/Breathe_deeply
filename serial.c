#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "serial.h"
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

void set_UART(int fd)
{
    struct termios options; // 터미널 제어 구조체 선언
    /*
    * struct termios
    * {
    *   tcflag_t c_iflag;   // input flags
    *   tcflag_t c_oflag;   // output flags
    *   tcflat_t c_cflag;   // control flags
    *   tcflag_t c_lflag;   // local flags
    *   cc_t c_cc[NCCS];    // control chars
    *   speed_t c_ispeed;   // input speed
    *   speed_t c_ospeed;   // output speed
    * }
    */

    tcgetattr(fd, &options);  // 터미널 속성을 얻는다

    cfsetospeed(&options, B9600); // 출력 속도(B9600) 설정
    cfsetispeed(&options, B9600); // 입력 속도(B9600) 설정

    // Serial control options
    options.c_cflag |= CLOCAL;    // ignore modem control lines
    options.c_cflag |= CREAD;     // enable receiver  
    options.c_cflag &= ~CSIZE;    // character size mask
    options.c_cflag |= CS8;       // character size 8 bits  
    options.c_cflag &= ~PARENB;   // no parity 
    options.c_cflag &= ~CSTOPB;   // one stop bit
    options.c_cflag &= ~CRTSCTS;  // no hardware flowcontrol  

    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 0; // block read to first character
    options.c_cc[VTIME] = 5; // 0.5 sec timeout

    tcsetattr(fd, TCSANOW, &options); // 터미널 속성(TCSANOW: 속성 바로 반영)을 설정한다
}
