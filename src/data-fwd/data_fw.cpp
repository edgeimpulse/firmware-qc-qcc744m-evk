/* The Clear BSD License
 *
 * Copyright (c) 2025 EdgeImpulse Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <zephyr/kernel.h>
#include "data_fw.h"
#include "sensors/ei_inertial.h"
#include "sensors/ei_adc.h"

#define DATA_FW_STACK_SIZE 1024
#define DATA_FW_PRIORITY   5

#define DATA_FW_SAMPLING_FREQUENCY_HZ CONFIG_SAMPLING_FREQUENCY_HZ
#define DATA_FW_PERIOD_MS (1000 / DATA_FW_SAMPLING_FREQUENCY_HZ)

#define INERTIAL_AXIS_SAMPLED 6

K_THREAD_STACK_DEFINE(data_fw_stack, DATA_FW_STACK_SIZE);
static struct k_thread data_fw_thread_data;
static k_tid_t data_fw_thread_id;
static bool data_fw_run = false;
static int8_t data_fw_sensor_idx = 0;	// default to inertial

struct sensor_list {
	char name[32];
	int8_t idx;
	uint8_t axis_sampled;
	float freq_hz;
	bool (*read_data_fwd)(float *data);
};

static struct sensor_list available_sensors[] = {
	{"Inertial", 0, INERTIAL_AXIS_SAMPLED, DATA_FW_SAMPLING_FREQUENCY_HZ, ei_inertial_read_data_fwd},
	{"ADC", 1, 1, DATA_FW_SAMPLING_FREQUENCY_HZ, ei_adc_read_data_fwd},
};

K_SEM_DEFINE(data_fw_tick_sem, 0, 1);

static void data_fw_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&data_fw_tick_sem);
}

K_TIMER_DEFINE(data_fw_timer, data_fw_timer_handler, NULL);

/**
 * @brief Data forwarding task
 */
static void data_fw_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);


	float* inertial_data = (float*)k_malloc(available_sensors[data_fw_sensor_idx].axis_sampled * sizeof(float));

	while (data_fw_run) {

		k_sem_take(&data_fw_tick_sem, K_FOREVER);
		if (!data_fw_run) {
			break;
		}

        if (available_sensors[data_fw_sensor_idx].read_data_fwd != NULL) {
            if (available_sensors[data_fw_sensor_idx].read_data_fwd(inertial_data)) {
                // Data read successfully, you can process or forward the data here
                for (int i = 0; i < available_sensors[data_fw_sensor_idx].axis_sampled; i++) {
                    if (i != 0) {
                        printk(",");
                    }
                    printk("%.3f", inertial_data[i]);
                }
                printk("\n");
            } else {
                printk("Sensor not present or failed to read data\n");
            }
        }
	}

	data_fw_thread_id = NULL;
}

/**
 * @brief Initialize the data forwarding task
 */
bool data_fw_init(void)
{
	if (data_fw_thread_id != NULL) {
		return false;
	}

	if (sizeof(available_sensors) / sizeof(available_sensors[0]) <= 0) {
		printk("No available sensors to forward data from\n");
		return false;
	}

	if (available_sensors[data_fw_sensor_idx].read_data_fwd == NULL) {
		printk("Selected sensor does not have a data forwarding function\n");
		return false;
	}

	if (data_fw_sensor_idx < 0 || data_fw_sensor_idx >= sizeof(available_sensors) / sizeof(available_sensors[0])) {
		printk("Invalid sensor index\n");
		return false;
	}

	data_fw_run = true;
	data_fw_thread_id = k_thread_create(&data_fw_thread_data,
									data_fw_stack,
									K_THREAD_STACK_SIZEOF(data_fw_stack),
									data_fw_task,
									NULL,
									NULL,
									NULL,
									DATA_FW_PRIORITY,
									0,
									K_NO_WAIT);
	k_thread_name_set(data_fw_thread_id, "data_fw");
	k_timer_start(&data_fw_timer, K_MSEC(DATA_FW_PERIOD_MS), K_MSEC(DATA_FW_PERIOD_MS));	// it's the same for both sensors now
	k_sem_reset(&data_fw_tick_sem);	// ensure it is empty before starting the task

	return true;
}

/**
 * @brief Deinitialize the data forwarding task
 */
bool data_fw_deinit(void)
{
	k_tid_t thread_id = data_fw_thread_id;

	if (thread_id == NULL) {
		return false;
	}

	data_fw_run = false;
	k_timer_stop(&data_fw_timer);
	k_sem_give(&data_fw_tick_sem);

	if (k_current_get() != thread_id) {
		(void)k_thread_join(&data_fw_thread_data, K_SECONDS(1));
		data_fw_thread_id = NULL;
	}

	return true;
}

/**
 * @brief Set the sensor to be used for data forwarding
 * @param sensor_idx Index of the sensor to be used
 */
void data_fw_set_sensor(int8_t sensor_idx)
{
	data_fw_sensor_idx = sensor_idx;
}

/**
 * @brief Print the list of available sensors
 */
void data_fw_get_sensor_list(char *sensor_list, size_t max_len)
{
	for (size_t i = 0; i < sizeof(available_sensors) / sizeof(available_sensors[0]); i++) {
		printk("%d: %s %.2f\n", available_sensors[i].idx, available_sensors[i].name, available_sensors[i].freq_hz);
	}
}
