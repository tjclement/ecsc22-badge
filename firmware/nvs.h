#ifndef NVS_H
#define NVS_H

#include <stdint.h>

#define FLASH_SIZE_BYTES (2*1024*1024)
#define NVS_SIZE_BYTES (4096)
#define NVS_FLASH_OFFSET ((FLASH_SIZE_BYTES) - NVS_SIZE_BYTES)
#define NVS_START ((void*)(XIP_BASE + NVS_FLASH_OFFSET))

void nvs_read(unsigned int offset, uint8_t *buffer, unsigned int len);
void nvs_write(unsigned int offset, uint8_t *data, unsigned int len);

#endif
