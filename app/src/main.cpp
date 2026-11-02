//
// Created by awing on 10/22/26.
// SPDX-License-Identifier: AGPL-3.0
//


#include <stdio.h>
#include <zephyr/kernel.h>
#include <drivers/crir-motors.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN, CONFIG_LOG_DEFAULT_LEVEL);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct device * const drv8844 = DEVICE_DT_GET(DT_NODELABEL(drv8844_0));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main() {
    k_sleep(Z_TIMEOUT_MS(1000));
    printk("Starting...");
    if (!device_is_ready(drv8844)) {
        LOG_ERR("DRV8844 is not ready");
        return -1;
    }
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }


    int n = 0;
    while (true) {
        //crir_motors_set_speed(drv8844, 0, static_cast<float>(n) / 10);
        //n++;
        //n %= 11;
        printk("Set speed to %d", n);
        //k_sleep(Z_TIMEOUT_MS(1000));
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_sleep(Z_TIMEOUT_MS(1000));
    }

    return 0;
}