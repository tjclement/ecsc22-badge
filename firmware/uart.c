#include <stdio.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define MAX_HANDLERS (16)
#define BUFFER_SIZE (4096)

static char read_buffer[BUFFER_SIZE];
static void (*input_handlers[MAX_HANDLERS])(char*);
static int current_depth = -1;

void on_uart_rx() {
    int chars_read = 0;
    while (uart_is_readable_within_us(UART_ID, 500) && chars_read < BUFFER_SIZE) {
        uint8_t ch = uart_getc(UART_ID);
        read_buffer[chars_read++] = (char) ch;
    }
    read_buffer[chars_read] = '\0';

    bool data_read = (read_buffer[0] != '\0' || chars_read > 1);

    if (data_read && current_depth >= 0) {
        input_handlers[current_depth](read_buffer);
    }
}

void uart_setup() {
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 115200);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    // uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}

void uart_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[BUFFER_SIZE];
    vsnprintf(buffer, BUFFER_SIZE-1, format, args);
    va_end(args);
}

void uart_push_handler(void (*handler)(char*)) {
    current_depth += 1;
    input_handlers[current_depth] = handler;
}
