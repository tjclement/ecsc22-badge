/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board.h"
#include "tusb.h"

#if CFG_TUD_MSC

// whether host does safe-eject
static bool ejected = false;

// Some MCU doesn't have enough 8KB SRAM to store the whole disk
// We will use Flash as read-only disk with board that has
// CFG_EXAMPLE_MSC_READONLY defined

#define README_CONTENTS \
"=== To all survivors ===\r\n\r\n" \
"If you're reading this, it means that we have succeeded against all odds in preserving mankind's most important knowledge for those who come after us.\r\n" \
"Unfortunately, it also means that as we expected society as we know it has collapsed. We've had it a long time coming, I'm afraid.\r\n\r\n" \
"The four Knowledge Vault modules attached to this device contain some critical blueprints that are needed to revive a global civilisation. To keep this information out of the hands of malicious parties, vault contents are protected from access by security measures running on a separate TPM chip.\r\n\r\n" \
"We hope with all might that you succeed in unlocking all vaults, and give mankind a second chance to thrive.\r\n\r\n" \
"You can connect to the main interface via serial over USB using baudrate 115200."

#define CHALL1_CONTENTS \
"void unlock() { \r\n" \
"    volatile bool success = false; \r\n" \
"    if (!success) { \r\n" \
"        console_printf(\"Refusing to continue\\n\"); \r\n" \
"        return; \r\n" \
"    } else { \r\n" \
"        console_printf(\"Requesting TPM to unlock vault\\n\"); \r\n" \
"        // Communication with TPM happens here \r\n" \
"        // [..] \r\n" \
"    } \r\n" \
"} \r\n"

#define CHALL3_CONTENTS \
"static char chall3_hash[] = {\r\n" \
"    0xca, 0xec, 0x1d, 0x1a, 0xe7, 0x9d, 0x2c, 0x50,\r\n" \
"    0x21, 0x5c, 0x17, 0x14, 0x74, 0x17, 0xe8, 0xd8,\r\n" \
"    0xa3, 0x6c, 0x17, 0x8b, 0xb7, 0x83, 0xe6, 0xc0,\r\n" \
"    0x01, 0x65, 0x06, 0xda, 0x95, 0x55, 0xd3, 0x26,\r\n" \
"    0x10, 0x7a, 0x50, 0xac, 0x50, 0x58, 0xad, 0x0b,\r\n" \
"    0x44, 0xa9, 0x25, 0xa4, 0x11, 0x58, 0x64, 0x21,\r\n" \
"    0x7b, 0x5e, 0xb5, 0x6b, 0x6c, 0xa4, 0xfe, 0x66,\r\n" \
"    0xc8, 0x10, 0x29, 0xff, 0x7d, 0x8f, 0x20, 0x1c \r\n" \
"};\r\n" \
"\r\n" \
"void keygen(uint8_t * key) {\r\n" \
"    interp_config cfg = interp_default_config();\r\n" \
"    interp_set_config(interp0, 1, &cfg);\r\n" \
"    interp_config_set_blend(&cfg, true);\r\n" \
"    interp_set_config(interp0, 0, &cfg);\r\n" \
"\r\n" \
"    interp0->base[0] = *((uint32_t *)0x61);\r\n" \
"    interp0->base[1] = *((uint32_t *)0x61 + 1);\r\n" \
"\r\n" \
"    for(int i = 0; i < 64; i++) {\r\n" \
"        interp0->accum[1] = 255 * i / 64;\r\n" \
"        uint32_t *div = (uint32_t *)&(interp0->peek[1]);\r\n" \
"        uint8_t *b = (uint8_t*)(div);\r\n" \
"        key[i] = b[0] + b[1] + b[2] + b[3];\r\n" \
"    }\r\n" \
"}\r\n" \
"\r\n" \
"void interpolarize(io_rw_32 accum0, io_rw_32 base0, io_rw_32 accum1, io_rw_32 base1) {\r\n" \
"    interp_config cfg = interp_default_config();\r\n" \
"    interp_set_config(interp0, 0, &cfg);\r\n" \
"    interp_set_config(interp0, 1, &cfg);\r\n" \
"    \r\n" \
"    interp_set_config(interp1, 0, &cfg);\r\n" \
"    interp_set_config(interp1, 1, &cfg);\r\n" \
"    \r\n" \
"    interp0->accum[0] = accum0;\r\n" \
"    interp0->base[0] = base0;\r\n" \
"    interp0->accum[1] = accum1;\r\n" \
"    interp0->base[1] = base1;\r\n" \
"\r\n" \
"    for(int i = 0; i < 63; i++) {\r\n" \
"        uint8_t *refa = (uint8_t*)(interp0->peek[0]);\r\n" \
"        uint8_t *refb = (uint8_t*)(interp0->pop[1]);\r\n" \
"        \r\n" \
"        interp1->accum[0] = *refa;\r\n" \
"        interp1->accum[1] = *refb;\r\n" \
"        \r\n" \
"        *refa = interp1->pop[2];\r\n" \
"    }\r\n" \
"}\r\n" \
"\r\n" \
"void hash(uint8_t *key, const char *input, uint8_t *output) {\r\n" \
"    int len = strlen(input);\r\n" \
"    memcpy(output, input, 64);\r\n" \
"    memset(&output[len], 64-len, 64-len);\r\n" \
"    \r\n" \
"    interpolarize((io_rw_32)(output-1), 1, (io_rw_32)(key-1), 1);    \r\n" \
"    interpolarize((io_rw_32)output, 1, (io_rw_32)(output-1), 1);\r\n" \
"    interpolarize((io_rw_32)(output+63), -1, (io_rw_32)(output+64), -1);\r\n" \
"}\r\n" \
"\r\n" \
"void chall3_handler(char *input, int len) {\r\n" \
"    char key[64];\r\n" \
"    char output[64];\r\n" \
"\r\n" \
"    if (input[len-2] == CARRIAGE_RETURN) { \r\n" \
"        input[len-2] = '\0';\r\n" \
"        len -= 2;\r\n" \
"    } else if (input[len-1] == CARRIAGE_RETURN || input[len-1] == NEWLINE) {\r\n" \
"        input[len-1] = '\0';\r\n" \
"        len -= 1;\r\n" \
"    }\r\n" \
"\r\n" \
"    if (len <= 0 || len >= (64)) {\r\n" \
"        uart_printf(\"Input needs to > 0 and < 64 characters\\r\\n\");\r\n" \
"        return;\r\n" \
"    }\r\n" \
"\r\n" \
"    keygen(key);\r\n" \
"    hash(key, input, output);\r\n" \
"       \r\n" \
"    if (memcmp(output, chall3_hash, 64) == 0) {\r\n" \
"        uart_printf(\"Winner winner\\r\\n\");\r\n" \
"    } else {\r\n" \
"        uart_printf(\"Nope.\\r\\n\");\r\n" \
"     return;\r\n" \
"    }\r\n" \
"\r\n" \
"    // [..] successfully unlocked\r\n" \
"}\r\n" \

