#ifndef UART_H
#define UART_H

void uart_setup();
void uart_printf(const char* format, ...);
void uart_push_handler(void (*handler)(char*));
void uart_pop_handler();

#endif
