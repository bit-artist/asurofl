/* See LICENSE file for copyright and license details. */

#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include <stdint.h>

#define ASURO_PAGE_SIZE 64
#define ASURO_MAX_PAGES 128
#define ASURO_BOOTLOADER_SIZE 1024
#define ASURO_MAX_FILE_SIZE (ASURO_PAGE_SIZE*ASURO_MAX_PAGES-ASURO_BOOTLOADER_SIZE)

/* 1 byte page number + 64 bytes page data + 2 bytes CRC */
#define ASURO_FRAME_SIZE (1+ASURO_PAGE_SIZE+2)

int get_frame(FILE *fp, uint8_t *frame);
void print_frame(const uint8_t *frame);

#endif /* SRC_UTIL_H_ */
