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
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/audio/dmic.h>

#include <errno.h>
#include <stdint.h>

#include "sensors/ei_mic.h"

#define EI_MIC_SAMPLE_RATE_HZ    16000
#define EI_MIC_SAMPLE_BITS       16
#define EI_MIC_BYTES_PER_SAMPLE  (EI_MIC_SAMPLE_BITS / 8)
#define EI_MIC_BLOCK_MS          100
#define EI_MIC_BLOCK_SIZE        ((EI_MIC_SAMPLE_RATE_HZ * EI_MIC_BLOCK_MS * EI_MIC_BYTES_PER_SAMPLE) / 1000)
#define EI_MIC_BUF_COUNT         4
#define EI_MIC_READ_TIMEOUT_MS   1000

#if DT_HAS_COMPAT_STATUS_OKAY(st_mpxxdtyy)
static const struct device *const ei_mic_dev = DEVICE_DT_GET_ONE(st_mpxxdtyy);
#define EI_MIC_HAS_DEVICE 1
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(pdm0), okay)
static const struct device *const ei_mic_dev = DEVICE_DT_GET(DT_NODELABEL(pdm0));
#define EI_MIC_HAS_DEVICE 1
#else
#define EI_MIC_HAS_DEVICE 0
#endif

K_MEM_SLAB_DEFINE_STATIC(ei_mic_mem_slab, EI_MIC_BLOCK_SIZE, EI_MIC_BUF_COUNT, 4);

static bool ei_mic_sampling;

bool ei_mic_init(void)
{
#if EI_MIC_HAS_DEVICE
	if (!device_is_ready(ei_mic_dev)) {
		return false;
	}
	return true;
#else
	return false;
#endif
}

bool ei_mic_start(void)
{
#if EI_MIC_HAS_DEVICE
	struct pcm_stream_cfg stream = {
		.pcm_rate = EI_MIC_SAMPLE_RATE_HZ,
		.pcm_width = EI_MIC_SAMPLE_BITS,
		.block_size = EI_MIC_BLOCK_SIZE,
		.mem_slab = &ei_mic_mem_slab,
	};

	struct dmic_cfg cfg = {
		.io = {
			.min_pdm_clk_freq = 1000000,
			.max_pdm_clk_freq = 3500000,
			.min_pdm_clk_dc = 40,
			.max_pdm_clk_dc = 60,
		},
		.streams = &stream,
		.channel = {
			.req_num_streams = 1,
			.req_num_chan = 1,
			.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT),
		},
	};

	if (ei_mic_sampling) {
		return true;
	}

	if (dmic_configure(ei_mic_dev, &cfg) < 0) {
		return false;
	}

	if (dmic_trigger(ei_mic_dev, DMIC_TRIGGER_START) < 0) {
		return false;
	}

	ei_mic_sampling = true;
	return true;
#else
	return false;
#endif
}

bool ei_mic_stop(void)
{
#if EI_MIC_HAS_DEVICE
	if (!ei_mic_sampling) {
		return true;
	}

	ei_mic_sampling = false;
	return (dmic_trigger(ei_mic_dev, DMIC_TRIGGER_STOP) == 0);
#else
	return false;
#endif
}

bool ei_mic_read(float *out_samples, size_t max_samples, size_t *samples_read)
{
#if EI_MIC_HAS_DEVICE
	void *buffer = NULL;
	uint32_t size = 0;
	int16_t *raw_samples;
	size_t raw_count;
	size_t n;

	if (samples_read != NULL) {
		*samples_read = 0U;
	}

	if (!ei_mic_sampling || out_samples == NULL || max_samples == 0U) {
		return false;
	}

	if (dmic_read(ei_mic_dev, 0, &buffer, &size, EI_MIC_READ_TIMEOUT_MS) < 0) {
		return false;
	}

	if (buffer == NULL || size == 0U) {
		return false;
	}

	raw_samples = (int16_t *)buffer;
	raw_count = size / sizeof(int16_t);
	n = (raw_count < max_samples) ? raw_count : max_samples;

	for (size_t i = 0; i < n; i++) {
		out_samples[i] = (float)raw_samples[i] / 32768.0f;
	}

	k_mem_slab_free(&ei_mic_mem_slab, buffer);

	if (samples_read != NULL) {
		*samples_read = n;
	}

	return true;
#else
	ARG_UNUSED(out_samples);
	ARG_UNUSED(max_samples);
	if (samples_read != NULL) {
		*samples_read = 0U;
	}
	return false;
#endif
}
