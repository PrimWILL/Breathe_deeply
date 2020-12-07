#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_TIME 85
#define DHTPIN 7
#define LOW 0
#define HIGH 1

int dht11_dat[5] = {0, };
void read_dht11_dat()
{

  uint8_t laststate = HIGH;
  uint8_t counter = 0;
  uint8_t j = 0, i;
  /*uint8_t flag = HIGH;
  uint8_t state = 0;
  float f;*/

  dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
	printf("1\n");
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(18);
 

  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHTPIN, INPUT);

  for (i = 0; i < MAX_TIME; i++) 
  {
    counter = 0;
    while ( digitalRead(DHTPIN) == laststate) 
   { 
       counter++;
      delayMicroseconds(1);
      if (counter == 200) break;
    }

    laststate = digitalRead(DHTPIN);

    if (counter == 200) break;

    if ((i >= 4) && (i % 2 == 0)) 
   {

      dht11_dat[j / 8] <<= 1 ;
      if (counter > 20) dht11_dat[j / 8] |= 1 ;
      j++ ;

    }
  }

  if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xff))) 
  {

   printf("humidity = %d.%d %% Temperature = %d.%d *C \n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]) ;

  }

  else read_dht11_dat();

}



int main(void)

{
  if (wiringPiSetup() == -1) exit(1);

  while (1) 
  {
    read_dht11_dat();
    sleep(1);
  }

  return 0;

}
