#include "stdio.h"

#include <stdarg.h>
#include "../OS/os.h"
#include "string.h"

// PRINT returns number of characters written (roughly)
int PRINT(const char *fmt, ...) {
    va_list args;
    int count = 0;

    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            uart_putc(*fmt++);
            count++;
            continue;
        }

        // handle format
        fmt++; // skip '%'
        if (*fmt == 's') {
            const char *s = va_arg(args, const char *);
            if (!s) s = "(null)";
            uart_puts(s);
            while (*s++) count++;
            fmt++;
        } else if (*fmt == 'd') {
            int n = va_arg(args, int);
            char buf[16];
            uart_itoa(n, buf);
            uart_puts(buf);
            {
                const char *p = buf;
                while (*p++) count++;
            }
            fmt++;
        } else if (*fmt == 'f') {
            // fixed-point (x100)
            int numb = va_arg(args, int);
            char buf[24];
            uart_float(numb, buf);
            uart_puts(buf);
            {
                const char *p = buf;
                while (*p++) count++;
            }
            fmt++;
        } else {
            // unknown: print literally
            uart_putc('%');
            uart_putc(*fmt);
            count += 2;
            if (*fmt) fmt++;
        }
    }

    va_end(args);
    return count;
}

// READ supports one specifier per call (simple like a manual lab)
int READ(const char *fmt, ...) {
    va_list args;
    char buf[64];

    va_start(args, fmt);

    // find first '%'
    while (*fmt && *fmt != '%') fmt++;
    if (*fmt != '%') {
        va_end(args);
        return 0;
    }

    fmt++;
    uart_gets_input(buf, sizeof(buf));

    if (*fmt == 's') {
        char *dest = va_arg(args, char *);
        // copy safely
        my_strncpy(dest, buf, 63);
        va_end(args);
        return 1;
    }

    if (*fmt == 'd') {
        int *out = va_arg(args, int *);
        *out = uart_atoi(buf);
        va_end(args);
        return 1;
    }

    if (*fmt == 'f') {
        // fixed-point (x100)
        int *out100 = va_arg(args, int *);
        *out100 = uart_atof100(buf);
        va_end(args);
        return 1;
    }

    va_end(args);
    return 0;
}
