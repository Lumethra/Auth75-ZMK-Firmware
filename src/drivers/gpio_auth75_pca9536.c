#define DT_DRV_COMPAT auth75_pca9536

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

LOG_MODULE_REGISTER(auth75_pca9536, CONFIG_GPIO_LOG_LEVEL);

#define PCA9536_INPUT 0x00
#define PCA9536_OUTPUT 0x01
#define PCA9536_POLARITY 0x02
#define PCA9536_CONFIG 0x03
#define PCA9536_I2C_RETRIES 3
#define PCA9536_LED_OUTPUTS 0x08

struct auth75_pca9536_config {
    struct gpio_driver_config common;
    struct i2c_dt_spec bus;
};

struct auth75_pca9536_data {
    struct gpio_driver_data common;
    struct k_sem lock;
    uint8_t output;
    uint8_t config;
};

static int auth75_pca9536_write(const struct auth75_pca9536_config *cfg,
                                uint8_t reg, uint8_t value) {
    uint8_t write_buf[] = {reg, value};
    int ret;

    for (int attempt = 0; attempt < PCA9536_I2C_RETRIES; attempt++) {
        ret = i2c_write_dt(&cfg->bus, write_buf, sizeof(write_buf));
        if (ret != -EAGAIN) {
            return ret;
        }
        (void)i2c_recover_bus(cfg->bus.bus);
        k_msleep(1);
    }

    return ret;
}

static int auth75_pca9536_read(const struct auth75_pca9536_config *cfg,
                               uint8_t reg, uint8_t *value) {
    int ret;

    for (int attempt = 0; attempt < PCA9536_I2C_RETRIES; attempt++) {
        ret = i2c_reg_read_byte_dt(&cfg->bus, reg, value);
        if (ret != -EAGAIN) {
            return ret;
        }
        (void)i2c_recover_bus(cfg->bus.bus);
        k_msleep(1);
    }

    return ret;
}

static int auth75_pca9536_configure(const struct device *dev, gpio_pin_t pin,
                                    gpio_flags_t flags) {
    const struct auth75_pca9536_config *cfg = dev->config;
    struct auth75_pca9536_data *data = dev->data;
    int ret;

    if (pin >= 4 || (flags & GPIO_SINGLE_ENDED) != 0 ||
        (flags & GPIO_PULL_UP) != 0 || (flags & GPIO_PULL_DOWN) != 0) {
        return -ENOTSUP;
    }

    if ((flags & GPIO_INPUT) != 0 && (flags & GPIO_OUTPUT) != 0) {
        return -ENOTSUP;
    }

    k_sem_take(&data->lock, K_FOREVER);

    if ((flags & GPIO_OUTPUT) != 0) {
        if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
            data->output |= BIT(pin);
        } else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
            data->output &= ~BIT(pin);
        }
        data->config &= ~BIT(pin);
        ret = auth75_pca9536_write(cfg, PCA9536_OUTPUT, data->output);
        if (ret == 0) {
            ret = auth75_pca9536_write(cfg, PCA9536_CONFIG, data->config);
        }
    } else if ((flags & GPIO_INPUT) != 0) {
        data->config |= BIT(pin);
        ret = auth75_pca9536_write(cfg, PCA9536_CONFIG, data->config);
    } else {
        ret = -ENOTSUP;
    }

    k_sem_give(&data->lock);
    return ret;
}

static int auth75_pca9536_set_masked(const struct device *dev,
                                     gpio_port_pins_t mask,
                                     gpio_port_value_t value) {
    const struct auth75_pca9536_config *cfg = dev->config;
    struct auth75_pca9536_data *data = dev->data;
    uint8_t output;
    int ret;

    k_sem_take(&data->lock, K_FOREVER);
    output = (data->output & ~mask) | (value & mask);
    ret = auth75_pca9536_write(cfg, PCA9536_OUTPUT, output);
    if (ret == 0) {
        data->output = output;
    }
    k_sem_give(&data->lock);
    return ret;
}

static int auth75_pca9536_set_bits(const struct device *dev,
                                   gpio_port_pins_t pins) {
    return auth75_pca9536_set_masked(dev, pins, pins);
}

static int auth75_pca9536_clear_bits(const struct device *dev,
                                     gpio_port_pins_t pins) {
    return auth75_pca9536_set_masked(dev, pins, 0);
}

