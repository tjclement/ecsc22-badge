/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

#include "nvs.h"
#include "uart.h"
#include "interpolator.h"
#include "console.h"


// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define STATUS_PIN (26)

#define ROM_VERSION *(uint8_t*)0x13
#define CHIP_VERSION ((*(io_ro_32*)0x40000000) & 0xf0000000) >> 28
#define CORE_NUM *(uint32_t *)(0xd0000000)


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

static char chall3_hash[] = {
    0xca, 0xec, 0x1d, 0x1a, 0xe7, 0x9d, 0x2c, 0x50,
    0x21, 0x5c, 0x17, 0x14, 0x74, 0x17, 0xe8, 0xd8,
    0xa3, 0x6c, 0x17, 0x8b, 0xb7, 0x83, 0xe6, 0xc0,
    0x01, 0x65, 0x06, 0xda, 0x95, 0x55, 0xd3, 0x26,
    0x10, 0x7a, 0x50, 0xac, 0x50, 0x58, 0xad, 0x0b,
    0x44, 0xa9, 0x25, 0xa4, 0x11, 0x58, 0x64, 0x21,
    0x7b, 0x5e, 0xb5, 0x6b, 0x6c, 0xa4, 0xfe, 0x66,
    0xc8, 0x10, 0x29, 0xff, 0x7d, 0x8f, 0x20, 0x1c 
};

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

void help_chall4() {
    uart_printf("Available commands:\r\n");
    uart_printf(" * HELP    - this menu\r\n");
    uart_printf(" * VERSION - get version info\r\n");
    uart_printf(" * CLOCK   - clock info\r\n");
    uart_printf(" * REGS    - dump registers\r\n");
    uart_printf(" * UNLOCK  - unlock this vault\r\n");
    uart_printf(" * EXIT    - exit\r\n");
}

void version() {
    uart_printf("Version function located at 0x%p\r\n", version);
    uart_printf("ROM version: %d\r\n", ROM_VERSION);
    uart_printf("Chip version: %d (RP2-B2)\r\n", CHIP_VERSION);
}

void clock() {
    uart_printf("Clock function located at 0x%p\r\n", clock);
    uart_printf("Core number: %d\r\n", CORE_NUM);
    uart_printf("Clock (REF): %d Hz\r\n", clock_get_hz(4));
    uart_printf("Clock (SYS): %d Hz\r\n", clock_get_hz(5));
}

__attribute__ ((optimize(0)))
void unlock(int key) {
    uart_printf("Unlock function located at 0x%p\r\n", unlock);

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }
    char flag[48] = {0};

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }
    strcpy(flag, flags[3]);

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }
    got_flags[3] = 1;

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }
    nvs_write(0, got_flags, 4);
    set_chall_leds();

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }
    uart_printf("Unlocked vault: %s\r\n", flag);
}

void regs() {
    uart_printf("Register dump function located at 0x%p\r\n", regs);
    unsigned int reg[16];
    int i = 0;
    
    __asm__ volatile ("MOV %[output], R0"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R1"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R2"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R3"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R4"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R5"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R6"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R7"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R8"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R9"  : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R10" : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R11" : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R12" : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R13" : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R14" : [output] "=r" (reg[i++]));
    __asm__ volatile ("MOV %[output], R15" : [output] "=r" (reg[i++]));

    uart_printf("\r\n");
    for (i = 0; i < 16; i++) {
        char * c = i < 10 ? " " : "";
        uart_printf("R%d:%s %08x\t", i, c, reg[i]);
        if (i % 4 == 3) puts("");
    }
    uart_printf("\r\n");
}

void chall1_handler(char *input, int len) {
    if (strnstr(input, "421337", 7)) {
        got_flags[0] = 1;
        nvs_write(0, got_flags, 4);
        uart_printf("Unlocked vault with flag %s\r\n", flags[0]);
    }
}

void chall2_handler(char *input, int len) {
    if (strlen(input) < 3) { return; }

    if (input[0] != 0x06) {
        uart_printf("Unknown main command");
        return;
    }

    if (input[1] != 0x8F) {
        uart_printf("Unknown subcommand");
        return;
    }


    got_flags[1] = 1;
    nvs_write(0, got_flags, 4);
    uart_printf("Unlocked vault: %s\r\n", flags[1]);
}

__attribute__((optimize(0)))
void chall3_handler(char *input, int len) {
    char key[64];
    char output[64];

    if (input[len-2] == CARRIAGE_RETURN) { 
        input[len-2] = '\0';
        len -= 2;
    } else if (input[len-1] == CARRIAGE_RETURN || input[len-1] == NEWLINE) {
        input[len-1] = '\0';
        len -= 1;
    }

    if (len <= 0 || len >= (64)) {
        uart_printf("Input needs to > 0 and < 64 characters\r\n");
        return;
    }

    keygen(key);
    hash(key, input, output);
       
       if (memcmp(output, chall3_hash, 64) == 0) {
           uart_printf("Winner winner\r\n");
       } else {
           uart_printf("Nope.\r\n");
        return;
       }
       
    got_flags[2] = 1;
    nvs_write(0, got_flags, 4);
    uart_printf("Unlocked vault: %s\r\n", flags[2]);
}

void chall4_handler(char *input, int len) {
    char cmd[8];
    memcpy(cmd, input, len);

    if (strnstr(cmd, "VERSION", 8) != 0) {
        version();
        return;
    } else if (strnstr(cmd, "REGS", 8) != 0) {
        regs();
        return;
    } else if (strnstr(cmd, "CLOCK", 8) != 0) {
        clock();
        return;
    } else if (strnstr(cmd, "UNLOCK", 8) != 0) {
        unlock(0);
        return;
    } else if (strnstr(cmd, "HELP", 8) != 0) {
        help_chall4();
        return;
    } else {
        uart_printf("Unknown command\r\n");
    }
    return;
}

void uart_handler(char *input, int len) {
    busy_wait_us(5000);

    switch (input[0]) {
        case '1':
            chall1_handler(input+1, len-1);
        break;
        case '2':
            chall2_handler(input+1, len-1);
        break;
        case '3':
            chall3_handler(input+1, len-1);
        break;
        case '4':
            chall4_handler(input+1, len-1);
        break;
        case 'd':
            if (strnstr(input, "status", 16) != 0) {
                uart_write(got_flags, 4);
                uart_printf("flagbits:%d%d%d%d\r\n", got_flags[0], got_flags[1], got_flags[2], got_flags[3]);
            } else if (strnstr(input, "flags", 16) != 0) {
                for (int i = 0; i < 4; i++) {
                    if (got_flags[i]) {
                        printf("Flag %d: %s\r\n", i, flags[i]);
                    }
                }
            }
        break;
        /*case 'c':
            if (strnstr(input, "gibflagsbls", 16) != 0) {
                *((int*)got_flags) = 0x01010101;
                nvs_write(0, got_flags, 4);
            }
        break;*/
        case 'r':
            *((int*)got_flags) = 0x0;
            nvs_write(0, got_flags, 4);
        break;
        case 'f':
            if (strnstr(input, "gibusbpl0x", 16) != 0) {
                uart_printf("Rebooting into USB bootloader\r\n");
                reset_usb_boot(0, 0);
            }
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

    while (1) {
        set_chall_leds();
        uart_task();
    }
}
