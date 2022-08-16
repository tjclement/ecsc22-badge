/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "nvs.h"
#include "uart.h"


/// \tag::uart_advanced[]

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define LED_PIN (25)

static uint8_t got_flags[4] = {0,0,0,0};

void uart_handler(char *input) {
    uart_printf("%s", input);
    if (strnstr(input, "c", 3)) {
        *((int*)got_flags) = 0x01010101;
        nvs_write(0, got_flags, 4);
        nvs_read(0, got_flags, 4);
        uart_printf("Flags: %p", *(int*)(got_flags));
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    uart_setup();
    uart_push_handler(uart_handler);

    nvs_read(0, got_flags, 4);
    uart_printf("Flags: %p", *(int*)(got_flags));

    uart_printf("\nTPM bootrom B2 version\n");

    while (1) {
        tight_loop_contents();
    }
}

/// \end:uart_advanced[]