#define CHALL4_CONTENTS \
"void unlock(int key) {\r\n" \
"    uart_printf(\"Unlock function located at 0x%p\\r\\n\", unlock);\r\n" \
"\r\n" \
"    if(key != 42) { uart_printf(\"Wrong key\\r\\n\"); return; }\r\n" \
"    char flag[48] = {0};\r\n" \
"\r\n" \
"    if(key != 42) { uart_printf(\"Wrong key\\r\\n\"); return; }\r\n" \
"\r\n" \
"    //[..]\r\n" \
"\r\n" \
"    if(key != 42) { uart_printf(\"Wrong key\\r\\n\"); return; }\r\n" \
"    uart_printf(\"Unlocked vault: %s\\r\\n\", flag);\r\n" \
"}\r\n\r\n" \
"void chall4_handler(char *input, int len) {\r\n" \
"    char cmd[8];\r\n" \
"    memcpy(cmd, input, len);\r\n" \
"\r\n" \
"    if (strnstr(cmd, \"VERSION\", 8) != 0) {\r\n" \
"        version();\r\n" \
"        return;\r\n" \
"    } else if (strnstr(cmd, \"REGS\", 8) != 0) {\r\n" \
"        regs();\r\n" \
"        return;\r\n" \
"    } else if (strnstr(cmd, \"CLOCK\", 8) != 0) {\r\n" \
"        clock();\r\n" \
"        return;\r\n" \
"    } else if (strnstr(cmd, \"UNLOCK\", 8) != 0) {\r\n" \
"        unlock(0);\r\n" \
"        return;\r\n" \
"    } else if (strnstr(cmd, \"HELP\", 8) != 0) {\r\n" \
"        help_chall4();\r\n" \
"        return;\r\n" \
"    } else {\r\n" \
"        uart_printf(\"Unknown command\\r\\n\");\r\n" \
"    }\r\n" \
"    return;\r\n" \
"}\r\n" \

#define FILESIZE(a) FILESIZE_(a)
#define FILESIZE_(a) \
  (sizeof(a)-1) & 0xFF, ((sizeof(a)-1) & 0xFF00) >> 8, ((sizeof(a)-1) & 0xFF0000) >> 16, ((sizeof(a)-1) & 0xFF00) >> 24
enum
{
  DISK_BLOCK_NUM  = 16, // 8KB is the smallest size that windows allow to mount
  DISK_BLOCK_SIZE = 512
};

