#define DT_DRV_COMPAT beacon_tracker

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "vl53l5cx_api.h"

LOG_MODULE_REGISTER(beacon_tracker, CONFIG_SENSOR_LOG_LEVEL);

#define BEACON_NB_SENSORS           8
#define BEACON_SENSOR_FREQ_HZ       15
#define BEACON_SENSOR_BOOT_MS       30
#define PI_F	                    3.14159265358979323846F


struct beacon_config
{
    const struct device* i2c;
    uint8_t addr[BEACON_NB_SENSORS];
    struct gpio_dt_spec stcp, shcp, ds;
};

struct beacon_result
{
    uint16_t dist_mm;
    float angle_deg;
    uint8_t found;
};

struct beacon_data
{
    VL53L5CX_Configuration dev[BEACON_NB_SENSORS];
    uint8_t alive[BEACON_NB_SENSORS];
    struct beacon_result result;
    uint8_t power_mask;
    struct k_mutex lock;
};

static void beacon_apply_power(const struct beacon_config* c, uint8_t mask)
{
    gpio_pin_set_dt(&c->stcp, 0);
    gpio_pin_set_dt(&c->shcp, 0);

    for (int i = BEACON_NB_SENSORS - 1; i >= 0; i--)
    {
        //MSB first
        gpio_pin_set_dt(&c->shcp, 0);
        gpio_pin_set_dt(&c->ds, (mask >> i) & 1);
        k_busy_wait(1);

        //Shift on rising edge
        gpio_pin_set_dt(&c->shcp, 1);
        k_busy_wait(1);
    }
    gpio_pin_set_dt(&c->shcp, 0);
    gpio_pin_set_dt(&c->ds, 0);

    //Latch on rising edge
    gpio_pin_set_dt(&c->stcp, 1);
    k_busy_wait(1);
    gpio_pin_set_dt(&c->stcp, 0);
}


static int beacon_sample_fetch(const struct device* dev, enum sensor_channel chan)
{
    struct beacon_data* d = dev->data;
    uint8_t result = 0;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DISTANCE && chan != SENSOR_CHAN_ROTATION)
    {
        return -ENOTSUP;
    }

    k_mutex_lock(&d->lock, K_FOREVER);

    VL53L5CX_ResultsData data;
    d->result.found = 0;

    for (uint8_t i = 0; i < BEACON_NB_SENSORS; i++)
    {
        uint8_t ready = 0;

        //Skip the sensor if it was not started at boot
        if (!d->alive[i])
        {
            continue;
        }

        if (vl53l5cx_check_data_ready(&d->dev[i], &ready) != VL53L5CX_STATUS_OK || !ready)
        {
            continue;
        }

        if (vl53l5cx_get_ranging_data(&d->dev[i], &data) != VL53L5CX_STATUS_OK)
        {
            continue;
        }

        for (uint8_t j = 0; j < VL53L5CX_RESOLUTION_8X8; j++)
        {
            uint8_t status = data.target_status[j];

            //According to uld's guide, um2884 5, 9 and 10 status code doesn't impact mesurments
            if (status != 5 && status != 9 && status != 10)
            {
                continue;
            }

            //It's not the Tag
            if (data.reflectance[j] != 127)
            {
                continue;
            }

            //Find the closet tag measurement
            if (!d->result.found || d->result.dist_mm > data.distance_mm[j])
            {

                //The horizontal FOV is 45deg (PI/4), each sensor is rotated by PI/8
                //Each colum on of measurements is spaced by one 8th of 45deg hence PI/32
                d->result.angle_deg = floorf((float)j / 8.0F) * PI_F / 32.0f + PI_F / 8.0f * (float)i;
                d->result.dist_mm = data.distance_mm[j];
                d->result.found = 1;
            }
        }
    }

    if (!d->result.found)
    {
        result = ENODATA;
    }

    k_mutex_unlock(&d->lock);
    return result;
}

static int beacon_channel_get(const struct device* dev, enum sensor_channel chan, struct sensor_value* val)
{
    struct beacon_data* d = dev->data;
    int ret = 0;

    k_mutex_lock(&d->lock, K_FOREVER);
    if (!d->result.found)
    {
        //No beacon in sight
        ret = -ENODATA;
    }
    else if (chan == SENSOR_CHAN_DISTANCE)
    {
        //In mm
        val->val1 = d->result.dist_mm;
    }
    else if (chan == SENSOR_CHAN_ROTATION)
    {
        //In rads
        sensor_value_from_float(val, d->result.angle_deg);
    }
    else
    {
        ret = -ENOTSUP;
    }
    k_mutex_unlock(&d->lock);
    return ret;
}

static const struct sensor_driver_api beacon_api = {
    .sample_fetch = beacon_sample_fetch,
    .channel_get = beacon_channel_get,
};


