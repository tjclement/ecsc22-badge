#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "uart.h"

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
static int cur_write_index = 0;
static void (*input_handlers[MAX_HANDLERS])(char*, int);
static int current_depth = -1;


void uart_setup() {
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, true, true);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, true);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    // int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    // irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    // irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    // uart_set_irq_enables(UART_ID, true, false);
}

void uart_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[BUFFER_SIZE];
    vsnprintf(buffer, BUFFER_SIZE-1, format, args);
    va_end(args);
    uart_puts(UART_ID, buffer);
}

void uart_write(uint8_t* data, unsigned int length) {
    uart_write_blocking(UART_ID, data, length);
}

void uart_push_handler(void (*handler)(char*, int)) {
    current_depth += 1;
    input_handlers[current_depth] = handler;
}

void uart_task() {
    if (!uart_is_readable(UART_ID)) { return; }

    int chars_read = 0;
    while (uart_is_readable_within_us(UART_ID, 500) && chars_read < BUFFER_SIZE) {
        uint8_t ch = uart_getc(UART_ID);
        read_buffer[cur_write_index++] = (char) ch;
    }
    read_buffer[cur_write_index] = '\0';

    bool data_read = (read_buffer[0] != '\0' || cur_write_index > 0);

    if (data_read && current_depth >= 0) {
        input_handlers[current_depth](read_buffer, cur_write_index);
        cur_write_index = 0;
    }
}