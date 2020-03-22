/**
 * I2C Hardware Abstraction Layer for nRF5x (nRF5_SDK)
 *
 * Copyright (C) 2020 HexRx <bps.programmer@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
 */

#include "hal.h"
#include <string.h>

#include "nrf_drv_twi.h"

/* TWI instance ID. */
#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID     1
#endif

static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

bool hal_i2c_init(const i2c_dev_t *dev)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = dev->scl_io_num,
       .sda                = dev->sda_io_num,
       .frequency          = NRF_DRV_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);

    return true;
}

bool hal_i2c_free(const i2c_dev_t *dev)
{
    return true;
}

bool hal_i2c_write_reg(const i2c_dev_t *dev, uint8_t reg, const void *out_data, size_t out_size)
{
    uint8_t data[out_size + 1];
    data[0] = reg;
    memcpy(data + 1, out_data, out_size);
    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, dev->addr, data, out_size + 1, false);
    APP_ERROR_CHECK(err_code);
    if (err_code == NRF_SUCCESS) {
        return false;
    }
    return true;
}

bool hal_i2c_read_reg(const i2c_dev_t *dev, uint8_t reg, void *in_data, size_t in_size)
{
    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, dev->addr, &reg, 1, true);
    if (err_code == NRF_SUCCESS) {
        err_code = nrf_drv_twi_rx(&m_twi, dev->addr, in_data, in_size);
    }
    APP_ERROR_CHECK(err_code);
    return true;
}