#ifdef CFG_EXAMPLE_MSC_READONLY
const
#endif
uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE] =
{
  //------------- Block0: Boot Sector -------------//
  // byte_per_sector    = DISK_BLOCK_SIZE; fat12_sector_num_16  = DISK_BLOCK_NUM;
  // sector_per_cluster = 1; reserved_sectors = 1;
  // fat_num            = 1; fat12_root_entry_num = 16;
  // sector_per_fat     = 1; sector_per_track = 1; head_num = 1; hidden_sectors = 0;
  // drive_number       = 0x80; media_type = 0xf8; extended_boot_signature = 0x29;
  // filesystem_type    = "FAT12   "; volume_serial_number = 0x1234; volume_label = "TinyUSB MSC";
  // FAT magic code at offset 510-511
  {
      0xEB, 0x3C, 0x90, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 0x00, 0x02, 0x01, 0x01, 0x00,
      0x01, 0x10, 0x00, 0x10, 0x00, 0xF8, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x29, 0x34, 0x12, 0x00, 0x00, 'E' , 'C' , 'S' , 'C' , ' ' ,
      '2' , '0' , '2' , '2' , ' ' , ' ' , 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x00, 0x00,

      // Zero up to 2 last bytes of FAT magic code
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
  },

  //------------- Block1: FAT12 Table -------------//
  {
    // first 2 entries must be F8FF, entry at index 0x02 is next cluster of readme file (0x03),
    // and entry at 0x03 marks end of file (0xFF8). Rinse and repeat for following files.
      0xF8, 0xFF, 0xFF, // cluster 0 and 1
      0x03, 0x80, 0xFF, // 2 and 3
      0x05, 0x80, 0xFF, // 4 and 5
      0x07, 0x80, 0x00, // 6 and 7
      0x09, 0xA0, 0x00, // 8 and 9
      0x0B, 0x80, 0xFF, // 10 and 11
      0x0D, 0x80, 0xFF, // 12 and 13
  },

  //------------- Block2: Root Directory -------------//
  {
      // first entry is volume label
      'E' , 'C' , 'S' , 'C' , ' ' , '2' , '0' , '2' , '2' , ' ' , ' ' , 0x08, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x6D, 0x65, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      
      'R' , 'E' , 'A' , 'D' , 'M' , 'E' , ' ' , ' ' , 'T' , 'X' , 'T' , 0x20, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 0x02, 0x00,
      FILESIZE(README_CONTENTS), // readme's files size (4 Bytes)
      
      'C' , 'H' , 'A' , 'L' , 'L' , '_' , '1' , ' ' , 'C' , ' ' , ' ' , 0x20, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 0x04, 0x00,
      FILESIZE(CHALL1_CONTENTS),
      
      'C' , 'H' , 'A' , 'L' , 'L' , '_' , '3' , ' ' , 'C' , ' ' , ' ' , 0x20, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 0x06, 0x00,
      FILESIZE(CHALL3_CONTENTS),
      
      'C' , 'H' , 'A' , 'L' , 'L' , '_' , '4' , ' ' , 'C' , ' ' , ' ' , 0x20, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 0x0C, 0x00,
      FILESIZE(CHALL4_CONTENTS)
  }
};

void msc_init() {
  memcpy(&msc_disk[3], README_CONTENTS, sizeof(README_CONTENTS));
  memcpy(&msc_disk[5], CHALL1_CONTENTS, sizeof(CHALL1_CONTENTS));
  memcpy(&msc_disk[7], CHALL3_CONTENTS, sizeof(CHALL3_CONTENTS));
  memcpy(&msc_disk[13], CHALL4_CONTENTS, sizeof(CHALL4_CONTENTS));
  return;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  (void) lun;

  const char vid[] = "ECSC2022";
  const char pid[] = "Revive Society";
  const char rev[] = "1337";

  memcpy(vendor_id  , vid, strlen(vid));
  memcpy(product_id , pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void) lun;

  // RAM disk is ready until ejected
  if (ejected) {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }

  return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  (void) lun;

  *block_count = DISK_BLOCK_NUM;
  *block_size  = DISK_BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void) lun;
  (void) power_condition;

  if ( load_eject )
  {
    if (start)
    {
      // load disk storage
    }else
    {
      // unload disk storage
      ejected = true;
    }
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  (void) lun;

  // out of ramdisk
  if ( lba >= DISK_BLOCK_NUM ) return -1;

  uint8_t const* addr = msc_disk[lba] + offset;
  memcpy(buffer, addr, bufsize);

  return bufsize;
}

bool tud_msc_is_writable_cb (uint8_t lun)
{
  (void) lun;

#ifdef CFG_EXAMPLE_MSC_READONLY
  return false;
#else
  return true;
#endif
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  (void) lun;

  // out of ramdisk
  if ( lba >= DISK_BLOCK_NUM ) return -1;

#ifndef CFG_EXAMPLE_MSC_READONLY
  uint8_t* addr = msc_disk[lba] + offset;
  memcpy(addr, buffer, bufsize);
#else
  (void) lba; (void) offset; (void) buffer;
#endif

  return bufsize;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
  // read10 & write10 has their own callback and MUST not be handled here

  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
      // Host is about to read/write etc ... better not to disconnect disk
      resplen = 0;
    break;

    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if ( resplen > bufsize ) resplen = bufsize;

  if ( response && (resplen > 0) )
  {
    if(in_xfer)
    {
      memcpy(buffer, response, resplen);
    }else
    {
      // SCSI output
    }
  }

  return resplen;
}

#endif
