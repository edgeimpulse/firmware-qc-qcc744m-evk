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
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <stdio.h>

#include "sensors/ei_buttons.h"
#include "ei_loop.h"
#include "data-fwd/data_fw.h"

/*
 * The Modulino Buttons are handled by the in-tree arduino,modulino-buttons
 * input driver, which polls the module over I2C (default 50 ms) and reports a
 * key event on every state change: value == 1 on press, value == 0 on release.
 *
 * We register an input callback that forwards only *releases* into a small
 * message queue, and run a dedicated task whose while(1) blocks on that queue.
 * The task therefore consumes zero CPU until a button is actually released,
 * which is what "wake up on release" means here (the module has no interrupt
 * line on the Qwiic connector, so the driver's I2C poll is the wake source).
 */

#if DT_HAS_COMPAT_STATUS_OKAY(arduino_modulino_buttons)
static const struct device *const ei_buttons_dev =
	DEVICE_DT_GET_ONE(arduino_modulino_buttons);
#define EI_BUTTONS_HAS_DEVICE 1
#else
#define EI_BUTTONS_HAS_DEVICE 0
#endif

#if EI_BUTTONS_HAS_DEVICE

#define EI_BUTTONS_STACK_SIZE 1024
#define EI_BUTTONS_PRIORITY   7
#define EI_BUTTONS_QUEUE_DEPTH 8

/* Released key codes, produced by the input callback and consumed by the task. */
K_MSGQ_DEFINE(ei_buttons_msgq, sizeof(uint16_t), EI_BUTTONS_QUEUE_DEPTH,
	      sizeof(uint16_t));

static K_THREAD_STACK_DEFINE(ei_buttons_stack, EI_BUTTONS_STACK_SIZE);
static struct k_thread ei_buttons_thread;

/*
 * Input-subsystem callback. Runs in the input thread on every key transition;
 * we forward only releases (value == 0) to the button task. Non-blocking: if the
 * task has fallen behind, drop the oldest pending release rather than stall the
 * input thread.
 */
static void ei_buttons_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY || evt->value != 0) {
		return; /* not a key event, or a press -> ignore */
	}

	uint16_t code = evt->code;

	if (k_msgq_put(&ei_buttons_msgq, &code, K_NO_WAIT) != 0) {
		uint16_t dropped;

		(void)k_msgq_get(&ei_buttons_msgq, &dropped, K_NO_WAIT);
		(void)k_msgq_put(&ei_buttons_msgq, &code, K_NO_WAIT);
	}
}

INPUT_CALLBACK_DEFINE(ei_buttons_dev, ei_buttons_input_cb, NULL);

/* The button task: sleeps until a release arrives, then handles it. */
static void ei_buttons_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint16_t code;

	while (1) {
		/*
		 * K_FOREVER: the task is fully descheduled here and only wakes
		 * when the input callback enqueues a released button.
		 */
		if (k_msgq_get(&ei_buttons_msgq, &code, K_FOREVER) != 0) {
			continue;
		}

		ei_app_state_t state = ei_get_app_state();
		bool inference_running =
			(state == EI_APP_STATE_RUN_INFERENCE ||
			 state == EI_APP_STATE_RUN_INFERENCE_CONT ||
			 state == EI_APP_STATE_RUN_INFERENCE_DEBUG);

		if (code == INPUT_KEY_A) {
			/* A: toggle inference -- start it, or stop it if running.
			 * Inference and data forwarding are mutually exclusive. */
			if (inference_running) {
				ei_set_app_state(EI_APP_STATE_STOP_INFERENCE);
			} else if (data_fw_is_running()) {
				printf("Data forward running; stop it before "
				       "starting inference\n");
			} else {
				ei_set_app_state(EI_APP_STATE_RUN_INFERENCE);
			}
		} else if (code == INPUT_KEY_B) {
			/* B: toggle data forwarding (streams the selected sensor).
			 * Inference and data forwarding are mutually exclusive. */
			if (data_fw_is_running()) {
				ei_set_app_state(EI_APP_STATE_STOP_DATA_FWD);
			} else if (inference_running) {
				printf("Inference running; stop it before "
				       "starting data forward\n");
			} else {
				ei_set_app_state(EI_APP_STATE_RUN_DATA_FWD);
			}
		} else if (code == INPUT_KEY_C) {
			/* C: change the data-forward sensor, but only while data
			 * forwarding is stopped -- switching the source mid-stream
			 * would corrupt the sample flow. */
			if (data_fw_is_running()) {
				printf("Data forward running; stop it before "
				       "changing the sensor\n");
			} else {
				data_fw_sensor_t sensor = data_fw_toggle_sensor();

				printf("Data-forward sensor toggled to: %s\n",
				       data_fw_sensor_name(sensor));
			}
		}
	}
}

bool ei_buttons_init(void)
{
	if (!device_is_ready(ei_buttons_dev)) {
		return false;
	}

	k_thread_create(&ei_buttons_thread, ei_buttons_stack,
			K_THREAD_STACK_SIZEOF(ei_buttons_stack),
			ei_buttons_task, NULL, NULL, NULL,
			EI_BUTTONS_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&ei_buttons_thread, "ei_buttons");

	return true;
}

#else /* !EI_BUTTONS_HAS_DEVICE */

bool ei_buttons_init(void)
{
	return false;
}

#endif /* EI_BUTTONS_HAS_DEVICE */
