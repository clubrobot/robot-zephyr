//
// Created by awing on 11/16/26.
// SPDX-License-Identifier: AGPL-3.0
//

#ifndef APP_MOTORS_H
#define APP_MOTORS_H

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

enum query {
    MOTOR_SPEED,
    MOTOR_ENABLE,
};

__subsystem struct motors_driver_api {
    int (*set)(const struct device *dev, enum query q, uint8_t channel, struct sensor_value* val);
    int (*get)(const struct device *dev, enum query q, uint8_t channel, struct sensor_value* val);
};

__syscall int motors_set(const struct device *dev, enum query q, uint8_t channel, struct sensor_value* val);
__syscall int motors_get(const struct device *dev, enum query q, uint8_t channel, struct sensor_value* val);

static inline int z_impl_motors_set(const struct device *dev, enum query q, uint8_t channel, struct sensor_value* val) {
    __ASSERT_NO_MSG(DEVICE_API_IS(motors, dev));
    return DEVICE_API_GET(motors, dev)->set(dev, q, channel, val);
}
static inline int z_impl_motors_get(const struct device *dev, enum query q, uint8_t channel, struct sensor_value* val) {
    __ASSERT_NO_MSG(DEVICE_API_IS(motors, dev));
    return DEVICE_API_GET(motors, dev)->get(dev, q, channel, val);
}

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/motors.h>
#endif //APP_MOTORS_H
