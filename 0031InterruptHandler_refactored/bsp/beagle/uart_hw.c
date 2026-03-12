#include "../../hal/uart.h"
#include "../../hal/types.h"

// AM335x UART0
#define UART0_BASE       0x44E09000
#define UART_THR         (UART0_BASE + 0x00)  // Transmit Holding Register
#define UART_LSR         (UART0_BASE + 0x14)  // Line Status Register
#define UART_LSR_THRE    0x20                 // Transmit Holding Register Empty
#define UART_LSR_RXFE    0x01                 // Data Ready (Bit 0)

void uart_putc(char c) {
    // Wait until Transmit Holding Register is empty
    while ((GET32(UART_LSR) & UART_LSR_THRE) == 0);
    PUT32(UART_THR, c);
}

char uart_getc(void) {
    // Wait until data is available
    while ((GET32(UART_LSR) & UART_LSR_RXFE) == 0);
    return (char)(GET32(UART_THR) & 0xFF);
}
