#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#define PWM_LED_NODE DT_ALIAS(pwm_led0)

static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(PWM_LED_NODE);

int main(void)
{
    if (!device_is_ready(pwm_led.dev)) {
        return -1;
    }

	printf("PWM LED device is ready\n");

    while (1) {
        // 100% brilho
        pwm_set_dt(&pwm_led, pwm_led.period, pwm_led.period);
        k_msleep(1000);

		// 75% brilho
		pwm_set_dt(&pwm_led, pwm_led.period, pwm_led.period * 3 / 4);
		k_msleep(1000);

        // 50% brilho
        pwm_set_dt(&pwm_led, pwm_led.period, pwm_led.period / 2);
        k_msleep(1000);

		// 25% brilho
		pwm_set_dt(&pwm_led, pwm_led.period, pwm_led.period / 4);
		k_msleep(1000);

        // 0% brilho
        pwm_set_dt(&pwm_led, pwm_led.period, 0);
        k_msleep(1000);
    }
	return 0;
}
