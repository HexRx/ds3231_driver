/*
 * Driver for DS3231 high precision RTC module
 *
 * Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>
 * Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>
 * Copyright (C) 2018 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (C) 2020 HexRx <bps.programmer@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
*/

#include "ds3231.h"
#include "hal/hal.h"

/* Convert binary coded decimal to normal decimal */
static uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

/* Convert normal decimal to binary coded decimal */
static uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

bool ds3231_init(i2c_dev_t *dev, uint8_t port, uint8_t sda_gpio, uint8_t scl_gpio)
{
    dev->port = port;
    dev->addr = DS3231_ADDR;
    dev->sda_io_num = sda_gpio;
    dev->scl_io_num = scl_gpio;

    return hal_i2c_init(dev);
}

bool ds3231_free(i2c_dev_t *dev)
{
    return hal_i2c_free(dev);
}

bool ds3231_set_time(i2c_dev_t *dev, struct tm *time)
{
    uint8_t data[7];

    /* time/date data */
    data[0] = dec2bcd(time->tm_sec);
    data[1] = dec2bcd(time->tm_min);
    data[2] = dec2bcd(time->tm_hour);
    /* The week data must be in the range 1 to 7, and to keep the start on the
     * same day as for tm_wday have it start at 1 on Sunday. */
    data[3] = dec2bcd(time->tm_wday + 1);
    data[4] = dec2bcd(time->tm_mday);
    data[5] = dec2bcd(time->tm_mon + 1);
    data[6] = dec2bcd(time->tm_year - 2000);

    return hal_i2c_write_reg(dev, DS3231_ADDR_TIME, data, 7);
}

bool ds3231_get_time(i2c_dev_t *dev, struct tm *time)
{
    uint8_t data[7];

    /* read time */
    hal_i2c_read_reg(dev, DS3231_ADDR_TIME, data, 7);

    /* convert to unix time structure */
    time->tm_sec = bcd2dec(data[0]);
    time->tm_min = bcd2dec(data[1]);
    if (data[2] & DS3231_12HOUR_FLAG) {
        /* 12H */
        time->tm_hour = bcd2dec(data[2] & DS3231_12HOUR_MASK) - 1;
        /* AM/PM? */
        if (data[2] & DS3231_PM_FLAG) {
            time->tm_hour += 12;
        }
    } else {
        time->tm_hour = bcd2dec(data[2]); /* 24H */
    }
    time->tm_wday = bcd2dec(data[3]) - 1;
    time->tm_mday = bcd2dec(data[4]);
    time->tm_mon  = bcd2dec(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = bcd2dec(data[6]) + 2000;
    time->tm_isdst = 0;

    return true;
}

bool ds3231_set_alarm(i2c_dev_t *dev, ds3231_alarm_t alarms, struct tm *time1, ds3231_alarm1_rate_t option1,
        struct tm *time2, ds3231_alarm2_rate_t option2)
{
    int i = 0;
    uint8_t data[7];

    /* alarm 1 data */
    if (alarms != DS3231_ALARM_2) {
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SEC ? dec2bcd(time1->tm_sec) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMIN ? dec2bcd(time1->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMINHOUR ? dec2bcd(time1->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 == DS3231_ALARM1_MATCH_SECMINHOURDAY ? (dec2bcd(time1->tm_wday + 1) & DS3231_ALARM_WDAY) :
            (option1 == DS3231_ALARM1_MATCH_SECMINHOURDATE ? dec2bcd(time1->tm_mday) : DS3231_ALARM_NOTSET));
    }

    /* alarm 2 data */
    if (alarms != DS3231_ALARM_1) {
        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MIN ? dec2bcd(time2->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MINHOUR ? dec2bcd(time2->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 == DS3231_ALARM2_MATCH_MINHOURDAY ? (dec2bcd(time2->tm_wday + 1) & DS3231_ALARM_WDAY) :
            (option2 == DS3231_ALARM2_MATCH_MINHOURDATE ? dec2bcd(time2->tm_mday) : DS3231_ALARM_NOTSET));
    }

    return hal_i2c_write_reg(dev, (alarms == DS3231_ALARM_2 ? DS3231_ADDR_ALARM2 : DS3231_ADDR_ALARM1), data, i);
}

/* Get a byte containing just the requested bits
 * pass the register address to read, a mask to apply to the register and
 * an uint* for the output
 * you can test this value directly as true/false for specific bit mask
 * of use a mask of 0xff to just return the whole register byte
 * returns true to indicate success
 */
static bool ds3231_get_flag(i2c_dev_t *dev, uint8_t addr, uint8_t mask, uint8_t *flag)
{
    uint8_t data;

    /* get register */
    bool res = hal_i2c_read_reg(dev, addr, &data, 1);
    if (res != true) {
        return res;
    }

    /* return only requested flag */
    *flag = (data & mask);
    return true;
}

/* Set/clear bits in a byte register, or replace the byte altogether
 * pass the register address to modify, a byte to replace the existing
 * value with or containing the bits to set/clear and one of
 * DS3231_SET/DS3231_CLEAR/DS3231_REPLACE
 * returns true to indicate success
 */
static uint8_t ds3231_set_flag(i2c_dev_t *dev, uint8_t addr, uint8_t bits, uint8_t mode)
{
    uint8_t data;

    /* get status register */
    bool res = hal_i2c_read_reg(dev, addr, &data, 1);
    if (res != true) {
        return res;
    }
    /* clear the flag */
    if (mode == DS3231_REPLACE) {
        data = bits;
    } else if (mode == DS3231_SET) {
        data |= bits;
    } else {
        data &= ~bits;
    }

    return hal_i2c_read_reg(dev, addr, &data, 1);
}

bool ds3231_get_oscillator_stop_flag(i2c_dev_t *dev, bool *flag)
{
    uint8_t f = 0;

    ds3231_get_flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, &f);

    *flag = (f ? true : false);

    return true;
}

bool ds3231_clear_oscillator_stop_flag(i2c_dev_t *dev)
{
    return ds3231_set_flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, DS3231_CLEAR);
}

bool ds3231_get_alarm_flags(i2c_dev_t *dev, ds3231_alarm_t *alarms)
{
    return ds3231_get_flag(dev, DS3231_ADDR_STATUS, DS3231_ALARM_BOTH, (uint8_t *)alarms);
}

bool ds3231_clear_alarm_flags(i2c_dev_t *dev, ds3231_alarm_t alarms)
{
    return ds3231_set_flag(dev, DS3231_ADDR_STATUS, alarms, DS3231_CLEAR);
}

bool ds3231_enable_alarm_ints(i2c_dev_t *dev, ds3231_alarm_t alarms)
{
    return ds3231_set_flag(dev, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS | alarms, DS3231_SET);
}

bool ds3231_disable_alarm_ints(i2c_dev_t *dev, ds3231_alarm_t alarms)
{

    /* Just disable specific alarm(s) requested
     * does not disable alarm interrupts generally (which would enable the squarewave)
     */
    return ds3231_set_flag(dev, DS3231_ADDR_CONTROL, alarms, DS3231_CLEAR);
}

bool ds3231_enable_32khz(i2c_dev_t *dev)
{
    return ds3231_set_flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_SET);
}

