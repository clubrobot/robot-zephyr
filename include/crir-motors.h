//
// Created by awing on 10/21/26.
//

#ifndef WORKSPACE_CRIR_MOTORS_H
#define WORKSPACE_CRIR_MOTORS_H

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

__subsystem struct crir_motors_speed_driver_api {
    /**
     *  @brief Change la vitesse du moteur
     *
     *  @param dev l'instance du moteur
     *  @param speed la vitesse à mettre
     *
     *  @retval 0 si succès
     */
    int (*set_speed)(const struct device *dev, float speed);
};

__subsystem struct crir_motors_position_driver_api {
    /**
     *  @brief Change la position du moteur
     *
     *  @param dev l'instance du moteur
     *  @param angle l'angle du moteur
     *
     *  @retval 0 si succès
     */
    int (*set_position)(const struct device *dev, float angle);
};

__syscall int crir_motors_speed(const struct device *dev, float speed);
__syscall int crir_motors_position(const struct device *dev, float angle);

static inline int z_impl_crir_motors_speed(const struct device *dev, float speed) {
    __ASSERT_NO_MSG(DEVICE_API_IS(crir_motors_speed, dev));
    return DEVICE_API_GET(crir_motors_speed, dev)->set_speed(speed);
}

static inline int z_impl_crir_motors_position(const struct device *dev, float angle) {
    __ASSERT_NO_MSG(DEVICE_API_IS(crir_motors_position, dev));
    return DEVICE_API_GET(crir_motors_position, dev)->set_position(angle);
}

#include <zephyr/syscalls/crir_motors_speed.h>
#include <zephyr/syscalls/crir_motors_position.h>

#endif //WORKSPACE_CRIR_MOTORS_H
