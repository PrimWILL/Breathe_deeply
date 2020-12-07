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
static const char *deviceName = "/dev/i2c-1";  
#define ADDRESS 0x04
   
 void arduino(char *message)
{  
    int file;  
   
    if ((file = open( deviceName, O_RDWR ) ) < 0)   
    {  
        fprintf(stderr, "I2C: Failed to access %s\n", deviceName);  
        exit(1);  
    }  
    printf("I2C: Connected\n");  
  
    printf("I2C: acquiring buss to 0x%x\n", ADDRESS);  
    if (ioctl(file, I2C_SLAVE, ADDRESS) < 0)   
    {  
        fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x\n", ADDRESS);  
        exit(1);  
    }  

    unsigned char cmd[16];  


    printf("Sending %d\n", 1);  

    cmd[0] = 1;  
    if (write(file, cmd, 1) == 1)   
    {  
            
        usleep(10000);  

        char buf[16], buf2[16];  
        read( file, buf, 16 );        

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
        strcpy(message, buf2);
    } 
    usleep(10000);  
    close(file);  
}  