static int auth75_pca9536_toggle_bits(const struct device *dev,
                                      gpio_port_pins_t pins) {
    const struct auth75_pca9536_config *cfg = dev->config;
    struct auth75_pca9536_data *data = dev->data;
    uint8_t output;
    int ret;

    k_sem_take(&data->lock, K_FOREVER);
    output = data->output ^ pins;
    ret = auth75_pca9536_write(cfg, PCA9536_OUTPUT, output);
    if (ret == 0) {
        data->output = output;
    }
    k_sem_give(&data->lock);
    return ret;
}

static int auth75_pca9536_get(const struct device *dev, uint32_t *value) {
    const struct auth75_pca9536_config *cfg = dev->config;
    struct auth75_pca9536_data *data = dev->data;
    uint8_t input;
    int ret;

    k_sem_take(&data->lock, K_FOREVER);
    ret = i2c_reg_read_byte_dt(&cfg->bus, PCA9536_INPUT, &input);
    if (ret == 0) {
        *value = input;
    }
    k_sem_give(&data->lock);
    return ret;
}

static int auth75_pca9536_init(const struct device *dev) {
    const struct auth75_pca9536_config *cfg = dev->config;
    struct auth75_pca9536_data *data = dev->data;
    uint8_t input;
    int ret;

    if (!device_is_ready(cfg->bus.bus)) {
        LOG_ERR("I2C bus %s is not ready", cfg->bus.bus->name);
        return -ENODEV;
    }

    k_sem_init(&data->lock, 1, 1);
    LOG_INF("initializing PCA9536 at I2C address 0x%02x on bus %s",
            cfg->bus.addr, cfg->bus.bus->name);
#if defined(CONFIG_I2C_DW_CLOCK_SPEED)
    LOG_INF("compiled I2C_DW clock speed: %d MHz",
            CONFIG_I2C_DW_CLOCK_SPEED);
#endif
    ret = auth75_pca9536_read(cfg, PCA9536_INPUT, &input);
    if (ret != 0) {
        LOG_ERR("PCA9536 ACK/register read failed at 0x%02x: %d",
                PCA9536_INPUT, ret);
        return ret;
    }
    LOG_INF("PCA9536 responded; input register: 0x%02x", input);

    ret = auth75_pca9536_write(cfg, PCA9536_OUTPUT, 0);
    if (ret != 0) {
        LOG_ERR("failed writing output register 0x%02x: %d",
                PCA9536_OUTPUT, ret);
    }
    if (ret == 0) {
        ret = auth75_pca9536_write(cfg, PCA9536_CONFIG,
                                   PCA9536_LED_OUTPUTS);
        if (ret != 0) {
            LOG_ERR("failed writing config register 0x%02x: %d",
                    PCA9536_CONFIG, ret);
        }
    }
    if (ret == 0) {
        LOG_INF("PCA9536 initialized");
    }
    return ret;
}

static DEVICE_API(gpio, auth75_pca9536_api) = {
    .pin_configure = auth75_pca9536_configure,
    .port_get_raw = auth75_pca9536_get,
    .port_set_masked_raw = auth75_pca9536_set_masked,
    .port_set_bits_raw = auth75_pca9536_set_bits,
    .port_clear_bits_raw = auth75_pca9536_clear_bits,
    .port_toggle_bits = auth75_pca9536_toggle_bits,
};

#define AUTH75_PCA9536_INIT(inst)                                             \
    static struct auth75_pca9536_data auth75_pca9536_data_##inst = {          \
        .output = 0,                                                           \
        .config = PCA9536_LED_OUTPUTS,                                          \
    };                                                                         \
    static const struct auth75_pca9536_config auth75_pca9536_config_##inst = { \
        .common = {                                                            \
            .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),           \
        },                                                                     \
        .bus = I2C_DT_SPEC_INST_GET(inst),                                     \
    };                                                                         \
    DEVICE_DT_INST_DEFINE(inst, auth75_pca9536_init, NULL,                     \
                          &auth75_pca9536_data_##inst,                        \
                          &auth75_pca9536_config_##inst, POST_KERNEL, 80,     \
                          &auth75_pca9536_api);

DT_INST_FOREACH_STATUS_OKAY(AUTH75_PCA9536_INIT)
