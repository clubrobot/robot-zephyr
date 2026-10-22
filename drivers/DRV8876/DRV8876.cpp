//
// Created by awing on 10/21/26.
// SPDX-License-Identifier: AGPL-3.0
//

#define DT_DRV_COMPAT drv8876

#include <zephyr/device.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/crir-motors.h>

LOG_MODULE_REGISTER(DRV8876, CONFIG_DRV8876_LOG_LEVEL);

struct drv8876_data {

};

struct drv8876_config {
    struct gpio_dt_spec gpio_pwm;
    struct gpio_dt_spec gpio_dir;
    struct gpio_dt_spec gpio_sleep;
    struct gpio_dt_spec gpio_fault;
};

int drv8876_set_speed(const struct device *dev, uint8_t channel, float speed) {
    return 0;
}

static DEVICE_API(crir_motors_speed, crir_motors_speed_driver_api) = {
    .set_speed = &drv8876_set_speed,
};