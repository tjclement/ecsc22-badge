#include <stdbool.h>
#include "hardware/watchdog.h"
#include "console.h"
#include "uart.h"

__attribute__ ((optimize(0)))
static void unlock() {
    volatile bool success = true;
    if (!success) {
        console_printf("Refusing to continue\r\n");
        return;
    } else {
        console_printf("Requesting TPM to unlock vault\r\n");
        uart_printf("1421337");
    }
}

static void chall1_input_handler(char *input, int len) {
    if (input[0] == CTRL_C) {
        watchdog_reboot(0, 0, 0);
    } else if (input[0] == CARRIAGE_RETURN) {
        unlock();
    }
}

static console_handler_t console_handler = {
    chall1_input_handler,
    .echo_input = false,
    .wait_for_newline = true
};

void chall1() {
    console_clear();
    console_push_handler(console_handler);
    console_printf("This vault is locked by a check that is never true. See chall_1.c for details.\r\n\r\nPress enter to attempt to unlock, or CTRL+C to exit.\r\n");
}