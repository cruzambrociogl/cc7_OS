#include "os.h"

#define UART0_BASE 0x44E09000 // BeagleBone Black UART0

#define UART_THR     0x00  // Transmit Holding Register (Offset 0x00)
#define UART_LSR     0x14  // Line Status Register (Offset 0x14)
#define UART_LSR_THRE 0x20 // Transmit Holding Register Empty
#define UART_LSR_DR   0x01 // Data Ready

volatile unsigned int * const UART0 = (unsigned int *)UART0_BASE;

void uart_putc(char c) {
    if (c == '\n') {
        while ((UART0[UART_LSR / 4] & UART_LSR_THRE) == 0);
        UART0[UART_THR / 4] = '\r';
    }
    while ((UART0[UART_LSR / 4] & UART_LSR_THRE) == 0);
    UART0[UART_THR / 4] = (unsigned int)c;
}

char uart_getc(void) {
    while ((UART0[UART_LSR / 4] & UART_LSR_DR) == 0);
    return (char)(UART0[UART_THR / 4] & 0xFF);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_gets_input(char *buffer, int max_length) {
    int i = 0;
    char c;

    while (i < max_length - 1) {
        c = uart_getc();

        if (c == '\n' || c == '\r') {
            uart_putc('\n');
            break;
        }

        uart_putc(c); // echo
        buffer[i++] = c;
    }

    buffer[i] = '\0';
}

int uart_atoi(const char *s) {
    int num = 0;
    int sign = 1;
    int i = 0;

    if (s[i] == '-') {
        sign = -1;
        i++;
    }

    for (; s[i] >= '0' && s[i] <= '9'; i++) {
        num = num * 10 + (s[i] - '0');
    }

    return sign * num;
}

void uart_itoa(int num, char *buffer) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0 && i < 14) {
        buffer[i++] = (char)('0' + (num % 10));
        num /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // reverse
    {
        int start = 0;
        int end = i - 1;
        char temp;
        while (start < end) {
            temp = buffer[start];
            buffer[start] = buffer[end];
            buffer[end] = temp;
            start++;
            end--;
        }
    }
}

int uart_atof100(const char *s) {
    // fixed-point parser: [sign] digits [ '.' digits(0..2) ]
    int sign = 1;
    int i = 0;
    int int_part = 0;
    int frac_part = 0;
    int frac_digits = 0;

    if (s[i] == '-') {
        sign = -1;
        i++;
    }

    while (s[i] >= '0' && s[i] <= '9') {
        int_part = int_part * 10 + (s[i] - '0');
        i++;
    }

    if (s[i] == '.') {
        i++;
        while (s[i] >= '0' && s[i] <= '9' && frac_digits < 2) {
            frac_part = frac_part * 10 + (s[i] - '0');
            frac_digits++;
            i++;
        }
    }

    // normalize fractional digits to 2
    if (frac_digits == 0) {
        frac_part = 0;
    } else if (frac_digits == 1) {
        frac_part *= 10;
    }

    return sign * (int_part * 100 + frac_part);
}
void uart_float(int num100, char *buffer) {
    // convert fixed-point (x100) to string with 2 decimals
    // e.g., 2025 -> "20.25"

    int is_negative = 0;
    int i = 0;

    if (num100 < 0) {
        is_negative = 1;
        num100 = -num100;
    }

    // Split into integer and fractional parts
    int int_part = num100 / 100;
    int frac_part = num100 % 100;

    // Convert integer part
    if (int_part == 0) {
        buffer[i++] = '0';
    } else {
        char temp[16];
        int temp_i = 0;
        int temp_num = int_part;

        while (temp_num > 0) {
            temp[temp_i++] = (char)('0' + (temp_num % 10));
            temp_num /= 10;
        }

        // Reverse and copy
        while (temp_i > 0) {
            buffer[i++] = temp[--temp_i];
        }
    }

    // Add decimal point and fractional part
    buffer[i++] = '.';

    // Ensure two digits for fractional part
    buffer[i++] = (char)('0' + (frac_part / 10));
    buffer[i++] = (char)('0' + (frac_part % 10));

    buffer[i] = '\0';

    // Add negative sign if needed (insert at beginning)
    if (is_negative) {
        int j = i;
        while (j > 0) {
            buffer[j] = buffer[j-1];
            j--;
        }
        buffer[0] = '-';
    }
}