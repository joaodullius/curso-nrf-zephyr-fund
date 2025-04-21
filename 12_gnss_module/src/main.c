/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/logging/log.h>

#define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))

LOG_MODULE_REGISTER(gnss_sample, CONFIG_GNSS_LOG_LEVEL);

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		printf("Got a fix!\n");

     /* If values are in 1e9 format, scale them */
	 double latitude = data->nav_data.latitude / 1e9;
	 double longitude = data->nav_data.longitude / 1e9;
	 double altitude = data->nav_data.altitude / 1e3;  // Check if this needs scaling

	 printf("Latitude: %.7f°\n", latitude);
	 printf("Longitude: %.7f°\n", longitude);
	 printf("Altitude: %.2f m\n", altitude);
	}
}
GNSS_DATA_CALLBACK_DEFINE(GNSS_MODEM, gnss_data_cb);

#if CONFIG_GNSS_SATELLITES
static void gnss_satellites_cb(const struct device *dev, const struct gnss_satellite *satellites,
			       uint16_t size)
{
	unsigned int tracked_count = 0;

	for (unsigned int i = 0; i != size; ++i) {
		tracked_count += satellites[i].is_tracked;
	}
	printf("%u satellite%s reported (of which %u tracked)!\n",
		size, size > 1 ? "s" : "", tracked_count);
}
#endif
GNSS_SATELLITES_CALLBACK_DEFINE(GNSS_MODEM, gnss_satellites_cb);

int main(void)
{
	gnss_set_fix_rate(GNSS_MODEM, 1000);
	return 0;
}
