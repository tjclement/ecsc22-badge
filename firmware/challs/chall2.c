#include <stdbool.h>
#include "hardware/watchdog.h"
#include "console.h"
#include "uart.h"
#include "msc_disk.h"
#include "tusb.h"

static bool is_active = false;

static void uart_handler(char *input) {
    int input_size = strnlen(input, 1024);
    char buffer[input_size + 1 + 4];
    sprintf(buffer, "ERR: %s", input);
    tud_vendor_write_str(input);
}

static void chall2_input_handler(char *input) {
    if (input[0] == CTRL_C) {
        watchdog_reboot(0, 0, 0);
    }
}

void tud_vendor_rx_cb(uint8_t itf) {
    if (!is_active) { return; }

    if (tud_vendor_available() != 4) {
        tud_vendor_write_str("ERR: message length does not follow proprietary protocol\n");
        console_printf("ERR: message length does not follow proprietary protocol -> %d\n", tud_vendor_available());
        tud_vendor_read_flush();
        return;
    }

    char cmd[4+1] = {'2'};
    tud_vendor_read(cmd+1, 4);
    uart_write((uint8_t*) cmd, sizeof(cmd));
}

void chall2() {
    is_active = true;
    console_clear();
    console_push_handler(chall2_input_handler);
    uart_push_handler(uart_handler);
    console_printf("This vault can only be unlocked via an undocumented proprietary USB protocol. Press CTRL+C to exit.");
}