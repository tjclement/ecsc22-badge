/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"

#include "bsp/board.h"
#include "tusb.h"

#include "msc_disk.h"
#include "cdc.h"
#include "console.h"
#include "uart.h"
#include "menu.h"

#define STATUS_PIN (26)
#define MOUNT_PIN (27)
#define CONN_PIN (28)

#define ascii_art \
":::::::..  .,:::::::::      .::.::::::      .::..,::::::       .::::::.     ...       .,-:::::  :::.,::::::::::::::::::.-:.     ::-.\r\n" \
";;;;``;;;; ;;;;''''';;,   ,;;;' ;;;';;,   ,;;;' ;;;;''''      ;;;`    `  .;;;;;;;.  ,;;;'````'  ;;;;;;;'''';;;;;;;;'''' ';;.   ;;;;'\r\n" \
" [[[,/[[['  [[cccc  \\[[  .[[/   [[[ \\[[  .[[/    [[cccc       '[==/[[[[,,[[     \\[[,[[[         [[[ [[cccc      [[        '[[,[[['  \r\n" \
" $$$$$$c    $$\"\"\"\"   Y$c.$$\"    $$$  Y$c.$$\"     $$\"\"\"\"         '''    $$$$,     $$$$$$         $$$ $$\"\"\"\"      $$          c$$\"    \r\n" \
" 888b \"88bo,888oo,__  Y88P      888   Y88P       888oo,__      88b    dP\"888,_ _,88P`88bo,__,o, 888 888oo,__    88,       ,8P\"`     \r\n" \
" MMMM   \"W\" \"\"\"\"YUMMM  MP       MMM    MP        \"\"\"\"YUMMM      \"YMmMY\"   \"YMMMMMP\"   \"YUMMMMMP\"MMM \"\"\"\"YUMMM   MMM      mM\"        \r\n" \
"\r\n" \
"                      :::      .::.:.      .::.   .::.       ...:::::\r\n" \
"                      ';;,   ,;;;' ;;     ;'`';;,;'`';;,     '''``;;'\r\n" \
"                       \\[[  .[[/   [[        .n[[   .n[[         .[' \r\n" \
"                        Y$c.$$\"    $$       ``\"$$$.``\"$$$.     ,$$'  \r\n" \
"                         Y88P      88       ,,o888\",,o888\"     888   \r\n" \
"                          MP       MM  mmm  YMMP\"  YMMP\"  mmm  MMM  \r\n"

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void parrot_uart(char *input) {
    printf("Got input: %s", input);
}

void menu_handler(int chosen_index) {
    printf("Chosen: %d", chosen_index);
}

void cdc_connect_handler(bool connected) {
    gpio_put(CONN_PIN, connected);
    sleep_ms(50);
    menu_render();
}

void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) { return; /* not enough time */ }
  start_ms += blink_interval_ms;

  gpio_put(MOUNT_PIN, led_state);
  led_state = 1 - led_state; // toggle
}

int main() {    
    uart_setup();
    uart_push_handler(parrot_uart);

    gpio_init(STATUS_PIN);
    gpio_set_dir(STATUS_PIN, GPIO_OUT);
    gpio_put(STATUS_PIN, 1);
    gpio_init(CONN_PIN);
    gpio_set_dir(CONN_PIN, GPIO_OUT);
    gpio_init(MOUNT_PIN);
    gpio_set_dir(MOUNT_PIN, GPIO_OUT);


    msc_init();
    tusb_init();
    tud_task();
    cdc_push_connect_handler(cdc_connect_handler);
    cdc_install_stdio(false);
    console_setup();

    char *names[] = {
        "Vault 1 - Water Purification", 
        "Vault 2 - Supercomputing Architecture", 
        "Vault 3 - Wireless Communication", 
        "Vault 4 - Power Generation"};

    menu_t menu_info = {
        .instructions = \
        ascii_art \
        "\r\nYou hold in your hands the knowledge to rebuild a globalised civilisation, stored in protected vaults.\r\n" \
        " Choose a Knowledge Vault to interface with:",
        .names = names,
        .num_names = 4,
        .callback = menu_handler,
        .cur_index = 0
    };
    
    menu(&menu_info);

    while (1) {
        tud_task();
        led_blinking_task();
        console_task();
    }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

/// \end:uart_advanced[]
