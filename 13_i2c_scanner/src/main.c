/*
 * Copyright (c) 2021 Joao Dullius
 * Copyright (c) 2020 Sigurd Nevstad
 * Based on:
 * https://github.com/sigurdnev/ncs-playground/tree/master/samples/i2c_scanner
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <string.h>
#include <stdlib.h>

#define I2C_NODE DT_NODELABEL(i2c0)

void main(void)
{

    printk("Starting i2c scanner\n");

	const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
    	if (i2c_dev == NULL || !device_is_ready(i2c_dev)) {
		printk("I2C: Device driver not found.\n");
		return;
	}
	
	i2c_dev = DEVICE_DT_GET(I2C_NODE);

    uint8_t buffer = 0;
	uint8_t ret = 0;
	
	i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD));
	
	for (uint8_t i = 0; i <= 0x7F; i++) {	

        ret = i2c_reg_read_byte(i2c_dev, i, 0, &buffer);

        if (ret == 0) {
			printk("0x%2x FOUND\n", i);
		}
		else {
			printk("error %d \n", ret);
		}
		
	}
	printk("Scanning done\n");
	
}