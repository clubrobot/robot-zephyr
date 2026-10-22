//
// Created by awing on 10/21/26.
// SPDX-License-Identifier: AGPL-3.0
//

#ifndef WORKSPACE_CRIR_MOTORS_H
#define WORKSPACE_CRIR_MOTORS_H

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

extern "C" {
    __subsystem struct crir_motors_speed_driver_api {
        /**
         *  @brief Change la vitesse du moteur
         *
         *  @param dev l'instance du moteur
         *  @param channel le channel du moteur à modifier
         *  @param speed la vitesse à mettre
         *
         *  @retval 0 si succès
         */
        int (*set_speed)(const struct device *dev, uint8_t channel, float speed);
    };

    __subsystem struct crir_motors_position_driver_api {
        /**
         *  @brief Change la position du moteur
         *
         *  @param dev l'instance du moteur
         *  @param channel le channel du moteur à modifier
         *  @param angle l'angle du moteur
         *
         *  @retval 0 si succès
         */
        int (*set_position)(const struct device *dev, uint8_t channel, float angle);
    };

    __syscall int crir_motors_speed(const struct device *dev, uint8_t channel, float speed);
    __syscall int crir_motors_position(const struct device *dev, uint8_t channel, float angle);

    static inline int z_impl_crir_motors_speed(const struct device *dev, uint8_t channel, float speed) {
        __ASSERT_NO_MSG(DEVICE_API_IS(crir_motors_speed, dev));
        return DEVICE_API_GET(crir_motors_speed, dev)->set_speed(dev, channel, speed);
    }

    static inline int z_impl_crir_motors_position(const struct device *dev, uint8_t channel, float angle) {
        __ASSERT_NO_MSG(DEVICE_API_IS(crir_motors_position, dev));
        return DEVICE_API_GET(crir_motors_position, dev)->set_position(dev, channel, angle);
    }
}
#include <zephyr/syscalls/crir-motors.h>

#endif //WORKSPACE_CRIR_MOTORS_H
