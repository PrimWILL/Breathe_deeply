#ifndef _DUST_H
#define _DUST_H

#include <stdint.h>

typedef struct {
    float pm25;
    float pm10;
} fine_dust;

void sds011_init(int *fd);

int ProcessResponse(const uint8_t *packet, fine_dust *ret);

int read_sds(int loop, fine_dust *origin, int fd);

#endif /* _DUST_H */
