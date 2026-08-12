/* The Clear BSD License
 *
 * Copyright (c) 2026 EdgeImpulse Inc.
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
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "sensors/ei_inertial.h"
#include "inference/inferencing.h"

const struct device *const lsm6dso = DEVICE_DT_GET_ONE(st_lsm6dso);

static bool _is_present = false;

static void lsm6dso_config(const struct device *lsm6dso)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LSM6DSO accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {

		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {

		return;
	}

	/* set LSM6DSO gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {

		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {

		return;
	}
}

bool ei_inertial_init(void)
{
	printk("Initializing inertial sensor\n");
	if (!device_is_ready(lsm6dso)) {
		printk("Inertial sensor not ready\n");
		return false;
	}

	_is_present = true;
	printk("lsm6dso_config\n");
    lsm6dso_config(lsm6dso);
	printk("done\n");

    return true;
}

float *ei_fusion_inertial_read_data(int n_samples)
{
    struct sensor_value accel[3];
    struct sensor_value gyro[3];
    static float inertial[INERTIAL_AXIS_SAMPLED];

    memset(inertial, 0, INERTIAL_AXIS_SAMPLED * sizeof(float));

	if (!_is_present) {
		return inertial;
	}

    if (sensor_sample_fetch(lsm6dso) < 0) {
		printk("error sensor_sample_fetch\n");
    }
    else {
		
        sensor_channel_get(lsm6dso, SENSOR_CHAN_ACCEL_XYZ, accel);		
        inertial[0] = sensor_value_to_double(&accel[0]);
        inertial[1] = sensor_value_to_double(&accel[1]);
        inertial[2] = sensor_value_to_double(&accel[2]);

		if (n_samples == 6) {
			sensor_channel_get(lsm6dso, SENSOR_CHAN_GYRO_XYZ, gyro);
			inertial[3] = sensor_value_to_double(&gyro[0]);
			inertial[4] = sensor_value_to_double(&gyro[1]);
			inertial[5] = sensor_value_to_double(&gyro[2]);
		}

		ei_samples_callback(inertial, n_samples * sizeof(float));
    }

    return inertial;
}

bool ei_inertial_read_data_fwd(float* intertial)
{
	if (!_is_present) {
		return false;
	}

	struct sensor_value accel[3];
	struct sensor_value gyro[3];

	if (sensor_sample_fetch(lsm6dso) < 0) {
		printk("error sensor_sample_fetch\n");
		return false;
	}
	else {
		
		sensor_channel_get(lsm6dso, SENSOR_CHAN_ACCEL_XYZ, accel);		
		intertial[0] = sensor_value_to_double(&accel[0]);
		intertial[1] = sensor_value_to_double(&accel[1]);
		intertial[2] = sensor_value_to_double(&accel[2]);

		sensor_channel_get(lsm6dso, SENSOR_CHAN_GYRO_XYZ, gyro);
		intertial[3] = sensor_value_to_double(&gyro[0]);
		intertial[4] = sensor_value_to_double(&gyro[1]);
		intertial[5] = sensor_value_to_double(&gyro[2]);
	}

	return true;
}

bool ei_inertial_is_present(void)
{
	return _is_present;
}
