/* LibreSolar Battery Management System firmware
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

#ifdef BMS_ISL94202

#include "isl94202_hw.h"
#include "isl94202_registers.h"

#ifdef ZEPHYR

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <string.h>

#define I2C_DEV "I2C_2"

#define I2C_PULLUP_PORT DT_ALIAS_I2C_PULLUP_GPIOS_CONTROLLER
#define I2C_PULLUP_PIN  DT_ALIAS_I2C_PULLUP_GPIOS_PIN

// static (private) variables
//----------------------------------------------------------------------------

static struct device *i2c_dev;

//----------------------------------------------------------------------------

int isl94202_write_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
	uint8_t buf[5];
	if ((reg_addr > 0x58 && reg_addr < 0x7F) || reg_addr + num_bytes > 0xAB || num_bytes > 4)
	    return -1;

	buf[0] = reg_addr;		// first byte contains register address
	memcpy(buf + 1, data, num_bytes);

    return i2c_write(i2c_dev, buf, num_bytes + 1, ISL94202_I2C_ADDRESS);
}

int isl94202_read_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
    return i2c_write_read(i2c_dev, ISL94202_I2C_ADDRESS, &reg_addr, 1, data, num_bytes);
}

void isl94202_init()
{
    // activate pull-up at I2C SDA and SCL
	struct device *i2c_pullup;
	i2c_pullup = device_get_binding(I2C_PULLUP_PORT);
	gpio_pin_configure(i2c_pullup, I2C_PULLUP_PIN, GPIO_DIR_OUT);
	gpio_pin_write(i2c_pullup, I2C_PULLUP_PIN, 1);

    i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		printk("I2C: Device driver not found.\n");
		return;
	}
}

#elif defined(__MBED__)

int isl94202_write_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
	/* TODO */
	return 1;
}

int isl94202_read_bytes(uint8_t reg_addr, uint8_t *data, uint32_t num_bytes)
{
	/* TODO */
	return 1;
}

void isl94202_init() {;}

#endif // ZEPHYR

int isl94202_write_word(uint8_t reg_addr, uint16_t word)
{
	uint8_t buf[2];
	buf[0] = word;
	buf[1] = word >> 8;
	return isl94202_write_bytes(reg_addr, buf, 2);
}

float isl94202_apply_current_limit(uint8_t reg_addr,
    const uint16_t *voltage_thresholds_mV, int num_thresholds,
	float current_limit, float shunt_res_mOhm,
    uint8_t delay_unit, uint16_t delay_value)
{
    uint16_t reg = 0;
    float actual_current_limit = 0;

	if (delay_value > 1023 || delay_unit > ISL94202_DELAY_MIN) {
		return 0;
	}

	// delay value: bits 0-9, unit: bits A-B
	reg = (delay_unit << 0xA) + delay_value;

    for (int i = num_thresholds - 1; i > 0; i--) {
        if (current_limit * shunt_res_mOhm / 1000 >= voltage_thresholds_mV[i]) {
            reg = i << 0xC;	// threshold: bits C-E
            actual_current_limit = voltage_thresholds_mV[i] * 1000 / shunt_res_mOhm;
            break;
        }
    }
    isl94202_write_word(reg_addr, reg);

	return actual_current_limit;
}

#endif // BMS_ISL94202