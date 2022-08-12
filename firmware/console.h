#pragma once

void console_init();
void console_printf(const char* format, ...);
void console_push_handler(void (*handler)(char*));
void console_pop_handler();