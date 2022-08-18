#ifndef UART_H
#define UART_H

#define UART_ID uart0

void uart_setup();
void uart_printf(const char* format, ...);
void uart_write(uint8_t* data, unsigned int length);
void uart_push_handler(void (*handler)(char*));
void uart_pop_handler();

#endif
