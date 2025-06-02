#include <nrfx_timer.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(blinky_nrfx_timer, LOG_LEVEL_DBG);

#define LED0_NODE DT_ALIAS(led0)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#ifdef CONFIG_BOARD_NRF54L15DK
static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(22);
#else
static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(2);
#endif

void timer_event_handler(nrf_timer_event_t event_type, void *p_context)
{
    
    if (event_type == NRF_TIMER_EVENT_COMPARE0) {
        LOG_DBG("Timer event: %d", event_type);
        gpio_pin_toggle_dt(&led);
    }
}

int main(void)
{

    LOG_INF("Blinky with NRFX Timer started");
    int ret;

    if (!device_is_ready(led.port)) {
        LOG_ERR("LED GPIO port %s is not ready", led.port->name);
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED GPIO pin");
        return -1;
    }

   nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(16000000);
   timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32; 

    // Check initialization result
    nrfx_err_t status = nrfx_timer_init(&timer_inst, &timer_config, timer_event_handler);
    if (status != NRFX_SUCCESS) {
        LOG_ERR("Failed to initialize timer: %d", status);
        return -1;
    }

    LOG_DBG("Timer initialized successfully");
    nrfx_timer_clear(&timer_inst);
    LOG_DBG("Timer cleared");

    #define SAADC_SAMPLE_INTERVAL_MS 1000

    uint32_t timer_ticks = nrfx_timer_ms_to_ticks(&timer_inst, SAADC_SAMPLE_INTERVAL_MS);
    LOG_DBG("Timer ticks for %d ms: %d", SAADC_SAMPLE_INTERVAL_MS, timer_ticks);
    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, timer_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);


    #ifdef CONFIG_BOARD_NRF54L15DK
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(22)), IRQ_PRIO_LOWEST,
                   NRFX_TIMER_INST_HANDLER_GET(22), 0);
    #else
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(2)), IRQ_PRIO_LOWEST,
                   NRFX_TIMER_INST_HANDLER_GET(2), 0);
    #endif

    nrfx_timer_enable(&timer_inst);
    LOG_INF("Timer started, toggling LED every second");

}




