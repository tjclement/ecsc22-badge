#include <stdbool.h>
#include "hardware/watchdog.h"
#include "console.h"
#include "uart.h"

__attribute__ ((optimize(0)))
static void unlock() {
    volatile bool success = false;
    if (!success) {
        console_printf("Refusing to continue\r\n");
        return;
    } else {
        console_printf("Requesting TPM to unlock vault\r\n");
        uart_printf("1421337");
    }
}

static void chall1_input_handler(char *input) {
    if (input[0] == CTRL_C) {
        watchdog_reboot(0, 0, 0);
    } else if (input[0] == ENTER) {
        unlock();
    }
}

void chall1() {
    console_clear();
    console_push_handler(chall1_input_handler);
    console_printf("This vault is locked by a check that is never true. See chall1.c for details.\r\n\r\nPress enter to attempt to unlock, or CTRL+C to exit.\r\n");
}