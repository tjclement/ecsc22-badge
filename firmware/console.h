#ifndef CONSOLE_H
#define CONSOLE_H

#define CTRL_C 0x03
#define CTRL_D 0x04
#define ESCAPE 0x1B
#define ENTER  0x0D

void console_setup();
void console_task();
void console_clear();
void console_printf(const char* format, ...);
void console_push_handler(void (*handler)(char*));
void console_pop_handler();

#endif
