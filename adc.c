#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "adc.h"

static uint8_t MODE = SPI_MODE_0; // 디바이스의 통신 타이밍 지정 (클록 극성 = 0, 클록 단계 = 0)
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

/*
 * Ensure all settings are correct for the ADC
 */
int prepare(int fd) {

  // SPI mode 설정
  if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
    perror("Can't set MODE");
    return -1;
  }

  // set 8 bits of data per transfer word
  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
    perror("Can't set number of BITS");
    return -1;
  }

  // set maximum SPI transfer speed of WR
  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
    perror("Can't set write CLOCK");
    return -1;
  }

  // set maximum SPI transfer speed of RD
  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
    perror("Can't set read CLOCK");
    return -1;
  }

  return 0;
}

/*
 * (SGL/DIF = 0, D2=D1=D0=0)
 */ 
uint8_t control_bits_differential(uint8_t channel) {
  return (channel & 7) << 4;
}

/*
 * (SGL/DIF = 1, D2=D1=D0=0)
 */ 
uint8_t control_bits(uint8_t channel) {
  return 0x8 | control_bits_differential(channel);
}

/*
 * Given a prep'd descriptor, and an ADC channel, fetch the
 * raw ADC value for the given channel.
 */
int readadc(int fd, uint8_t channel) {
  uint8_t tx[] = {1, control_bits(channel), 0};
  uint8_t rx[3];

  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)tx,  // a pointer to the data to be transferred
    .rx_buf = (unsigned long)rx,  // a pointer to storage for received data
    .len = ARRAY_SIZE(tx),        // length in bytes of tx and rx buffers
    .delay_usecs = DELAY,         // delay between last bit and deassertion of CS
    .speed_hz = CLOCK,            // the clock speed, in Hz
    .bits_per_word = BITS,        // override global word length for this transfer
  };

  // driver를 읽어오기를 요청
  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
    perror("IO Error");
    abort();
  }
  // rx[1]과 rx[2]로 나누어져 ADC에 들어온 10bits의 값을 계산하여 return
  return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}
