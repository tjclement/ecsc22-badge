#include "nvs.h"

#include <string.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include <hardware/flash.h>

static bool cache_cold = true;
static uint8_t cache[NVS_SIZE_BYTES];

void nvs_fill_cache() {
    memcpy(cache, NVS_START, NVS_SIZE_BYTES);
    cache_cold = false;
}

void nvs_read(unsigned int offset, uint8_t *buffer, unsigned int len) {
    if (cache_cold) {
        nvs_fill_cache();
    }

    memcpy(buffer, cache + offset, len);
}

void nvs_write(unsigned int offset, uint8_t *data, unsigned int len) {
    if (cache_cold) {
        nvs_fill_cache();
    }

    memcpy(cache + offset, data, len);
    flash_range_erase(NVS_FLASH_OFFSET, NVS_SIZE_BYTES);
    flash_range_program(NVS_FLASH_OFFSET, cache, NVS_SIZE_BYTES);

    volatile uint8_t *flash = (uint8_t*)(NVS_START);

    for (int i = 0; i < NVS_SIZE_BYTES; i++) {
        if (cache[i] != flash[i]) {
            return;
        }
    }
}
