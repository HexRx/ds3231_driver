/**
 * Driver for DS3231 high precision RTC module
 *
 * Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>\n
 * Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>\n
 * Copyright (C) 2018 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (C) 2020 HexRx <bps.programmer@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
 */

#ifndef __DS3231_H__
#define __DS3231_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "hal/hal.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define DS3231_ADDR 0x68

#define DS3231_STAT_OSCILLATOR 0x80
#define DS3231_STAT_32KHZ      0x08
#define DS3231_STAT_ALARM_2    0x02
#define DS3231_STAT_ALARM_1    0x01

#define DS3231_CTRL_OSCILLATOR    0x80
#define DS3231_CTRL_TEMPCONV      0x20
#define DS3231_CTRL_ALARM_INTS    0x04
#define DS3231_CTRL_ALARM2_INT    0x02
#define DS3231_CTRL_ALARM1_INT    0x01

#define DS3231_ALARM_WDAY   0x40
#define DS3231_ALARM_NOTSET 0x80

#define DS3231_ADDR_TIME    0x00
#define DS3231_ADDR_ALARM1  0x07
#define DS3231_ADDR_ALARM2  0x0b
#define DS3231_ADDR_CONTROL 0x0e
#define DS3231_ADDR_STATUS  0x0f
#define DS3231_ADDR_AGING   0x10
#define DS3231_ADDR_TEMP    0x11

#define DS3231_12HOUR_FLAG  0x40
#define DS3231_12HOUR_MASK  0x1f
#define DS3231_PM_FLAG      0x20
#define DS3231_MONTH_MASK   0x1f

enum {
    DS3231_SET = 0,
    DS3231_CLEAR,
    DS3231_REPLACE
};

/**
 * Alarms
 */
typedef enum {
    DS3231_ALARM_NONE = 0,//!< No alarms
    DS3231_ALARM_1,       //!< First alarm
    DS3231_ALARM_2,       //!< Second alarm
    DS3231_ALARM_BOTH     //!< Both alarms
} ds3231_alarm_t;

/**
 * First alarm rate
 */
typedef enum {
    DS3231_ALARM1_EVERY_SECOND = 0,
    DS3231_ALARM1_MATCH_SEC,
    DS3231_ALARM1_MATCH_SECMIN,
    DS3231_ALARM1_MATCH_SECMINHOUR,
    DS3231_ALARM1_MATCH_SECMINHOURDAY,
    DS3231_ALARM1_MATCH_SECMINHOURDATE
} ds3231_alarm1_rate_t;

/**
 * Second alarm rate
 */
typedef enum {
    DS3231_ALARM2_EVERY_MIN = 0,
    DS3231_ALARM2_MATCH_MIN,
    DS3231_ALARM2_MATCH_MINHOUR,
    DS3231_ALARM2_MATCH_MINHOURDAY,
    DS3231_ALARM2_MATCH_MINHOURDATE
} ds3231_alarm2_rate_t;

/**
 * Squarewave frequency
 */
typedef enum {
    DS3231_SQWAVE_1HZ    = 0x00,
    DS3231_SQWAVE_1024HZ = 0x08,
    DS3231_SQWAVE_4096HZ = 0x10,
    DS3231_SQWAVE_8192HZ = 0x18
} ds3231_sqwave_freq_t;

/**
 * @brief Initialize device descriptor
 * @param dev I2C device descriptor
 * @param port I2C port
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return true to indicate success
 */
bool ds3231_init(i2c_dev_t *dev, uint8_t port, uint8_t sda_gpio, uint8_t scl_gpio);

/**
 * @brief Free device descriptor
 * @param dev I2C device descriptor
 * @return true to indicate success
 */
bool ds3231_free_desc(i2c_dev_t *dev);

/**
 * @brief Set the time on the RTC
 *
 * Timezone agnostic, pass whatever you like.
 * I suggest using GMT and applying timezone and DST when read back.
 *
 * @return true to indicate success
 */
bool ds3231_set_time(i2c_dev_t *dev, struct tm *time);

/**
 * @brief Get the time from the RTC, populates a supplied tm struct
 * @param dev Device descriptor
 * @param[out] time RTC time
 * @return true to indicate success
 */
bool ds3231_get_time(i2c_dev_t *dev, struct tm *time);

/**
 * @brief Set alarms
 *
 * `alarm1` works with seconds, minutes, hours and day of week/month, or fires every second.
 * `alarm2` works with minutes, hours and day of week/month, or fires every minute.
 *
 * Not all combinations are supported, see `DS3231_ALARM1_*` and `DS3231_ALARM2_*` defines
 * for valid options you only need to populate the fields you are using in the `tm` struct,
 * and you can set both alarms at the same time (pass `DS3231_ALARM_1`/`DS3231_ALARM_2`/`DS3231_ALARM_BOTH`).
 *
 * If only setting one alarm just pass 0 for `tm` struct and `option` field for the other alarm.
 * If using `DS3231_ALARM1_EVERY_SECOND`/`DS3231_ALARM2_EVERY_MIN` you can pass 0 for `tm` stuct.
 *
 * If you want to enable interrupts for the alarms you need to do that separately.
 *
 * @return true to indicate success
 */
