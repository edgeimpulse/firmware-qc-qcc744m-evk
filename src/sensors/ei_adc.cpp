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

#include "ei_adc.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ei_adc, CONFIG_ADC_LOG_LEVEL);

#define EI_ADC_NODE DT_NODELABEL(adc0)
//#define EI_ADC_NODE DT_ALIAS(adc0)
#define EI_ADC_CHANNEL_ID 0
#define EI_ADC_INPUT_POSITIVE 0
#define EI_ADC_RESOLUTION 12

static const struct device *const adc_dev = DEVICE_DT_GET(EI_ADC_NODE);

static bool adc_ready = false;

bool ei_adc_init(void)
{
    if (adc_ready) {
        return true;
    }

	if (!device_is_ready(adc_dev)) {
		printk("ADC device is not ready");
		return false;
	}

	struct adc_channel_cfg channel_cfg = {};

	channel_cfg.gain = ADC_GAIN_1;
	channel_cfg.reference = ADC_REF_INTERNAL;
	channel_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;
	channel_cfg.channel_id = EI_ADC_CHANNEL_ID;
	channel_cfg.differential = 0;
	channel_cfg.input_positive = EI_ADC_INPUT_POSITIVE;

	if (adc_channel_setup(adc_dev, &channel_cfg) != 0) {
		printk("adc_channel_setup failed\n");
		return false;
	}

	adc_ready = true;
	return true;
}

bool ei_adc_read(uint16_t *raw_value)
{
	if (raw_value == NULL) {
		return false;
	}

	if (!adc_ready && !ei_adc_init()) {
        printk("ADC not ready and initialization failed\n");
		return false;
	}

	uint16_t sample_buffer = 0;
	struct adc_sequence sequence = {};

	sequence.channels = BIT(EI_ADC_CHANNEL_ID);
	sequence.buffer = &sample_buffer;
	sequence.buffer_size = sizeof(sample_buffer);
	sequence.resolution = EI_ADC_RESOLUTION;

	if (adc_read(adc_dev, &sequence) != 0) {
		printk("adc_read failed\n");
		return false;
	}

	*raw_value = sample_buffer;

	return true;
}