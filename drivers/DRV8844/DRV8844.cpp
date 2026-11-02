//
// Created by awing on 10/21/26.
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

#include <drivers/crir-motors.h>


LOG_MODULE_REGISTER(DRV8844, CONFIG_DRV8844_LOG_LEVEL);
#define DRV8844_CHANNEL_COUNT 2

struct drv8844_channel_config {
    struct gpio_dt_spec gpio_en;
    struct pwm_dt_spec pwm;
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

    auto [gpio_en, pwm, gpio_dir] = config->channels[channel];
    gpio_pin_set_dt(&gpio_en, 1);
    gpio_pin_set_dt(&gpio_dir, speed > 0 ? 1 : 0);

    const uint32_t pulse = static_cast<uint32_t>(magnitude * pwm.period);
    pwm_set_dt(&pwm, pwm.period, pulse);


    return 0;
}

static DEVICE_API(crir_motors_speed, crir_motors_speed_driver_api) = {
    .set_speed = &drv8844_set_speed,
};

static int drv8844_init(const struct device *dev) {
    const auto *config =  static_cast<const drv8844_config *>(dev->config);

    if (!gpio_is_ready_dt(&config->gpio_sleep)) {
        LOG_ERR("Sleep GPIO not ready");
        return -ENODEV;
    } if (!gpio_is_ready_dt(&config->gpio_fault)) {
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

    if (const auto ret = gpio_pin_configure_dt(&config->gpio_sleep, GPIO_OUTPUT_INACTIVE); ret < 0) {
        LOG_ERR("Failed to configure gpio sleep");
        return ret;
    } if (const auto ret = gpio_pin_configure_dt(&config->gpio_fault, GPIO_INPUT); ret < 0) {
        LOG_ERR("Failed to configure gpio fault");
        return ret;
    }
    return 0;
};

#define DRV8844_NODE DT_NODELABEL(drv8844_0)
#define DRV8844_CHANNEL0_NODE DT_NODELABEL(channel0)
#define DRV8844_CHANNEL1_NODE DT_NODELABEL(channel1)

static struct drv8844_data drv8844_data_0;

static const struct drv8844_config drv8844_config_0 = {
    .channel_count = DT_PROP(DRV8844_NODE, channel_count),
    .channels = {
        {
            .gpio_en = GPIO_DT_SPEC_GET(DRV8844_CHANNEL0_NODE, enable_gpios),
            .pwm = PWM_DT_SPEC_GET(DRV8844_CHANNEL0_NODE),
            .gpio_dir = GPIO_DT_SPEC_GET(DRV8844_CHANNEL0_NODE, dir_gpios),
        },
        {
            .gpio_en = GPIO_DT_SPEC_GET(DRV8844_CHANNEL1_NODE, enable_gpios),
            .pwm = PWM_DT_SPEC_GET(DRV8844_CHANNEL1_NODE),
            .gpio_dir = GPIO_DT_SPEC_GET(DRV8844_CHANNEL1_NODE, dir_gpios),
        },
    },
    .gpio_sleep = GPIO_DT_SPEC_GET(DRV8844_NODE, sleep_gpios),
    .gpio_fault = GPIO_DT_SPEC_GET(DRV8844_NODE, fault_gpios),
};

DEVICE_DT_DEFINE(DRV8844_NODE, drv8844_init, NULL, &drv8844_data_0,
                 &drv8844_config_0, POST_KERNEL, CONFIG_DRV8844_INIT_PRIORITY,
                 &crir_motors_speed_driver_api);
