#ifndef CONSOLE_H
#define CONSOLE_H

void console_setup();
void console_task();
void console_clear();
void console_printf(const char* format, ...);
void console_push_handler(void (*handler)(char*));
void console_pop_handler();

#endif
