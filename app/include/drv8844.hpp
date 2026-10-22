#pragma once

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

struct Drv8844Motor {
    pwm_dt_spec enable;
    gpio_dt_spec input_a;
    gpio_dt_spec input_b;
};

class Drv8844 {
public:
    Drv8844(const Drv8844Motor& left, const Drv8844Motor& right,
            const gpio_dt_spec& reset, const gpio_dt_spec& fault);

    int init();
    int setVelocity(float left, float right);
    int stop();
    int readFault(bool& faulted) const;

private:
    int setMotor(const Drv8844Motor& motor, float velocity);

    Drv8844Motor left_;
    Drv8844Motor right_;
    gpio_dt_spec reset_;
    gpio_dt_spec fault_;
};
