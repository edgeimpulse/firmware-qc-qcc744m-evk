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

#ifndef DATA_FW_H
#define DATA_FW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Sensors that data forwarding can stream. Only sensors with a data-forward
 * read path are listed. */
typedef enum {
	DATA_FW_SENSOR_INERTIAL = 0,
	DATA_FW_SENSOR_ADC,
	DATA_FW_SENSOR_COUNT,
} data_fw_sensor_t;

bool data_fw_init(void);
bool data_fw_deinit(void);

/** @return true while the data-forwarding task is running. */
bool data_fw_is_running(void);

/** @return the sensor currently selected for data forwarding. */
data_fw_sensor_t data_fw_get_sensor(void);

/**
 * @brief Cycle to the next data-forward sensor.
 *
 * The selection is fixed while forwarding is running, so this is a no-op (and
 * returns the unchanged selection) unless data forwarding is stopped.
 *
 * @return the selection after the call.
 */
data_fw_sensor_t data_fw_toggle_sensor(void);

/** @return a human-readable name for @p sensor. */
const char *data_fw_sensor_name(data_fw_sensor_t sensor);

#ifdef __cplusplus
}
#endif

#endif // DATA_FW_H
