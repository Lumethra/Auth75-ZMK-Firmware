#define DT_DRV_COMPAT zmk_behavior_auth75_led

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <dt-bindings/zmk/auth75_led.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static const struct gpio_dt_spec white_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(white_led), gpios);
static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(green_led), gpios);
static const struct gpio_dt_spec red_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(red_led), gpios);

static int set_led(const struct gpio_dt_spec *led, bool on) {
    int ret;

    if (!device_is_ready(led->port)) {
        LOG_ERR("Auth75 LED GPIO controller is not ready: %s", led->port->name);
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Auth75 LED configure failed on pin %u: %d", led->pin, ret);
        return ret;
    }

    ret = gpio_pin_set_dt(led, on);
    LOG_INF("Auth75 LED pin %u set to %d (ret %d)", led->pin, on, ret);
    return ret;
}

static int auth75_led_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    int ret;

    ARG_UNUSED(event);

    if (binding->param1 < AUTH75_LED_OFF || binding->param1 > AUTH75_LED_RED) {
        LOG_ERR("Invalid Auth75 LED command: %d", binding->param1);
        return -EINVAL;
    }

    LOG_INF("Auth75 LED command: %d", binding->param1);

    ret = set_led(&white_led, binding->param1 == AUTH75_LED_WHITE);
    if (ret < 0) {
        return ret;
    }

    ret = set_led(&green_led, binding->param1 == AUTH75_LED_GREEN);
    if (ret < 0) {
        return ret;
    }

    return set_led(&red_led, binding->param1 == AUTH75_LED_RED);
}

static int auth75_led_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api auth75_led_driver_api = {
    .binding_pressed = auth75_led_binding_pressed,
    .binding_released = auth75_led_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &auth75_led_driver_api);

#endif
