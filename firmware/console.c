#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "console.h"

static char read_buffer[READ_BUFFER_SIZE];
static int cur_write_index = 0;
static console_handler_t input_handlers[MAX_HANDLERS];
static int current_depth = -1;

void default_handler(char *input, int len) {
    if (strnstr(input, "\033[", 3) != NULL) {
        // Don't print ANSI escape codes
        return;
    }

    printf("%s", input);

    // Convert \r to \r\n
    if (input[len-1] == CARRIAGE_RETURN) {
        printf("\n");
    }
}

console_handler_t default_handler_struct = {
    default_handler,
    .wait_for_newline = false
};

void console_setup() {
    current_depth = 0;
    input_handlers[current_depth] = default_handler_struct;
}

void console_task() {
    int ch;
    int chars_read = 0;

    if (current_depth < 0) { 
        return;
    }

    console_handler_t handler = input_handlers[current_depth];

    while ((ch = getchar_timeout_us(500)) != PICO_ERROR_TIMEOUT) {
        read_buffer[cur_write_index++] = (char) ch;
        chars_read++;
        if (handler.echo_input) {
            putchar(ch);
        }
    }

    if ((handler.wait_for_newline && (read_buffer[cur_write_index-1] != CARRIAGE_RETURN && read_buffer[cur_write_index-1] != NEWLINE && read_buffer[cur_write_index-1] != CTRL_C)) || chars_read <= 0) {
        return;
    }

    read_buffer[cur_write_index] = '\0';
    handler.callback(read_buffer, cur_write_index);
    cur_write_index = 0;
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

void console_push_handler(console_handler_t handler) {
    current_depth += 1;
    input_handlers[current_depth] = handler;
}

void console_pop_handler() {
    if (current_depth < 0) { return; }
    current_depth -= 1;
}