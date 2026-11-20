//
// Created by awing on 11/20/26.
// SPDX-License-Identifier: AGPL-3.0
//

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"

#define THREAD_STACK_SIZE 512
#define DEFAULT_PRIORITY 5

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

K_MSGQ_DEFINE(ihm_queue, sizeof(char[100]), 20, 1);

static void dummy(void* ihm_queue_tag,void*,void*){}

K_THREAD_DEFINE(ihm_thread, CONFIG_IHM_THREAD_STACK_SIZE, dummy, &ihm_queue, NULL, NULL, CONFIG_IHM_THREAD_PRIORITY, 0, -1);
K_THREAD_DEFINE(motor_thread, CONFIG_MOTOR_THREAD_STACK_SIZE, dummy, &ihm_queue, NULL, NULL, CONFIG_MOTOR_THREAD_PRIORITY, 0, -1);
K_THREAD_DEFINE(sensor_thread, CONFIG_SENSOR_THREAD_STACK_SIZE, dummy, &ihm_queue, NULL, NULL, CONFIG_SENSOR_THREAD_PRIORITY, 0, -1);

int main() {



    k_sched_lock();
    k_thread_start(ihm_thread);
    k_thread_start(motor_thread);
    k_thread_start(sensor_thread);
    k_sched_unlock();

    while (true) {
        
        k_yield();
    }
}