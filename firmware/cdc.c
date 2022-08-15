#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "tusb.h"
#include "pico/stdio/driver.h"
#include "pico/mutex.h"

#include "cdc.h"

// PICO_CONFIG: PICO_STDIO_USB_STDOUT_TIMEOUT_US, Number of microseconds to be blocked trying to write USB output before assuming the host has disappeared and discarding data, default=500000, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_STDOUT_TIMEOUT_US
#define PICO_STDIO_USB_STDOUT_TIMEOUT_US 500000
#endif

#define MAX_HANDLERS (16)

static mutex_t stdio_usb_mutex;
static void (*connect_handlers[MAX_HANDLERS])(bool);
static int current_depth = -1;

void stdio_usb_out_chars(const char *buf, int length) {
    static uint64_t last_avail_time;
    uint32_t owner;
    if (!mutex_try_enter(&stdio_usb_mutex, &owner)) {
        if (owner == get_core_num()) return; // would deadlock otherwise
        mutex_enter_blocking(&stdio_usb_mutex);
    }
    if (tud_cdc_connected()) {
        for (int i = 0; i < length;) {
            int n = length - i;
            int avail = (int) tud_cdc_write_available();
            if (n > avail) n = avail;
            if (n) {
                int n2 = (int) tud_cdc_write(buf + i, (uint32_t)n);
                tud_task();
                tud_cdc_write_flush();
                i += n2;
                last_avail_time = time_us_64();
            } else {
                tud_task();
                tud_cdc_write_flush();
                if (!tud_cdc_connected() ||
                    (!tud_cdc_write_available() && time_us_64() > last_avail_time + PICO_STDIO_USB_STDOUT_TIMEOUT_US)) {
                    break;
                }
            }
        }
    } else {
        // reset our timeout
        last_avail_time = 0;
    }
    mutex_exit(&stdio_usb_mutex);
}

int stdio_usb_in_chars(char *buf, int length) {
    uint32_t owner;
    if (!mutex_try_enter(&stdio_usb_mutex, &owner)) {
        if (owner == get_core_num()) return PICO_ERROR_NO_DATA; // would deadlock otherwise
        mutex_enter_blocking(&stdio_usb_mutex);
    }
    int rc = PICO_ERROR_NO_DATA;
    if (tud_cdc_connected() && tud_cdc_available()) {
        int count = (int) tud_cdc_read(buf, (uint32_t) length);
        rc =  count ? count : PICO_ERROR_NO_DATA;
    }
    mutex_exit(&stdio_usb_mutex);
    return rc;
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;
    (void) rts;

    bool connected = dtr;
    if (current_depth >= 0 && connect_handlers[current_depth] != NULL) {
        connect_handlers[current_depth](connected);
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}

stdio_driver_t stdio_usb = {
    .out_chars = stdio_usb_out_chars,
    .in_chars = stdio_usb_in_chars,
    .crlf_enabled = true
};

void cdc_install_stdio(bool block_until_connected) {
    mutex_init(&stdio_usb_mutex);
    stdio_set_driver_enabled(&stdio_usb, true);

    if (!block_until_connected) { return; }

    while (!tud_cdc_connected()) {
        tud_task();
        sleep_ms(10);
    }
}

void cdc_push_connect_handler(void (*handler)(bool)) {
    current_depth += 1;
    connect_handlers[current_depth] = handler;
}

void cdc_pop_connect_handler() {
    if (current_depth < 0) { return; }

    connect_handlers[current_depth] = NULL;
    current_depth -= 1;
}