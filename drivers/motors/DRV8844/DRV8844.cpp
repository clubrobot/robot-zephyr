//
// Created by awing on 11/16/26.
// SPDX-License-Identifier: AGPL-3.0
//


#define DT_DRV_COMPAT drv8844

#include <zephyr/device.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <math.h>

#include <drivers/motors.h>

#include "zephyr/drivers/sensor.h"

LOG_MODULE_REGISTER(DRV8844, CONFIG_DRV8844_LOG_LEVEL);
#define DRV8844_CHANNEL_COUNT 2

struct drv8844_channel_config {
    struct gpio_dt_spec en_gpio;
    struct pwm_dt_spec pwm;
    struct gpio_dt_spec dir_gpio;
};

struct drv8844_data {

};

struct drv8844_config {
    uint8_t channel_count;
    struct gpio_dt_spec reset_gpio;
    struct gpio_dt_spec fault_gpio;

    struct drv8844_channel_config channels[DRV8844_CHANNEL_COUNT];
};

static inline int drv8844_set_speed(const struct device *dev, uint8_t channel, float speed) {
    const auto *config =  static_cast<const drv8844_config *>(dev->config);

    const auto magnitude = fminf(fabsf(speed), CONFIG_DRV8844_MAX_PWM / 100.0f);
    if (magnitude > 1) {
        LOG_ERR("Invalid speed");
        return -EINVAL;
    }

    auto [gpio_en, pwm, gpio_dir] = config->channels[channel];
    gpio_pin_set_dt(&gpio_en, 1);
    gpio_pin_set_dt(&gpio_dir, speed > 0 ? 1 : 0);

    const uint32_t pulse = static_cast<uint32_t>(magnitude) * pwm.period;
    pwm_set_dt(&pwm, pwm.period, pulse);

    return 0;
}


static int drv8844_set(const struct device* dev, enum query q, uint8_t channel, struct sensor_value* val) {
    const auto *config =  static_cast<const drv8844_config *>(dev->config);
    if (channel >= config->channel_count) {
        LOG_ERR("Invalid channel number");
        return -ENODEV;
    }
    switch (q) {
        case MOTOR_SPEED:
            return drv8844_set_speed(dev, channel, sensor_value_to_float(val));
        case MOTOR_ENABLE: {
            auto [gpio_en, pwm, gpio_dir] = config->channels[channel];
            gpio_pin_set_dt(&gpio_en, val->val1 ? 1 : 0);
            return 0;
        }
        default:
            LOG_ERR("Invalid query");
            return -EINVAL;
    }
}

static int drv8844_get(const struct device* dev, enum query q, uint8_t channel, struct sensor_value* val) {
    return -EINVAL;
}

static int drv8844_init(const struct device *dev) {
    const auto *config =  static_cast<const drv8844_config *>(dev->config);

    if (!gpio_is_ready_dt(&config->reset_gpio)) {
        LOG_ERR("Sleep GPIO not ready");
        return -ENODEV;
    } if (!gpio_is_ready_dt(&config->fault_gpio)) {
        LOG_ERR("Fault GPIO not ready");
        return -ENODEV;
    }

    for (int i = 0; i < config->channel_count; i++) {
        const auto [gpio_en, pwm, gpio_dir] = config->channels[i];
        if (!gpio_is_ready_dt(&gpio_en)) {
            LOG_ERR("Enable GPIO not ready");
            return -ENODEV;
        } if (!pwm_is_ready_dt(&pwm)) {
            LOG_ERR("PWM GPIO not ready");
            return -ENODEV;
        } if (!gpio_is_ready_dt(&gpio_dir)) {
            LOG_ERR("Direction GPIO not ready");
            return -ENODEV;
        }
    }

    for (int i = 0; i < config->channel_count; i++) {
        const auto [gpio_en, pwm, gpio_dir] = config->channels[i];
        if (const auto ret = gpio_pin_configure_dt(&gpio_en, GPIO_OUTPUT_INACTIVE); ret < 0) {
            LOG_ERR("Failed to configure gpio en");
            return ret;
        } if (const auto ret = gpio_pin_configure_dt(&gpio_dir, GPIO_OUTPUT_INACTIVE); ret < 0) {
            LOG_ERR("Failed to configure gpio dir");
            return ret;
        }

        if (const auto ret = pwm_set_dt(&pwm, pwm.period, 0); ret < 0) {
            LOG_ERR("Failed to configure pwm");
            return ret;
        }
    }

    if (const auto ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE); ret < 0) {
        LOG_ERR("Failed to configure gpio sleep");
        return ret;
    } if (const auto ret = gpio_pin_configure_dt(&config->fault_gpio, GPIO_INPUT); ret < 0) {
        LOG_ERR("Failed to configure gpio fault");
        return ret;
    }
    return 0;
};

static motors_driver_api drv_api = {
    .set = &drv8844_set,
    .get = &drv8844_get
};

#define DRV8844_INST(n)                                                                 \
    static struct drv8844_data drv8844_data_##n;                                        \
    static const struct drv8844_config drv8844_cfg##n = {                               \
        .channel_count = DT_INST_PROP(n, channel_count),                                \
        .reset_gpio = GPIO_DT_SPEC_INST_GET(n, reset_gpios),                            \
        .fault_gpio = GPIO_DT_SPEC_INST_GET(n, fault_gpios),                            \
        .channels = {                                                                   \
            {                                                                           \
                .en_gpio =  GPIO_DT_SPEC_GET(DT_INST_CHILD(n, channel_0), enable_gpios),\
                .pwm = PWM_DT_SPEC_GET(DT_PROP(DT_INST_CHILD(n, channel_0), pwm)),      \
                .dir_gpio = GPIO_DT_SPEC_GET(DT_INST_CHILD(n, channel_0), dir_gpios),   \
            },                                                                          \
            {                                                                           \
                .en_gpio =  GPIO_DT_SPEC_GET(DT_INST_CHILD(n, channel_1), enable_gpios),\
                .pwm = PWM_DT_SPEC_GET(DT_PROP(DT_INST_CHILD(n, channel_1), pwm)),      \
                .dir_gpio = GPIO_DT_SPEC_GET(DT_INST_CHILD(n, channel_1), dir_gpios),   \
            }                                                                           \
        }                                                                               \
    };                                                                                  \
    DEVICE_DT_INST_DEFINE(n, drv8844_init, NULL, &drv8844_data_##n, &drv8844_cfg##n,    \
                POST_KERNEL, CONFIG_DRV8844_INIT_PRIORITY, &drv_api);

DT_INST_FOREACH_STATUS_OKAY(DRV8844_INST)