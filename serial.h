#ifndef _SERIAL_H
#define _SERIAL_H


void configure_interface(int fd, int speed);

void set_blocking(int fd, int should_block);

void restore_ser(int fd);

#endif /* _SERIAL_H */
