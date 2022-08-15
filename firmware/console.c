#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
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

#define CARRIAGE_RETURN (0x0d)
#define ESCAPE (0x1B)

#define MAX_HANDLERS (16)
#define READ_BUFFER_SIZE (4096)

static char read_buffer[READ_BUFFER_SIZE];
static void (*input_handlers[MAX_HANDLERS])(char*);
static int current_depth = -1;

void default_handler(char *input) {
    if (strnstr(input, "\033[", 3) != NULL) {
        // Don't print ANSI escape codes
        return;
    }

    printf("%s", input);

    // Convert \r to \r\n
    int len = strlen(input);
    if (input[len-1] == CARRIAGE_RETURN) {
        printf("\n");
    }
}

void console_setup() {
    current_depth = 0;
    input_handlers[current_depth] = default_handler;
}

void console_task() {
    int chars_read = 0;
    int ch;
    while ((ch = getchar_timeout_us(1000)) != PICO_ERROR_TIMEOUT) {
        read_buffer[chars_read++] = (char) ch;
    }
    read_buffer[chars_read] = '\0';

    bool data_read = (read_buffer[0] != '\0' || chars_read > 1);

    if (data_read && current_depth >= 0) {
        input_handlers[current_depth](read_buffer);
    }
}

void console_clear() {
    printf("\033[H\033[2J");
}

void console_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void console_push_handler(void (*handler)(char*)) {
    current_depth += 1;
    input_handlers[current_depth] = handler;
}

void console_pop_handler() {
    if (current_depth < 0) { return; }
    
    input_handlers[current_depth] = NULL;
    current_depth -= 1;
}