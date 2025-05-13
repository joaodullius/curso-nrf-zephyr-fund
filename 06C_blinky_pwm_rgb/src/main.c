#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#define PWM_RED_NODE   DT_ALIAS(pwm_red)
#define PWM_GREEN_NODE DT_ALIAS(pwm_green)
#define PWM_BLUE_NODE  DT_ALIAS(pwm_blue)

static const struct pwm_dt_spec pwm_red   = PWM_DT_SPEC_GET(PWM_RED_NODE);
static const struct pwm_dt_spec pwm_green = PWM_DT_SPEC_GET(PWM_GREEN_NODE);
static const struct pwm_dt_spec pwm_blue  = PWM_DT_SPEC_GET(PWM_BLUE_NODE);

// Macro para facilitar a escrita do PWM RGB
#define SET_RGB(r, g, b)                                \
    do {                                                \
        pwm_set_dt(&pwm_red, pwm_red.period, r);        \
        pwm_set_dt(&pwm_green, pwm_green.period, g);    \
        pwm_set_dt(&pwm_blue, pwm_blue.period, b);      \
    } while (0)

#define FADE_STEP 5      // passo de variação do PWM (em %)
#define DELAY_MS 20      // delay entre cada passo (em ms)

void fade_rgb(uint8_t from_r, uint8_t from_g, uint8_t from_b,
              uint8_t to_r, uint8_t to_g, uint8_t to_b)
{
    for (int step = 0; step <= 100; step += FADE_STEP) {
        uint8_t r = from_r + (to_r - from_r) * step / 100;
        uint8_t g = from_g + (to_g - from_g) * step / 100;
        uint8_t b = from_b + (to_b - from_b) * step / 100;

        uint32_t pr = pwm_red.period * r / 100;
        uint32_t pg = pwm_green.period * g / 100;
        uint32_t pb = pwm_blue.period * b / 100;

        SET_RGB(pr, pg, pb);
        k_msleep(DELAY_MS);
    }
}

int main(void)
{
    if (!device_is_ready(pwm_red.dev) ||
        !device_is_ready(pwm_green.dev) ||
        !device_is_ready(pwm_blue.dev)) {
        return -1;
    }

    printf("PWM RGB LED devices are ready\n");

    while (1) {
        fade_rgb(0, 0, 0, 100, 0, 0);    // Preto → Vermelho
        fade_rgb(100, 0, 0, 0, 100, 0);  // Vermelho → Verde
        fade_rgb(0, 100, 0, 0, 0, 100);  // Verde → Azul
        fade_rgb(0, 0, 100, 100, 100, 0); // Azul → Amarelo
        fade_rgb(100, 100, 0, 0, 100, 100); // Amarelo → Ciano
        fade_rgb(0, 100, 100, 100, 0, 100); // Ciano → Magenta
        fade_rgb(100, 0, 100, 100, 100, 100); // Magenta → Branco
        fade_rgb(100, 100, 100, 0, 0, 0); // Branco → Preto
    }

    return 0;
}
