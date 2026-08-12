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
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>

#include <string.h>

#include "sensors/ei_pixels.h"
#include "ei_loop.h"
#include "data-fwd/data_fw.h"

/*
 * The board is an LED strip (arduino,modulino-pixels) reached over I2C. The
 * strip API has no "set one LED" primitive: the whole pixel array is pushed on
 * every update. We therefore keep a shadow copy of all pixels, mutate the ones
 * a caller selects, and flush the array. A moderate default on-colour is used
 * so ei_pixels_on() is bright but not blinding at full 8-LED white.
 */

#if DT_HAS_COMPAT_STATUS_OKAY(arduino_modulino_pixels)
static const struct device *const ei_pixels_dev =
	DEVICE_DT_GET_ONE(arduino_modulino_pixels);
#define EI_PIXELS_HAS_DEVICE 1
#else
#define EI_PIXELS_HAS_DEVICE 0
#endif

#if EI_PIXELS_HAS_DEVICE

/* Shadow of the strip; flushed to the device on every change. */
static struct led_rgb ei_pixels_buf[EI_PIXELS_COUNT];

/* Number of LEDs actually reported by the driver (clamped to EI_PIXELS_COUNT). */
static size_t ei_pixels_len;

/* Colour used by ei_pixels_on()/toggle(). Default: moderate white. */
static struct led_rgb ei_pixels_on_color = { .r = 0x20, .g = 0x20, .b = 0x20 };

static bool ei_pixels_ready;

static inline bool ei_pixels_is_lit(size_t i)
{
	return ei_pixels_buf[i].r || ei_pixels_buf[i].g || ei_pixels_buf[i].b;
}

static bool ei_pixels_flush(void)
{
	return led_strip_update_rgb(ei_pixels_dev, ei_pixels_buf, ei_pixels_len) == 0;
}

/* Apply @color to every LED selected by @mask, then push the strip. */
static bool ei_pixels_apply(uint32_t mask, const struct led_rgb *color)
{
	if (!ei_pixels_ready) {
		return false;
	}

	for (size_t i = 0; i < ei_pixels_len; i++) {
		if (mask & EI_PIXEL(i)) {
			ei_pixels_buf[i] = *color;
		}
	}

	return ei_pixels_flush();
}

bool ei_pixels_init(void)
{
	if (!device_is_ready(ei_pixels_dev)) {
		return false;
	}

	ei_pixels_len = led_strip_length(ei_pixels_dev);
	if (ei_pixels_len > EI_PIXELS_COUNT) {
		ei_pixels_len = EI_PIXELS_COUNT;
	}

	memset(ei_pixels_buf, 0, sizeof(ei_pixels_buf));
	ei_pixels_ready = true;

	/* Start with everything off. */
	return ei_pixels_flush();
}

bool ei_pixels_on(uint32_t mask)
{
	return ei_pixels_apply(mask, &ei_pixels_on_color);
}

bool ei_pixels_off(uint32_t mask)
{
	static const struct led_rgb black = { .r = 0, .g = 0, .b = 0 };

	return ei_pixels_apply(mask, &black);
}

bool ei_pixels_toggle(uint32_t mask)
{
	if (!ei_pixels_ready) {
		return false;
	}

	for (size_t i = 0; i < ei_pixels_len; i++) {
		if (mask & EI_PIXEL(i)) {
			if (ei_pixels_is_lit(i)) {
				ei_pixels_buf[i] = (struct led_rgb){ 0, 0, 0 };
			} else {
				ei_pixels_buf[i] = ei_pixels_on_color;
			}
		}
	}

	return ei_pixels_flush();
}

bool ei_pixels_set_color(uint32_t mask, uint8_t r, uint8_t g, uint8_t b)
{
	struct led_rgb color = { .r = r, .g = g, .b = b };

	/* Remember an explicit non-black colour as the new on-colour. */
	if (r || g || b) {
		ei_pixels_on_color = color;
	}

	return ei_pixels_apply(mask, &color);
}

bool ei_pixels_clear(void)
{
	return ei_pixels_off(EI_PIXELS_ALL);
}

/*
 * Status-indicator task: reflect the application state on the whole strip.
 * Colours are moderate (0x20) to match the default on-colour brightness.
 */

#define EI_PIXELS_STATUS_STACK_SIZE 1024
#define EI_PIXELS_STATUS_PRIORITY   7
#define EI_PIXELS_STATUS_LEVEL      0x20

static K_THREAD_STACK_DEFINE(ei_pixels_status_stack, EI_PIXELS_STATUS_STACK_SIZE);
static struct k_thread ei_pixels_status_thread;

/* Paint the strip for the current application state. */
static void ei_pixels_apply_state(void)
{
	const uint8_t on = EI_PIXELS_STATUS_LEVEL;
	uint8_t r, g, b;

	switch (ei_get_app_state()) {
	case EI_APP_STATE_RUN_INFERENCE:
	case EI_APP_STATE_RUN_INFERENCE_CONT:
	case EI_APP_STATE_RUN_INFERENCE_DEBUG:
		/* inference running -> yellow */
		r = on; g = on; b = 0;
		break;
	case EI_APP_STATE_RUN_DATA_FWD:
		if (data_fw_get_sensor() == DATA_FW_SENSOR_ADC) {
			/* data forward, ADC -> magenta */
			r = on; g = 0; b = on;
		} else {
			/* data forward, inertial -> blue */
			r = 0; g = 0; b = on;
		}
		break;
	default:
		/* idle (and the transient stop states) -> green */
		r = 0; g = on; b = 0;
		break;
	}

	ei_pixels_set_color(EI_PIXELS_ALL, r, g, b);
}

static void ei_pixels_status_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Show the initial (idle) state, then update on every transition. */
	ei_pixels_apply_state();

	while (1) {
		ei_state_wait_change();
		ei_pixels_apply_state();
	}
}

bool ei_pixels_start_status_task(void)
{
	if (!ei_pixels_ready) {
		return false;
	}

	k_thread_create(&ei_pixels_status_thread, ei_pixels_status_stack,
			K_THREAD_STACK_SIZEOF(ei_pixels_status_stack),
			ei_pixels_status_task, NULL, NULL, NULL,
			EI_PIXELS_STATUS_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&ei_pixels_status_thread, "ei_pixels_status");

	return true;
}

#else /* !EI_PIXELS_HAS_DEVICE */

bool ei_pixels_init(void)
{
	return false;
}

bool ei_pixels_on(uint32_t mask)
{
	ARG_UNUSED(mask);
	return false;
}

bool ei_pixels_off(uint32_t mask)
{
	ARG_UNUSED(mask);
	return false;
}

bool ei_pixels_toggle(uint32_t mask)
{
	ARG_UNUSED(mask);
	return false;
}

bool ei_pixels_set_color(uint32_t mask, uint8_t r, uint8_t g, uint8_t b)
{
	ARG_UNUSED(mask);
	ARG_UNUSED(r);
	ARG_UNUSED(g);
	ARG_UNUSED(b);
	return false;
}

bool ei_pixels_clear(void)
{
	return false;
}

bool ei_pixels_start_status_task(void)
{
	return false;
}

#endif /* EI_PIXELS_HAS_DEVICE */
