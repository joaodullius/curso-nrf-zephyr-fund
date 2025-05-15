#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(gnss_nmea, CONFIG_GNSS_NMEA_LOG_LEVEL);

#define UART_NODE DT_ALIAS(work_uart)
const struct device *uart = DEVICE_DT_GET(UART_NODE);
#define NMEA_BUFFER_SIZE 256
#define GNSS_FIX_TIMEOUT  60  // segundos

static volatile bool gnss_fix_ready = false;  // flag global

double encode_nmea_to_double(const char *nmea_coord, char direction) {
    if (nmea_coord == NULL) return 0.0;

    double raw_value = atof(nmea_coord);
    int degrees = (int)(raw_value / 100);
    double minutes = raw_value - (degrees * 100);
    double decimal_degrees = degrees + (minutes / 100.0);

    if (direction == 'S' || direction == 'W') {
        decimal_degrees = -decimal_degrees;
    }

    return decimal_degrees;
}

void process_nmea_sentence(char *nmea) {
    char *token;
    int field = 0;
    char *lat_str = NULL, *lat_dir = NULL;
    char *lon_str = NULL, *lon_dir = NULL;
    char *alt_str = NULL;
    int fix_status = 0, num_satellites = 0;
    float hdop = 0.0;

    if (strstr(nmea, "$GNGGA") != NULL) {
        LOG_WRN("%s", nmea);

        token = strtok(nmea, ",");
        while (token != NULL) {
            switch (field) {
                case 2: lat_str = token; break;
                case 3: lat_dir = token; break;
                case 4: lon_str = token; break;
                case 5: lon_dir = token; break;
                case 6: fix_status = atoi(token); break;
                case 7: num_satellites = atoi(token); break;
                case 8: hdop = atof(token); break;
                case 9: alt_str = token; break;
            }
            token = strtok(NULL, ",");
            field++;
        }

        if (fix_status > 0) {
            double latitude = encode_nmea_to_double(lat_str, lat_dir[0]);
            double longitude = encode_nmea_to_double(lon_str, lon_dir[0]);
            float altitude = atof(alt_str);

            const char *fix_type =
                (fix_status == 1) ? "GPS Fix" :
                (fix_status == 2) ? "DGPS Fix" :
                (fix_status == 4) ? "RTK Fix" : "Unknown";

            LOG_INF("GNSS Fix Acquired!");
            LOG_INF("----------------------------------");
            LOG_INF(" Fix Type      : %s", fix_type);
            LOG_INF(" Latitude      : %.7f deg", latitude);
            LOG_INF(" Longitude     : %.7f deg", longitude);
            LOG_INF(" Altitude      : %.2f m", (double)altitude);
            LOG_INF(" Satellites    : %d", num_satellites);
            LOG_INF(" HDOP          : %.2f", (double)hdop);
            LOG_INF("----------------------------------");

            gnss_fix_ready = true;
        } else {
            LOG_WRN("GNSS Fix Invalid! Retrying...");
        }
    }
}

static void uart_cb(const struct device *dev, void *user_data) {
    uart_irq_update(dev);
    if (uart_irq_is_pending(dev) && uart_irq_rx_ready(dev)) {
        uint8_t byte;
        static char rx_buf[NMEA_BUFFER_SIZE];
        static int rx_buf_pos = 0;

        while (uart_fifo_read(dev, &byte, 1)) {
            if (rx_buf_pos < sizeof(rx_buf) - 1) {
                rx_buf[rx_buf_pos++] = byte;
            }

            if (byte == '\n') {
                rx_buf[rx_buf_pos] = '\0';
                LOG_DBG("Received NMEA: %s", rx_buf);
                process_nmea_sentence(rx_buf);
                rx_buf_pos = 0;
            }
        }
    }
}

int acquire_gnss_fix(void) {
    gnss_fix_ready = false;

#ifdef CONFIG_GNSS_NMEA_LOW_POWER
    LOG_DBG("Enabling UART Peripheral...");
    pm_device_action_run(uart, PM_DEVICE_ACTION_RESUME);
#endif
    uart_irq_rx_enable(uart);


    LOG_INF("Waiting for GNSS fix...");
    for (int i = 0; i < GNSS_FIX_TIMEOUT; i++) {
        if (gnss_fix_ready) break;
        k_sleep(K_SECONDS(1));
    }

#ifdef CONFIG_GNSS_NMEA_LOW_POWER
    LOG_DBG("Disabling UART Peripheral...");
    uart_irq_rx_disable(uart);
    pm_device_action_run(uart, PM_DEVICE_ACTION_SUSPEND);
#endif

    if (!gnss_fix_ready) {
        LOG_ERR("GNSS Fix Timed Out!");
        return -1;
    }
    return 0;
}

int uart_init(void) {
    if (!device_is_ready(uart)) {
        LOG_ERR("UART device not ready");
        return -1;
    }
    uart_irq_callback_set(uart, uart_cb);
    return 0;
}

int main(void) {
    LOG_INF("GNSS Low-Power Mode Initialized. Waiting for Fix Requests...");
    if (uart_init() < 0) return -1;

    acquire_gnss_fix();  // Primeira tentativa

    while (1) {
        k_sleep(K_SECONDS(5));  // Temporário: 5s (pode ser aumentado depois)
        if (acquire_gnss_fix() < 0) {
            LOG_WRN("GNSS Fix Failed! Trying again in the next cycle.");
        }
    }

    return 0;
}
