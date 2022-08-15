#ifndef CDC_H
#define CDC_H

void cdc_install_stdio(bool block_until_connected);
void cdc_push_connect_handler(void (*handler)(bool));
void cdc_pop_connect_handler();

#endif