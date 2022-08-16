#include <stdbool.h>
#include "hardware/watchdog.h"
#include "console.h"
#include "uart.h"

void unlock() {
    volatile bool success = false;
    if (!success) {
        console_printf("Refusing to continue\n");
        return;
    } else {
        console_printf("Requesting TPM to unlock vault\n");
        uart_printf("1421337");
    }
}

void chall1_input_handler(char *input) {
    if (input[0] == ESCAPE || input[0] == CTRL_C) {
        watchdog_reboot(0, 0, 0);
    } else if (input[0] == ENTER) {
        unlock();
    }
}

void chall1() {
    console_clear();
    console_push_handler(chall1_input_handler);
    console_printf("This vault is locked by a check that is never true. See chall1.c for details.");
}