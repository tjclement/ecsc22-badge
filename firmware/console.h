#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define CTRL_C          (0x03)
#define CTRL_D          (0x04)
#define ESCAPE          (0x1B)
#define NEWLINE         (0x0A)
#define CARRIAGE_RETURN (0x0D)

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define MAX_HANDLERS (16)
#define READ_BUFFER_SIZE (4096)

typedef struct {
    void (*callback)(char*, int);
    bool echo_input;
    bool wait_for_newline;
} console_handler_t;

void console_setup();
void console_task();
void console_clear();
void console_printf(const char* format, ...);
void console_push_handler(console_handler_t handler);
void console_pop_handler();

#endif
