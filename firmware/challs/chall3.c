#include <stdbool.h>
#include "hardware/watchdog.h"
#include "console.h"
#include "uart.h"
#include "msc_disk.h"
#include "tusb.h"

static void chall_input_handler(char *input) {
    if (input[0] == CTRL_C) {
        watchdog_reboot(0, 0, 0);
    }
}

void chall3() {
    console_clear();
    console_push_handler(chall_input_handler);
    console_printf("Press CTRL+C to exit.");
}