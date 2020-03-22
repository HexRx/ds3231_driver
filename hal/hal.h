/**
 * I2C Hardware Abstraction Layer
 *
 * Copyright (C) 2020 HexRx <bps.programmer@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
 */

#ifndef __HAL_H__
#define __HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * I2C device descriptor
 */
typedef struct
{
    uint8_t port;
    uint8_t scl_io_num;
    uint8_t sda_io_num;
    uint8_t addr;
} i2c_dev_t;

bool hal_i2c_init(const i2c_dev_t *dev);

bool hal_i2c_free(const i2c_dev_t *dev);

bool hal_i2c_write_reg(const i2c_dev_t *dev, uint8_t reg, const void *out_data, size_t out_size);

bool hal_i2c_read_reg(const i2c_dev_t *dev, uint8_t reg, void *in_data, size_t in_size);

#endif