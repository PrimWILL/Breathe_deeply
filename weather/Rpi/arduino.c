#include <string.h>  
#include <unistd.h>  
#include <errno.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <linux/i2c-dev.h>  
#include <sys/ioctl.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <string.h>
   
//I2C bus  
static const char *fileName = "/dev/i2c-1";  
#define ADDRESS 0x04    // I2C DEVICE(Arduino) ADDRESS
   
 void arduino(char *message)
{  
    int file;  
    int sending_value = 1;
   
    // file open
    if ((file = open(fileName, O_RDWR)) < 0)   
    {  
        fprintf(stderr, "I2C: Failed to access %s\n", fileName);  
        exit(1);  
    }  

    // set address of I2C DEVICE
    if (ioctl(file, I2C_SLAVE, ADDRESS) < 0)   
    {  
        fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x\n", ADDRESS);  
        exit(1);  
    }  

    unsigned char cmd[16];  

    printf("Sending %d\n", sending_value); // for checking in SHELL
    cmd[0] = sending_value;  

    if (write(file, cmd, 1) == 1)   // send data to slave   
    {  
        usleep(10000);  

        char buf[16];            // buffer which read data
        char buf2[16];           // buffer which will be copied
        read(file, buf, 16);     // read data of Arduino

        int i;  
        for(i=0; i<16; i++ )  
        {  
            buf2[i] = buf[i];  

            if ( buf[i] == '\n' )   
            {  
                buf2[i]='\0';  
                break;  
            }  
        }  
        strcpy(message, buf2);  // string copy: from buf2 to message 
    } 
    printf("%s\n", message);    // shell에서 message를 확인하기 위한 용도
    usleep(10000);  
    // close(file);  
}  
