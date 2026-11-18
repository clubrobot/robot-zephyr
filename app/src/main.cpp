#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include <drivers/motors.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device * const drv8844 = DEVICE_DT_GET(DT_ALIAS(left_motor));

int main()
{
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
        struct sensor_value val;
        sensor_value_from_float(&val, static_cast<float>(n) / 10);
        motors_set(drv8844, MOTOR_SPEED, 0, &val);
        n++;
        n %= 11;
        printk("Set speed to %d\n", n / 10);
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_sleep(Z_TIMEOUT_MS(1000));
    }
}