static int beacon_init(const struct device* dev)
{
    const struct beacon_config* c = dev->config;
    struct beacon_data* d = dev->data;
    int started = 0;

    k_mutex_init(&d->lock);

    uint8_t status = VL53L5CX_STATUS_OK;

    //Check if the i2c bus is ready
    status |= !device_is_ready(c->i2c);

    //Check if all the pin of the shift register are ready
    status |= !gpio_is_ready_dt(&c->stcp);
    status |= !gpio_is_ready_dt(&c->shcp);
    status |= !gpio_is_ready_dt(&c->ds);

    if (status)
    {
        return -ENODEV;
    }

    //Addresses must be unique and different from the power-on default
    for (int i = 0; i < BEACON_NB_SENSORS; i++)
    {
        if (c->addr[i] == VL53L5CX_DEFAULT_I2C_ADDRESS >> 1)
        {
            LOG_ERR("address 0x29 is reserved for the default address");
            return -EINVAL;
        }

        //Check for duplicates
        for (int j = i + 1; j < BEACON_NB_SENSORS; j++)
        {
            if (c->addr[i] == c->addr[j])
            {
                LOG_ERR("duplicate address 0x%02x", c->addr[i]);
                return -EINVAL;
            }
        }
    }

    //Define all the pins as outputs
    gpio_pin_configure_dt(&c->stcp, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&c->shcp, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&c->ds, GPIO_OUTPUT_INACTIVE);

    //All sensors off
    d->power_mask = 0;
    beacon_apply_power(c, d->power_mask);

    //Required or else the sensors will not be powered in time to respond in i2c
    k_msleep(BEACON_SENSOR_BOOT_MS * 10);

    for (int i = 0; i < BEACON_NB_SENSORS; i++)
    {
        VL53L5CX_Configuration* p = &d->dev[i];
        uint8_t alive = 0;

        p->platform.i2c.bus = c->i2c;
        //Zephyr uses 7bit address (0x29)
        p->platform.i2c.addr = VL53L5CX_DEFAULT_I2C_ADDRESS >> 1;

        d->power_mask |= BIT(i);
        beacon_apply_power(c, d->power_mask);
        k_msleep(BEACON_SENSOR_BOOT_MS);

        status = vl53l5cx_is_alive(p, &alive);
        if (status != VL53L5CX_STATUS_OK || !alive)
        {
            LOG_WRN("sensor %d not detected at default address 0x%X, status : %d", i, p->platform.i2c.addr, status);

            //Try at the "future" address if already initialised
            p->platform.i2c.addr = c->addr[i];
            status = vl53l5cx_is_alive(p, &alive);

            if (status != VL53L5CX_STATUS_OK || !alive)
            {
                LOG_ERR("sensor %d not detected at 0x%X, status : %d", i, p->platform.i2c.addr, status);
                goto fail;
            }

            //If the address is already changed we can suppose that the firmware is already flashed
            goto skip_init;
        }

        //Flash the firmware
        status = vl53l5cx_init(p);
        if (status != VL53L5CX_STATUS_OK)
        {
            LOG_ERR("failed to initialize sensor %d, status : %d", i, status);
            goto fail;
        }


        //Move it away from the default address before powering the next one
        status = vl53l5cx_set_i2c_address(p, c->addr[i] << 1);
        if (status != VL53L5CX_STATUS_OK)
        {
            LOG_ERR("failed to change addresse of sensor %d, status : %d", i, status);
            goto fail;
        }

    skip_init:

        status = vl53l5cx_set_resolution(p, VL53L5CX_RESOLUTION_8X8);
        status |= vl53l5cx_set_ranging_mode(p, VL53L5CX_RANGING_MODE_CONTINUOUS);
        status |= vl53l5cx_set_ranging_frequency_hz(p, BEACON_SENSOR_FREQ_HZ);
        status |= vl53l5cx_start_ranging(p);
        if (status != VL53L5CX_STATUS_OK)
        {
            LOG_ERR("failed to configure sensor %d, status : %d", i, status);
            goto fail;
        }

        d->alive[i] = 1;
        started++;
        LOG_INF("sensor %d initialized successfully at 0x%02x", i, c->addr[i]);
        continue;

    fail:
        d->alive[i] = 0;

        //Don't enable this sensor
        d->power_mask &= ~BIT(i);
        beacon_apply_power(c, d->power_mask);
    }

    LOG_INF("started %d/%d sensors", started, BEACON_NB_SENSORS);
    return started ? 0 : -ENODEV;
}


#define BEACON_INST(n)                                                                          \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, addresses) == BEACON_NB_SENSORS,                           \
		     "addresses must contain exactly 8 entries");                                       \
	static struct beacon_data beacon_data_##n;                                                  \
	static const struct beacon_config beacon_cfg_##n = {                                        \
		.i2c = DEVICE_DT_GET(DT_INST_PROP(n, i2c_bus)),                                         \
		.addr = DT_INST_PROP(n, addresses),                                                     \
		.stcp = GPIO_DT_SPEC_INST_GET(n, stcp_gpios),                                           \
		.shcp = GPIO_DT_SPEC_INST_GET(n, shcp_gpios),                                           \
		.ds = GPIO_DT_SPEC_INST_GET(n, ds_gpios),                                               \
	};                                                                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(n, beacon_init, NULL, &beacon_data_##n, &beacon_cfg_##n,       \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &beacon_api);

DT_INST_FOREACH_STATUS_OKAY(BEACON_INST)
