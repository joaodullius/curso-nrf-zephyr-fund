#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(gnss_nmea, LOG_LEVEL_INF);

#define UART_NODE DT_NODELABEL(uart1)
const struct device *uart = DEVICE_DT_GET(UART_NODE);
#define NMEA_BUFFER_SIZE 256  // Buffer size for storing NMEA sentences

#define GNSS_FIX_TIMEOUT  60

//Start Aquire GNSS Fix Semaphore
K_SEM_DEFINE(gnss_get_fix, 0, 1);
K_SEM_DEFINE(gnss_fix_done, 0, 1);
K_SEM_DEFINE(gnss_fix_timeout, 0, 1);

/* Convert NMEA DDMM.MMMMM to Decimal Degrees (double) */
double encode_nmea_to_double(const char *nmea_coord, char direction) {
    if (nmea_coord == NULL) {
        return 0.0;
    }

    double raw_value = atof(nmea_coord);
    int degrees;
    double minutes;
    double decimal_degrees;

    // Determine if it's latitude (DDMM.MMMMM) or longitude (DDDMM.MMMMM)
    if (strlen(nmea_coord) > 9) {  // Longitude (DDDMM.MMMMM)
        degrees = (int)(raw_value / 100);
        minutes = raw_value - (degrees * 100);
    } else {  // Latitude (DDMM.MMMMM)
        degrees = (int)(raw_value / 100);
        minutes = raw_value - (degrees * 100);
    }

    // Convert to decimal degrees (divide minutes by 100 as per João's preference)
    decimal_degrees = degrees + (minutes / 100.0);

    // Apply negative sign if direction is South (S) or West (W)
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
        LOG_WRN("%s", nmea);  // Print the NMEA sentence for debugging

        token = strtok(nmea, ",");
        while (token != NULL) {
            switch (field) {
                case 2: lat_str = token; break;     // Latitude
                case 3: lat_dir = token; break;     // N/S
                case 4: lon_str = token; break;     // Longitude
                case 5: lon_dir = token; break;     // E/W
                case 6: fix_status = atoi(token); break; // Fix Quality
                case 7: num_satellites = atoi(token); break; // Number of Satellites
                case 8: hdop = atof(token); break;  // HDOP
                case 9: alt_str = token; break;     // Altitude
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

            k_sem_give(&gnss_fix_done);  // Signal valid fix
        } else {
            LOG_WRN("GNSS Fix Invalid! Retrying...");
        }
    }
}



static void uart_cb(const struct device *dev, void *user_data) {
    /* Make sure we re-enable interrupts */
    uart_irq_update(dev);

    /* Process all pending interrupts */
    if (uart_irq_is_pending(dev)) {
        /* Check if RX is ready */
        if (uart_irq_rx_ready(dev)) {
            uint8_t byte;
            static char rx_buf[NMEA_BUFFER_SIZE];
            static int rx_buf_pos = 0;

            while (uart_irq_rx_ready(dev)) {
                uart_fifo_read(dev, &byte, 1);  // Read one byte

                // Store data in buffer
                if (rx_buf_pos < sizeof(rx_buf) - 1) {
                    rx_buf[rx_buf_pos++] = byte;
                }

                // If end of message (`\n`), process the full NMEA sentence
                if (byte == '\n' && rx_buf_pos > 0) {
                    rx_buf[rx_buf_pos] = '\0';  // Null-terminate the string
                    LOG_DBG("Received NMEA: %s", rx_buf);
                    process_nmea_sentence(rx_buf);  // Process NMEA data
                    rx_buf_pos = 0;  // Reset buffer
                }
            }
        }
    }
}


int acquire_gnss_fix(void) {
    LOG_DBG("Enabling UART Peripheral...");

    k_sem_reset(&gnss_fix_done);

    pm_device_action_run(uart, PM_DEVICE_ACTION_RESUME);
    uart_irq_rx_enable(uart);  // Enable UART RX

    // Wait for GNSS fix with a timeout
    int ret = k_sem_take(&gnss_fix_done, K_SECONDS(GNSS_FIX_TIMEOUT));

    LOG_DBG("Disabling UART Peripheral...");
    uart_irq_rx_disable(uart);  // Disable UART RX
    pm_device_action_run(uart, PM_DEVICE_ACTION_SUSPEND);
    
    if (ret < 0) {
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
    
    // Set up interrupt-driven UART
    uart_irq_callback_set(uart, uart_cb);
    return 0;
}

/* Main function - stays in low-power mode, wakes up when needed */
int main(void) {
    LOG_INF("GNSS Low-Power Mode Initialized. Waiting for Fix Requests...");
    int err = uart_init();
    if (err < 0) {
        LOG_ERR("UART Initialization Failed!");
        return -1;
    }
    acquire_gnss_fix();  // Acquire GNSS fix at startup
    while (1) {
        k_sleep(K_SECONDS(5));  // Sleep for 10 minutes

        // Wake up and request a GNSS fix
        if (acquire_gnss_fix() < 0) {
            LOG_WRN("GNSS Fix Failed! Trying again in the next cycle.");
        }
    }

    return 0;
}
