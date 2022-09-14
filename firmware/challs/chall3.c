#include <stdbool.h>
#include "hardware/watchdog.h"
#include "console.h"
#include "uart.h"
#include "msc_disk.h"
#include "tusb.h"

static void chall3_input_handler(char *input, int len) {
    if (input[0] == CTRL_C) {
        watchdog_reboot(0, 0, 0);
    }

    char buf[len+1];
    strcpy(buf, "3");
    memcpy(buf+1, input, len);

    console_printf("\r\n");
    uart_write((uint8_t*)buf, len+1);
}

static console_handler_t console_handler = {
    chall3_input_handler,
    .echo_input = true,
    .wait_for_newline = true
};

void chall3() {
    console_clear();
    console_push_handler(console_handler);
    console_printf("This vault is locked by a hashing mechanism using interpolator hardware.\r\nSee chall_3.c for details. You can send linefeed-terminated raw input to this serial device directly. Press CTRL+C to exit.");
}