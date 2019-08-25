/* Libre Solar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"
#include "pcb.h"
#include "helper.h"

#if defined(BMS_BQ76920) || defined(BMS_BQ76930) || defined(BMS_BQ76940)

#include "bq769x0_interface.h"
#include "bq769x0_registers.h"

int adc_gain;    // factory-calibrated, read out from chip (uV/LSB)
int adc_offset;  // factory-calibrated, read out from chip (mV)

static bool alert_interrupt_flag;   // indicates if a new current reading or an error is available from BMS IC
static time_t alert_interrupt_timestamp;

#ifdef __MBED__

#include "mbed.h"

// static (private) variables
//----------------------------------------------------------------------------

static I2C bq_i2c(PIN_BMS_SDA, PIN_BMS_SCL);
static int i2c_address;
static bool crc_enabled;

static InterruptIn alert_interrupt(PIN_BQ_ALERT);

//----------------------------------------------------------------------------

uint8_t _crc8_ccitt_update (uint8_t inCrc, uint8_t inData)
{
    uint8_t   i;
    uint8_t   data;

    data = inCrc ^ inData;

    for ( i = 0; i < 8; i++ )
    {
        if (( data & 0x80 ) != 0 )
        {
            data <<= 1;
            data ^= 0x07;
        }
        else
        {
            data <<= 1;
        }
    }
    return data;
}

//----------------------------------------------------------------------------
// The bq769x0 drives the ALERT pin high if the SYS_STAT register contains
// a new value (either new CC reading or an error)
void bq769x0_alert_isr()
{
    alert_interrupt_timestamp = uptime();
    alert_interrupt_flag = true;
}

//----------------------------------------------------------------------------

void bq769x0_write_byte(int address, int data)
{
    uint8_t crc = 0;
    char buf[3];

    buf[0] = (char) address;
    buf[1] = data;

    if (crc_enabled == true) {
        // CRC is calculated over the slave address (including R/W bit), register address, and data.
        crc = _crc8_ccitt_update(crc, (i2c_address << 1) | 0);
        crc = _crc8_ccitt_update(crc, buf[0]);
        crc = _crc8_ccitt_update(crc, buf[1]);
        buf[2] = crc;
        bq_i2c.write(i2c_address << 1, buf, 3);
    }
    else {
        bq_i2c.write(i2c_address << 1, buf, 2);
    }
}

//----------------------------------------------------------------------------

int bq769x0_read_byte(int address)
{
    uint8_t crc = 0;
    char buf[2];

    #if BMS_DEBUG
    //printf("Read register: 0x%x \n", address);
    #endif

    buf[0] = (char)address;
    bq_i2c.write(i2c_address << 1, buf, 1);;

    if (crc_enabled == true) {
        do {
            bq_i2c.read(i2c_address << 1, buf, 2);
            // CRC is calculated over the slave address (including R/W bit) and data.
            crc = _crc8_ccitt_update(crc, (i2c_address << 1) | 1);
            crc = _crc8_ccitt_update(crc, buf[0]);
        } while (crc != buf[1]);
        return buf[0];
    }
    else {
        bq_i2c.read(i2c_address << 1, buf, 1);
        return buf[0];
    }
}

int bq769x0_read_word(char reg)
{
    int val = 0;
    char buf[4];

    uint8_t crc;

    // write starting register
    bq_i2c.write(i2c_address << 1, &reg, 1);

    if (crc_enabled == true) {
        bq_i2c.read(i2c_address << 1, buf, 4);

        // CRC of first bytes includes slave address (including R/W bit) and data
        crc = _crc8_ccitt_update(0, (i2c_address << 1) | 1);
        crc = _crc8_ccitt_update(crc, buf[0]);
        if (crc != buf[1]) {
            return -1;
        }

        // CRC of subsequent bytes contain only data
        crc = _crc8_ccitt_update(0, buf[2]);
        if (crc != buf[3]) {
            return -1;
        }

        val = buf[0] << 8 | buf[2];
    }
    else {
        bq_i2c.read(i2c_address << 1, buf, 2);
        val = buf[0] << 8 | buf[1];
    }
    return val;
}

//----------------------------------------------------------------------------
// automatically find out address and CRC setting

static bool determine_address_and_crc(void)
{
    i2c_address = 0x08;
    crc_enabled = true;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x18;
    crc_enabled = true;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x08;
    crc_enabled = false;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    i2c_address = 0x18;
    crc_enabled = false;
    bq769x0_write_byte(CC_CFG, 0x19);
    if (bq769x0_read_byte(CC_CFG) == 0x19) {
        return true;
    }

    return false;
}

void bq769x0_init()
{
    alert_interrupt.rise(bq769x0_alert_isr);
    alert_interrupt_flag = true;   // init with true to check and clear errors at start-up

    if (determine_address_and_crc())
    {
        // initial settings for bq769x0
        bq769x0_write_byte(SYS_CTRL1, 0b00011000);  // switch external thermistor and ADC on
        bq769x0_write_byte(SYS_CTRL2, 0b01000000);  // switch CC_EN on

        // get ADC offset and gain
        adc_offset = (signed int) bq769x0_read_byte(ADCOFFSET);  // convert from 2's complement
        adc_gain = 365 + (((bq769x0_read_byte(ADCGAIN1) & 0b00001100) << 1) |
            ((bq769x0_read_byte(ADCGAIN2) & 0b11100000) >> 5)); // uV/LSB
    }
    else {
        // TODO: do something else... e.g. set error flag
#if BMS_DEBUG
        printf("BMS communication error\n");
#endif
    }
}

#endif // MBED

bool bq769x0_alert_flag()
{
    return alert_interrupt_flag;
}

void bq769x0_alert_flag_reset()
{
    alert_interrupt_flag = false;
}

time_t bq769x0_alert_timestamp()
{
    return alert_interrupt_timestamp;
}

#endif // defined BQ769x0