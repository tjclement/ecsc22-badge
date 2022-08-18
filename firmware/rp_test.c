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

int main() {
    stdio_init_all();

    printf("Booted\n");

    while (1) {
        for (int i = 22; i < 30; i++) {
            gpio_init(i);
            gpio_set_dir(i, GPIO_OUT);
            gpio_put(i, 1);
            sleep_ms(300);
            gpio_put(i, 0);
        }
    }
}

/// \end:uart_advanced[]
