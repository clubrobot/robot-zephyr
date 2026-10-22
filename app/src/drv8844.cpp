#include "drv8844.hpp"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

namespace {
constexpr uint32_t kPwmPeriod = PWM_USEC(50);

bool gpioReady(const gpio_dt_spec& gpio)
{
    return gpio.port != nullptr && device_is_ready(gpio.port);
}
} // namespace

Drv8844::Drv8844(const Drv8844Motor& left, const Drv8844Motor& right,
                 const gpio_dt_spec& reset, const gpio_dt_spec& fault)
    : left_(left), right_(right), reset_(reset), fault_(fault)
{
}

int Drv8844::init()
{
    if (!device_is_ready(left_.enable.dev) ||
        !device_is_ready(right_.enable.dev) ||
        !gpioReady(left_.input_a) || !gpioReady(left_.input_b) ||
        !gpioReady(right_.input_a) || !gpioReady(right_.input_b) ||
        !gpioReady(reset_) || !gpioReady(fault_)) {
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&reset_, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return ret;
    }

    const Drv8844Motor* motors[] = {&left_, &right_};
    for (const Drv8844Motor* motor : motors) {
        ret = gpio_pin_configure_dt(&motor->input_a, GPIO_OUTPUT_INACTIVE);
        if (ret != 0) {
            return ret;
        }
        ret = gpio_pin_configure_dt(&motor->input_b, GPIO_OUTPUT_INACTIVE);
        if (ret != 0) {
            return ret;
        }
        ret = pwm_set_dt(&motor->enable, kPwmPeriod, 0);
        if (ret != 0) {
            return ret;
        }
    }

    ret = gpio_pin_configure_dt(&fault_, GPIO_INPUT);
    if (ret != 0) {
        return ret;
    }

    return gpio_pin_set_dt(&reset_, 0);
}

int Drv8844::setVelocity(float left, float right)
{
    int ret = setMotor(left_, left);
    if (ret != 0) {
        int stop_ret = stop();
        return stop_ret != 0 ? stop_ret : ret;
    }

    ret = setMotor(right_, right);
    if (ret != 0) {
        int stop_ret = stop();
        return stop_ret != 0 ? stop_ret : ret;
    }
    return ret;
}

int Drv8844::stop()
{
    int left_ret = pwm_set_dt(&left_.enable, kPwmPeriod, 0);
    int right_ret = pwm_set_dt(&right_.enable, kPwmPeriod, 0);
    int left_a_ret = gpio_pin_set_dt(&left_.input_a, 0);
    int left_b_ret = gpio_pin_set_dt(&left_.input_b, 0);
    int right_a_ret = gpio_pin_set_dt(&right_.input_a, 0);
    int right_b_ret = gpio_pin_set_dt(&right_.input_b, 0);

    if (left_ret != 0) return left_ret;
    if (right_ret != 0) return right_ret;
    if (left_a_ret != 0) return left_a_ret;
    if (left_b_ret != 0) return left_b_ret;
    if (right_a_ret != 0) return right_a_ret;
    return right_b_ret;
}

int Drv8844::readFault(bool& faulted) const
{
    int value = gpio_pin_get_dt(&fault_);
    if (value < 0) {
        return value;
    }

    faulted = value != 0;
    return 0;
}

int Drv8844::setMotor(const Drv8844Motor& motor, float velocity)
{
    if (!(velocity >= -1.0f && velocity <= 1.0f)) {
        return -EINVAL;
    }

    float duty = velocity < 0.0f ? -velocity : velocity;
    uint32_t pulse = static_cast<uint32_t>(kPwmPeriod * duty);
    int ret = pwm_set_dt(&motor.enable, kPwmPeriod, 0);
    if (ret != 0) {
        return ret;
    }

    ret = gpio_pin_set_dt(&motor.input_a, velocity > 0.0f);
    if (ret != 0) {
        return ret;
    }
    ret = gpio_pin_set_dt(&motor.input_b, velocity < 0.0f);
    if (ret != 0) {
        return ret;
    }

    return pwm_set_dt(&motor.enable, kPwmPeriod, pulse);
}
