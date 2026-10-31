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

static const struct device *const pca9536 =
    DEVICE_DT_GET(DT_NODELABEL(pca9536));

static int auth75_led_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    gpio_port_value_t output;
    int ret;

    ARG_UNUSED(event);

    if (binding->param1 < OFF || binding->param1 > RED) {
        LOG_ERR("Invalid Auth75 LED command: %d", binding->param1);
        return -EINVAL;
    }

    LOG_INF("Auth75 LED command: %d", binding->param1);

    if (!device_is_ready(pca9536)) {
        LOG_ERR("Auth75 LED GPIO controller is not ready: %s", pca9536->name);
        return -ENODEV;
    }

    output = 0;
    if (binding->param1 == WHITE) {
        output = BIT(0);
    } else if (binding->param1 == GREEN) {
        output = BIT(1);
    } else if (binding->param1 == RED) {
        output = BIT(2);
    }

    ret = gpio_port_set_masked_raw(pca9536, BIT_MASK(3), output);
    LOG_INF("Auth75 LED output mask 0x%02x value 0x%02x (ret %d)",
            BIT_MASK(3), output, ret);
    return ret;
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
