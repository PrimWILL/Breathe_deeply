#ifndef _MYPWM_H_
#define _MYPWM_H_

#define LOW 0
#define HIGH 1
#define PIN 20
#define POUT 21
#define IN 0
#define OUT 1

int GPIOExport(int pin);

int GPIODirection(int pin, int dir);

int GPIOUnexport(int pin);

int GPIORead(int pin);

int GPIOWrite(int pin, int value);

#endif /* _PWM_H */