bool ds3231_set_alarm(i2c_dev_t *dev, ds3231_alarm_t alarms, struct tm *time1,
        ds3231_alarm1_rate_t option1, struct tm *time2, ds3231_alarm2_rate_t option2);

/**
 * @brief Check if oscillator has previously stopped
 *
 * E.g. no power/battery or disabled
 * sets flag to true if there has been a stop
 *
 * @param dev Device descriptor
 * @param[out] flag Stop flag
 * @return true to indicate success
 */
bool ds3231_get_oscillator_stop_flag(i2c_dev_t *dev, bool *flag);

/**
 * @brief Clear the oscillator stopped flag
 * @param dev Device descriptor
 * @return true to indicate success
 */
bool ds3231_clear_oscillator_stop_flag(i2c_dev_t *dev);

/**
 * @brief Check which alarm(s) have past
 *
 * Sets alarms to `DS3231_ALARM_NONE`/`DS3231_ALARM_1`/`DS3231_ALARM_2`/`DS3231_ALARM_BOTH`
 *
 * @param dev Device descriptor
 * @param[out] alarms Alarms
 * @return true to indicate success
 */
bool ds3231_get_alarm_flags(i2c_dev_t *dev, ds3231_alarm_t *alarms);

/**
 * @brief Clear alarm past flag(s)
 *
 * Pass `DS3231_ALARM_1`/`DS3231_ALARM_2`/`DS3231_ALARM_BOTH`
 *
 * @param dev Device descriptor
 * @param alarms Alarms
 * @return true to indicate success
 */
bool ds3231_clear_alarm_flags(i2c_dev_t *dev, ds3231_alarm_t alarms);

/**
 * @brief enable alarm interrupts (and disables squarewave)
 *
 * Pass `DS3231_ALARM_1`/`DS3231_ALARM_2`/`DS3231_ALARM_BOTH`.
 *
 * If you set only one alarm the status of the other is not changed
 * you must also clear any alarm past flag(s) for alarms with
 * interrupt enabled, else it will trigger immediately.
 *
 * @param dev Device descriptor
 * @param alarms Alarms
 * @return true to indicate success
 */
bool ds3231_enable_alarm_ints(i2c_dev_t *dev, ds3231_alarm_t alarms);

/**
 * @brief Disable alarm interrupts
 *
 * Does not (re-)enable squarewave
 *
 * @param dev Device descriptor
 * @param alarms Alarm
 * @return true to indicate success
 */
bool ds3231_disable_alarm_ints(i2c_dev_t *dev, ds3231_alarm_t alarms);

/**
 * @brief Enable the output of 32khz signal
 *
 * **Supported only by DS3231**
 *
 * @param dev Device descriptor
 * @return true to indicate success
 */
bool ds3231_enable_32khz(i2c_dev_t *dev);

/**
 * @brief Disable the output of 32khz signal
 *
 * **Supported only by DS3231**
 *
 * @param dev Device descriptor
 * @return true to indicate success
 */
bool ds3231_disable_32khz(i2c_dev_t *dev);

/**
 * @brief Enable the squarewave output
 *
 * Disables alarm interrupt functionality.
 *
 * @param dev Device descriptor
 * @return true to indicate success
 */
bool ds3231_enable_squarewave(i2c_dev_t *dev);

/**
 * @brief Disable the squarewave output
 *
 * Which re-enables alarm interrupts, but individual alarm interrupts also
 * need to be enabled, if not already, before they will trigger.
 *
 * @param dev Device descriptor
 * @return true to indicate success
 */
bool ds3231_disable_squarewave(i2c_dev_t *dev);

/**
 * @brief Set the frequency of the squarewave output
 *
 * Does not enable squarewave output.
 *
 * @param dev Device descriptor
 * @param freq Squarewave frequency
 * @return true to indicate success
 */
bool ds3231_set_squarewave_freq(i2c_dev_t *dev, ds3231_sqwave_freq_t freq);

/**
 * @brief Get the raw temperature value
 *
 * **Supported only by DS3231**
 *
 * @param dev Device descriptor
 * @param[out] temp Raw temperature value
 * @return true to indicate success
 */
bool ds3231_get_raw_temp(i2c_dev_t *dev, int16_t *temp);

/**
 * @brief Get the temperature as an integer
 *
 * **Supported only by DS3231**
 *
 * @param dev Device descriptor
 * @param[out] temp Temperature, degrees Celsius
 * @return true to indicate success
 */
bool ds3231_get_temp_integer(i2c_dev_t *dev, int8_t *temp);

/**
 * @brief Get the temperature as a float
 *
 * **Supported only by DS3231**
 *
 * @param dev Device descriptor
 * @param[out] temp Temperature, degrees Celsius
 * @return true to indicate success
 */
bool ds3231_get_temp_float(i2c_dev_t *dev, float *temp);

#ifdef	__cplusplus
}
#endif

#endif  /* __DS3231_H__ */
