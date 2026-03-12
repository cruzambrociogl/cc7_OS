#include "../../hal/uart.h"
#include "../../hal/types.h"

// VersatilePB PL011 UART
#define UART0_BASE       0x101f1000
#define UART_DR          (UART0_BASE + 0x00)
#define UART_FR          (UART0_BASE + 0x18)
#define UART_FR_TXFF     0x20
#define UART_FR_RXFE     0x10

void uart_putc(char c) {
    // Wait until Transmit FIFO is NOT full
    while (GET32(UART_FR) & UART_FR_TXFF);
    PUT32(UART_DR, c);
}

char uart_getc(void) {
    // Wait until Receive FIFO is NOT empty
    while (GET32(UART_FR) & UART_FR_RXFE);
    return (char)(GET32(UART_DR) & 0xFF);
}
