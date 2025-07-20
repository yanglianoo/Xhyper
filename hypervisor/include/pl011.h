#ifndef __PL011_H__
#define __PL011_H__

void pl011_putc(char c);
void pl011_puts(char *s);
int  pl011_getc(void);
void pl011_init(void);

#endif
