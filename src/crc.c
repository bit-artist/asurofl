/* See LICENSE file for copyright and license details. */

/*
 * Source code generated by pycrc v0.9.2
 *
 * Pycrc is a free Cyclic Redundancy Check (CRC) C source code generator written in Python
 * and is released under the terms of the MIT licence.
 *
 * Copyright of the generated source code
 *
 * The code generated by pycrc is not considered a substantial portion of the software,
 * therefore the licence does not cover the generated code, and the author of pycrc will
 * not claim any copyright on the generated code.
 *
 * This information was retrieved on November 7, 2020 from https://pycrc.org
 *
 */

/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Sat Nov  7 16:34:00 2020
 * by pycrc v0.9.2, https://pycrc.org
 * using the configuration:
 *  - Width         = 16
 *  - Poly          = 0x8005
 *  - XorIn         = 0x0000
 *  - ReflectIn     = True
 *  - XorOut        = 0x0000
 *  - ReflectOut    = True
 *  - Algorithm     = bit-by-bit
 */
#include "crc.h"     /* include the header file generated with pycrc */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static crc_t crc_reflect(crc_t data, size_t data_len);



crc_t crc_reflect(crc_t data, size_t data_len)
{
    unsigned int i;
    crc_t ret;

    ret = data & 0x01;
    for (i = 1; i < data_len; i++) {
        data >>= 1;
        ret = (ret << 1) | (data & 0x01);
    }
    return ret;
}


crc_t crc_update(crc_t crc, const void *data, size_t data_len)
{
    const unsigned char *d = (const unsigned char *)data;
    unsigned int i;
    bool bit;
    unsigned char c;

    while (data_len--) {
        c = crc_reflect(*d++, 8);
        for (i = 0; i < 8; i++) {
            bit = crc & 0x8000;
            crc = (crc << 1) | ((c >> (7 - i)) & 0x01);
            if (bit) {
                crc ^= 0x8005;
            }
        }
        crc &= 0xffff;
    }
    return crc & 0xffff;
}


crc_t crc_finalize(crc_t crc)
{
    unsigned int i;
    bool bit;

    for (i = 0; i < 16; i++) {
        bit = crc & 0x8000;
        crc <<= 1;
        if (bit) {
            crc ^= 0x8005;
        }
    }
    crc = crc_reflect(crc, 16);
    return crc & 0xffff;
}