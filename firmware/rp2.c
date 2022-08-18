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

#define STATUS_PIN (26)

static char* flags[] = {
    "HL{2f75dbc8-ca31-4c7a-9562-edbf7d9eaf0f}", // Chall 1
    "HL{98ca5dd3-8596-43c6-81c1-d57f18d68975}", // Chall 2
    "HL{c61a5d9c-994d-4859-95e7-d2d9cfe7a13c}", // Chall 3
    "HL{eadd44df-4e61-4767-803c-18bc5bc2b22e}"  // Chall 4
};

static int chall_pins[] = {
    25,
    24,
    23,
    22
};

static uint8_t got_flags[4] = {0,0,0,0};

void update_got_flags() {
    nvs_read(0, got_flags, 4);
}

void set_chall_leds() {
    for (int i = 0; i < 4; i++) {
        int pin = chall_pins[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, got_flags[i]);
    }
}

void chall1_handler(char *input) {
    if (strnstr(input, "1421337", 7)) {
        got_flags[0] = 1;
        nvs_write(0, got_flags, 4);
        uart_printf("Unlocked vault with flag %s\n", flags[0]);
    }
}

void chall2_handler(char *input) {
    if (strlen(input) < 3) { return; }

    if (input[1] != 0x06) {
        uart_printf("unknown main command");
        return;
    }

    if (input[2] != 0x8F) {
        uart_printf("unknown subcommand");
        return;
    }


    got_flags[1] = 1;
    nvs_write(0, got_flags, 4);
    uart_printf("Unlocked vault: %s\n", flags[0]);
}

void chall3_handler(char *input) {
    return;
}

void chall4_handler(char *input) {
    return;
}

void uart_handler(char *input) {
    uart_printf("ECHO %s\n", input);

    switch (input[0]) {
        case '1':
            chall1_handler(input);
        break;
        case '2':
            chall2_handler(input);
        break;
        case '3':
            chall3_handler(input);
        break;
        case '4':
            chall4_handler(input);
        break;
        case 'c':
            *((int*)got_flags) = 0x01010101;
            nvs_write(0, got_flags, 4);
        break;
        case 'r':
            *((int*)got_flags) = 0x0;
            nvs_write(0, got_flags, 4);
        break;
        default:
        break;
    }
}

int main() {
    stdio_init_all();

    gpio_init(STATUS_PIN);
    gpio_set_dir(STATUS_PIN, GPIO_OUT);
    gpio_put(STATUS_PIN, 1);

    uart_setup();
    uart_push_handler(uart_handler);

    update_got_flags();
    uart_printf("Flags: %p", *(int*)(got_flags));

    uart_printf("\nTPM bootrom B2 version\n");

    while (1) {
        set_chall_leds();
    }
}

/// \end:uart_advanced[]
