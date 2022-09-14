#include <stdbool.h>
#include "hardware/watchdog.h"
#include "console.h"
#include "uart.h"
#include "msc_disk.h"
#include "tusb.h"

static void chall4_input_handler(char *input, int len) {
    if (input[0] == CTRL_C) {
        watchdog_reboot(0, 0, 0);
    }
    
    char buf[len+1];
    strcpy(buf, "4");
    memcpy(buf+1, input, len);
    
    console_printf("\r\n");
    uart_write((uint8_t*)buf, len+1);
}

static console_handler_t console_handler = {
    chall4_input_handler,
    .echo_input = true,
    .wait_for_newline = true
};

void chall4() {
    console_clear();
    console_push_handler(console_handler);
    console_printf("This vault is locked on the TPM chip. Type a command followed by enter to execute. The unlock function refuses to unlock.\r\nSee chall_4.c for details. You can send linefeed-terminated raw input to this serial device directly. Press CTRL+C to exit.\r\n");
    uart_printf("4HELP\r\n");
}