#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

#include "drv8844.hpp"

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), left_enable_pwms) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), right_enable_pwms) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), left_input_a_gpios) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), left_input_b_gpios) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), right_input_a_gpios) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), right_input_b_gpios) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), reset_gpios) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), fault_gpios)

namespace {
constexpr Drv8844Motor left_motor = {
    PWM_DT_SPEC_GET(DT_PATH(zephyr_user), left_enable_pwms),
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), left_input_a_gpios),
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), left_input_b_gpios),
};
constexpr Drv8844Motor right_motor = {
    PWM_DT_SPEC_GET(DT_PATH(zephyr_user), right_enable_pwms),
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), right_input_a_gpios),
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), right_input_b_gpios),
};
constexpr gpio_dt_spec reset_pin =
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), reset_gpios);
constexpr gpio_dt_spec fault_pin =
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), fault_gpios);

Drv8844 motors(left_motor, right_motor, reset_pin, fault_pin);
atomic_t fault_latched;
atomic_t motors_ready;
K_MUTEX_DEFINE(motor_mutex);

int commandSet(const shell* sh, size_t argc, char** argv)
{
    ARG_UNUSED(argc);
    char* left_end = nullptr;
    char* right_end = nullptr;
    float left = strtof(argv[1], &left_end);
    float right = strtof(argv[2], &right_end);

    if (*argv[1] == '\0' || *left_end != '\0' ||
        *argv[2] == '\0' || *right_end != '\0' ||
        !(left >= -1.0f && left <= 1.0f) ||
        !(right >= -1.0f && right <= 1.0f)) {
        shell_error(sh, "Vitesses attendues entre -1.0 et 1.0.");
        return -EINVAL;
    }
    if (atomic_get(&motors_ready) == 0) {
        shell_error(sh, "Le driver moteur n'est pas pret.");
        return -EAGAIN;
    }
    if (atomic_get(&fault_latched) != 0) {
        shell_error(sh, "Defaut moteur detecte; redemarrez apres verification.");
        return -EIO;
    }

    k_mutex_lock(&motor_mutex, K_FOREVER);
    int ret = motors.setVelocity(left, right);
    k_mutex_unlock(&motor_mutex);
    if (ret != 0) {
        shell_error(sh, "Echec de commande moteur (%d).", ret);
        return ret;
    }
    shell_print(sh, "Vitesses appliquees.");
    return 0;
}

int commandStop(const shell* sh, size_t argc, char** argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (atomic_get(&motors_ready) == 0) {
        shell_error(sh, "Le driver moteur n'est pas pret.");
        return -EAGAIN;
    }
    k_mutex_lock(&motor_mutex, K_FOREVER);
    int ret = motors.stop();
    k_mutex_unlock(&motor_mutex);
    if (ret != 0) {
        shell_error(sh, "Echec de l'arret moteur (%d).", ret);
        return ret;
    }
    shell_print(sh, "Moteurs arretes.");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    motor_commands,
    SHELL_CMD(set, nullptr, "Vitesses gauche et droite entre -1 et 1",
              commandSet),
    SHELL_CMD(stop, nullptr, "Arreter les moteurs", commandStop),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(motor, &motor_commands, "Commande des moteurs DRV8844",
                   nullptr);
} // namespace

int main()
{
    int ret = motors.init();
    if (ret != 0) {
        printk("Initialisation DRV8844 impossible (%d); moteurs a l'arret.\n",
               ret);
        return ret;
    }

    ret = motors.stop();
    if (ret != 0) {
        printk("Impossible de confirmer l'arret des moteurs (%d).\n", ret);
        return ret;
    }
    atomic_set(&motors_ready, 1);

    printk("DRV8844 pret. Utilisez 'motor set <gauche> <droite>' ou "
           "'motor stop'.\n");

    while (true) {
        bool faulted = false;
        ret = motors.readFault(faulted);
        if (ret != 0) {
            if (atomic_cas(&fault_latched, 0, 1)) {
                printk("Lecture du defaut DRV8844 impossible (%d).\n", ret);
            }
        } else if (faulted && atomic_cas(&fault_latched, 0, 1)) {
            printk("Defaut DRV8844 detecte; arret des moteurs.\n");
        }

        if (atomic_get(&fault_latched) != 0) {
            k_mutex_lock(&motor_mutex, K_FOREVER);
            ret = motors.stop();
            k_mutex_unlock(&motor_mutex);
            static bool stop_failure_reported = false;
            if (ret != 0 && !stop_failure_reported) {
                printk("Echec de l'arret apres defaut (%d).\n", ret);
                stop_failure_reported = true;
            }
        }
        k_sleep(K_MSEC(20));
    }
    return 0;
}

#else

int main()
{
    printk("Brochage DRV8844 manquant. Configurez app/boards/"
           "nucleo_h723zg.overlay avant d'utiliser les moteurs.\n");
    return 0;
}

#endif
