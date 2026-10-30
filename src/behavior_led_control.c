#define DT_DRV_COMPAT zmk_behavior_led_control

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define LED_COUNT 3

struct behavior_led_control_config {
    struct gpio_dt_spec leds[LED_COUNT];
    int target;
    int state;
};

static int led_control_init(const struct device *dev) {
    const struct behavior_led_control_config *config = dev->config;

    for (int i = 0; i < LED_COUNT; i++) {
        int ret = gpio_pin_configure_dt(&config->leds[i], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED %d (%d)", i, ret);
            return ret;
        }
    }

    return 0;
}

static int led_control_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_led_control_config *config = dev->config;

    ARG_UNUSED(event);

    for (int i = 0; i < LED_COUNT; i++) {
        int ret = gpio_pin_set_dt(&config->leds[i],
                                  config->state && i == config->target);
        if (ret < 0) {
            LOG_ERR("Failed to set LED %d (%d)", i, ret);
            return ret;
        }
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_led_control_driver_api = {
    .binding_pressed = led_control_binding_pressed,
};

#define LED_CONTROL_INST(n)                                                                    \
    static const struct behavior_led_control_config behavior_led_control_config_##n = {        \
        .leds = {                                                                               \
            GPIO_DT_SPEC_INST_GET_BY_IDX(n, gpios, 0),                                         \
            GPIO_DT_SPEC_INST_GET_BY_IDX(n, gpios, 1),                                         \
            GPIO_DT_SPEC_INST_GET_BY_IDX(n, gpios, 2),                                         \
        },                                                                                     \
        .target = DT_INST_PROP(n, target),                                                      \
        .state = DT_INST_PROP(n, state),                                                        \
    };                                                                                          \
    BEHAVIOR_DT_INST_DEFINE(n, led_control_init, NULL, NULL, &behavior_led_control_config_##n, \
                            POST_KERNEL,                                                                        \
                            CONFIG_AUTH75_LED_INIT_PRIORITY,                                  \
                            &behavior_led_control_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LED_CONTROL_INST)
