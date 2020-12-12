#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

#define VALUE_MAX    256 

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

int prepare(int fd);

uint8_t conrol_bits_differential(uint8_t channel);

uint8_t control_bits(uint8_t channel);

int readadc(int fd, uint8_t channel);

#endif /* _ADC_H_*/
