//
// Created by awing on 10/21/26.
// SPDX-License-Identifier: AGPL-3.0
//

#define DT_DRV_COMPAT drv8844

#include <zephyr/device.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>

#include <drivers/crir-motors.h>


LOG_MODULE_REGISTER(DRV8844, CONFIG_DRV8844_LOG_LEVEL);
#define DRV8844_CHANNEL_COUNT 2

struct drv8844_channel_config {
    struct gpio_dt_spec gpio_en;
    struct pwm_dt_spec gpio_pwm;
    struct gpio_dt_spec gpio_dir;
};

struct drv8844_data {

};

struct drv8844_config {
    uint8_t channel_count;
    struct drv8844_channel_config channels[DRV8844_CHANNEL_COUNT];

    struct gpio_dt_spec gpio_sleep;
    struct gpio_dt_spec gpio_fault;
};

static int drv8844_set_speed(const struct device *dev, uint8_t channel, float speed) {
    const auto *config =  static_cast<const drv8844_config *>(dev->config);

    auto magnitude = fabsf(speed);
    if (magnitude > 1) {
        LOG_ERR("Invalid speed");
        return -EINVAL;
    }
    if (channel >= config->channel_count) {
        LOG_ERR("Invalid channel number");
        return -ENODEV;
    }

    auto [gpio_en, gpio_pwm, gpio_dir] = config->channels[channel];
    gpio_pin_set_dt(&gpio_en, 1);
    gpio_pin_set_dt(&gpio_dir, speed > 0 ? 1 : 0);
    pwm_set_dt(&gpio_pwm, CONFIG_DRV8844_PWM_PERIOD, speed * CONFIG_DRV8844_PWM_PERIOD);


    return 0;
}

static DEVICE_API(crir_motors_speed, crir_motors_speed_driver_api) = {
    .set_speed = &drv8844_set_speed,
};

static int drv8844_init(const struct device *dev) {
    const auto *config =  static_cast<const drv8844_config *>(dev->config);
    auto *data = static_cast<drv8844_data *>(dev->data);

    if (!gpio_is_ready_dt(&config->gpio_sleep)) {
        LOG_ERR("Sleep GPIO not ready");
        return -ENODEV;
    } if (!gpio_is_ready_dt(&config->gpio_fault)) {
        LOG_ERR("Fault GPIO not ready");
        return -ENODEV;
    }

    for (int i = 0; i < config->channel_count; i++) {
        const auto [gpio_en, gpio_pwm, gpio_dir] = config->channels[i];
        if (!gpio_is_ready_dt(&gpio_en)) {
            LOG_ERR("Enable GPIO not ready");
            return -ENODEV;
        } if (!pwm_is_ready_dt(&gpio_pwm)) {
            LOG_ERR("PWM GPIO not ready");
            return -ENODEV;
        } if (!gpio_is_ready_dt(&gpio_dir)) {
            LOG_ERR("Direction GPIO not ready");
            return -ENODEV;
        }
    }

    for (int i = 0; i < config->channel_count; i++) {
        const auto [gpio_en, gpio_pwm, gpio_dir] = config->channels[i];
        if (const auto ret = gpio_pin_configure_dt(&gpio_en, GPIO_OUTPUT_INACTIVE); ret < 0) {
            LOG_ERR("Failed to configure gpio en");
            return ret;
        } if (const auto ret = gpio_pin_configure_dt(&gpio_dir, GPIO_OUTPUT_INACTIVE); ret < 0) {
            LOG_ERR("Failed to configure gpio dir");
            return ret;
        }
    }

    if (const auto ret = gpio_pin_configure_dt(&config->gpio_sleep, GPIO_OUTPUT_INACTIVE); ret < 0) {
        LOG_ERR("Failed to configure gpio sleep");
        return ret;
    } if (const auto ret = gpio_pin_configure_dt(&config->gpio_fault, GPIO_INPUT); ret < 0) {
        LOG_ERR("Failed to configure gpio fault");
        return ret;
    }
    return 0;
};

#define DRV8844_DEFINE(inst)                                    \
    static struct drv8844_data data##inst                       \
    static const struct drv8844_config config##inst = {         \
        .gpio_pwm = GPIO_DT_SPEC_GET(inst, gpio_pwm),           \
        .gpio_dir = GPIO_DT_SPEC_GET(inst, gpio_dir),           \
        .gpio_sleep = GPIO_DT_SPEC_GET(inst, gpio_sleep),       \
        .gpio_fault = GPIO_DT_SPEC_GET(inst, gpio_fault),       \
    }                                                           \
    DEVICE_DT_INST_DEFINE(inst, drv8844_init, NULL, &data##inst,\
    &config##inst, POST_KERNEL, CONFIG_DRV8844_INIT_PRIORITY,   \
    &crir_motors_speed_driver_api)

DT_INST_FOREACH_STATUS_OKAY(DRV8844_DEFINE);