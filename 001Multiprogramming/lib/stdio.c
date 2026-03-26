#include "stdio.h"
#include "../hal/uart.h"

static void print_int(int n) {
    if (n < 0) {
        uart_putc('-');
        n = -n;
    }
    if (n == 0) {
        uart_putc('0');
        return;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (--i >= 0)
        uart_putc(buf[i]);
}

void PRINT(const char *fmt, ...) {
    // On ARM, variadic args after fmt are passed in R1-R3, then stack.
    // We access them by walking memory after &fmt.
    unsigned int *args = ((unsigned int *)&fmt) + 1;
    int arg_idx = 0;

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                    print_int((int)args[arg_idx++]);
                    break;
                case 'c':
                    uart_putc((char)args[arg_idx++]);
                    break;
                case 's':
                    os_write((const char *)args[arg_idx++]);
                    break;
                case '%':
                    uart_putc('%');
                    break;
            }
        } else {
            if (*fmt == '\n') uart_putc('\r');
            uart_putc(*fmt);
        }
        fmt++;
    }
}
