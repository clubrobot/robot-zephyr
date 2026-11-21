#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct device* lidar = DEVICE_DT_GET(DT_ALIAS(lidar));

static const struct device *const qright = DEVICE_DT_GET(DT_ALIAS(right_encoder));
static const struct device *const qleft = DEVICE_DT_GET(DT_ALIAS(left_encoder));

int main(void)
{
    int ret;

    if (!device_is_ready(led.port))
    {
        return -1;
    }

    if (!device_is_ready(lidar))
    {
        LOG_ERR_DEVICE_NOT_READY(lidar);
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        return -1;
    }

    struct sensor_value result;
    uint16_t distance;
    float angle;

    while (1)
    {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0)
        {
            return 0;
        }

        k_msleep(20);


        struct sensor_value v;
        int32_t counts;

        sensor_sample_fetch(qright);
        sensor_channel_get(qright, SENSOR_CHAN_ENCODER_COUNT, &v);
        counts = v.val1;

        sensor_channel_get(qright, SENSOR_CHAN_ROTATION, &v);
        float rotation = sensor_value_to_float(&v);

        printk("TIM23: %d  %f\n", counts, rotation);

        if (sensor_sample_fetch(lidar) != 0)
        {
            LOG_INF("No sensor data available");
        }
        else
        {
            uint8_t status = sensor_channel_get(lidar, SENSOR_CHAN_DISTANCE, &result);
            distance = result.val1;


            status = sensor_channel_get(lidar, SENSOR_CHAN_ROTATION, &result);
            angle = sensor_value_to_float(&result);

            printk("Result from the sensor : %d mm %f rad \r\n", distance, angle);
        }
    }
}