bool ds3231_disable_32khz(i2c_dev_t *dev)
{
    return ds3231_set_flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_CLEAR);
}

bool ds3231_enable_squarewave(i2c_dev_t *dev)
{
    return ds3231_set_flag(dev, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_CLEAR);
}

bool ds3231_disable_squarewave(i2c_dev_t *dev)
{
    return ds3231_set_flag(dev, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_SET);
}

bool ds3231_set_squarewave_freq(i2c_dev_t *dev, ds3231_sqwave_freq_t freq)
{
    uint8_t flag = 0;

    ds3231_get_flag(dev, DS3231_ADDR_CONTROL, 0xff, &flag);
    flag &= ~DS3231_SQWAVE_8192HZ;
    flag |= freq;
    ds3231_set_flag(dev, DS3231_ADDR_CONTROL, flag, DS3231_REPLACE);

    return true;
}

bool ds3231_get_raw_temp(i2c_dev_t *dev, int16_t *temp)
{
    uint8_t data[2];

    if (hal_i2c_read_reg(dev, DS3231_ADDR_TEMP, data, sizeof(data)) == true) {
        *temp = (int16_t)(int8_t)data[0] << 2 | data[1] >> 6;
        return true;
    }

    return false;
}

bool ds3231_get_temp_integer(i2c_dev_t *dev, int8_t *temp)
{
    int16_t t_int;

    bool res = ds3231_get_raw_temp(dev, &t_int);
    if (res == true) {
        *temp = t_int >> 2;
    }

    return res;
}

bool ds3231_get_temp_float(i2c_dev_t *dev, float *temp)
{
    int16_t t_int;

    bool res = ds3231_get_raw_temp(dev, &t_int);
    if (res == true)
        *temp = t_int * 0.25;

    return res;
}
