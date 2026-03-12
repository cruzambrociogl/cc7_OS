#ifndef UART_H
#define UART_H

// Platform-specific (implemented in bsp/<target>/uart_hw.c)
void uart_putc(char c);
char uart_getc(void);

// Generic API (implemented in hal/uart.c)
void os_write(const char *s);
void os_read(char *buffer, int max_length);

#endif // UART_H
