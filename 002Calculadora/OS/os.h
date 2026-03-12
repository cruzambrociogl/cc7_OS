// #ifndef OS_H
#define OS_H

// OS Layer: UART + basic conversions
// Exposes low-level primitives used by the library layer.

void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *s);
void uart_gets_input(char *buffer, int max_length);

int uart_atoi(const char *s);
void uart_itoa(int num, char *buffer);
int uart_atof100(const char *s);  // Parse decimal to fixed-point (x100)
void uart_float(int num100, char *buffer);  // Convert fixed-point (x100) to string