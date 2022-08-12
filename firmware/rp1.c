/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "console.h"


/// \tag::uart_advanced[]

void parrot_input(char *line) {
    printf("Got input: %s", line);
}

int main() {
    stdio_init_all();
    console_init();
    console_push_handler(parrot_input);

    while (1) {
        tight_loop_contents();
    }
}

/// \end:uart_advanced[]
