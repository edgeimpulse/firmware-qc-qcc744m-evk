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
#include <zephyr/drivers/uart.h>
#include <string.h>
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"

#include "msg_handler.h"
#include "ei_loop.h"

#define MSG_HANDLER_STACK_SIZE 1024
#define MSG_HANDLER_PRIORITY   5
#define MSG_HANDLER_RX_LINE_MAX 128
#define MSG_HANDLER_RX_QUEUE_LEN 4

enum msg_handler_cmd {
	MSG_HANDLER_CMD_UNKNOWN = 0,
	MSG_HANDLER_CMD_START_IMPULSE,
	MSG_HANDLER_CMD_START_IMPULSE_CONT,
	MSG_HANDLER_CMD_START_IMPULSE_DEBUG,
	MSG_HANDLER_CMD_STOP_IMPULSE,
	MSG_HANDLER_CMD_START_DATA_FWD,
	MSG_HANDLER_CMD_STOP_DATA_FWD
};

K_THREAD_STACK_DEFINE(msg_handler_stack, MSG_HANDLER_STACK_SIZE);
static struct k_thread msg_handler_thread_data;
static k_tid_t msg_handler_thread_id;
static const struct device *const msg_handler_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
K_MSGQ_DEFINE(msg_handler_rx_queue, MSG_HANDLER_RX_LINE_MAX, MSG_HANDLER_RX_QUEUE_LEN, 4);
static char msg_handler_isr_line[MSG_HANDLER_RX_LINE_MAX];
static size_t msg_handler_isr_idx;

static void msg_handler_uart_isr(const struct device *dev, void *user_data)
{
	unsigned char c;

	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &c, 1) != 1) {
			break;
		}

		if (c == '\r') {
			continue;
		}

		if (c == '\n') {
			msg_handler_isr_line[msg_handler_isr_idx] = '\0';
			(void)k_msgq_put(&msg_handler_rx_queue, msg_handler_isr_line, K_NO_WAIT);
			msg_handler_isr_idx = 0U;
			continue;
		}

		if (msg_handler_isr_idx < (MSG_HANDLER_RX_LINE_MAX - 1U)) {
			msg_handler_isr_line[msg_handler_isr_idx++] = (char)c;
		} else {
			msg_handler_isr_idx = 0U;
		}
	}
}

static enum msg_handler_cmd msg_handler_decode_cmd(const char *line)
{
	const size_t at_prefix_len = strlen(MSG_AT_START);
	const char *cmd;

	if (line == NULL) {
		return MSG_HANDLER_CMD_UNKNOWN;
	}
	ei_app_state_t app_state = ei_get_app_state();

	if (app_state == EI_APP_STATE_RUN_INFERENCE || app_state == EI_APP_STATE_RUN_INFERENCE_CONT || app_state == EI_APP_STATE_RUN_INFERENCE_DEBUG) {
		if (strncmp(line, MSG_STOP_IMPULSE, at_prefix_len) == 0) {
			return MSG_HANDLER_CMD_STOP_IMPULSE;
		}
		else
			return MSG_HANDLER_CMD_UNKNOWN;
	}

	if (strncmp(line, MSG_AT_START, at_prefix_len) != 0) {
		return MSG_HANDLER_CMD_UNKNOWN;
	}

	cmd = line + at_prefix_len;

	if (strcmp(cmd, MSG_START_IMPULSE) == 0) {
		return MSG_HANDLER_CMD_START_IMPULSE;
	}

	if (strcmp(cmd, MSG_START_IMPULSE_CONT) == 0) {
		return MSG_HANDLER_CMD_START_IMPULSE_CONT;
	}

	if (strcmp(cmd, MSG_START_IMPULSE_DEBUG) == 0) {
		return MSG_HANDLER_CMD_START_IMPULSE_DEBUG;
	}

	if (strcmp(cmd, MSG_START_DATA_FWD) == 0) {
		return MSG_HANDLER_CMD_START_DATA_FWD;
	}

	if (strcmp(cmd, MSG_STOP_DATA_FWD) == 0) {
		return MSG_HANDLER_CMD_STOP_DATA_FWD;
	}

	return MSG_HANDLER_CMD_UNKNOWN;
}

static void msg_handler_process_line(const char *line)
{
	enum msg_handler_cmd msg_cmd = msg_handler_decode_cmd(line);
	if (msg_cmd == MSG_HANDLER_CMD_UNKNOWN) {
		ei_printf("ERR: Unknown command: %s\n", line);
		return;
	}
	else {
		switch (msg_cmd) {
		case MSG_HANDLER_CMD_START_IMPULSE:
			ei_set_app_state(EI_APP_STATE_RUN_INFERENCE);
			break;
		case MSG_HANDLER_CMD_START_IMPULSE_CONT:
			ei_set_app_state(EI_APP_STATE_RUN_INFERENCE_CONT);
			break;
		case MSG_HANDLER_CMD_START_IMPULSE_DEBUG:
			ei_set_app_state(EI_APP_STATE_RUN_INFERENCE_DEBUG);
			break;
		case MSG_HANDLER_CMD_STOP_IMPULSE:
			ei_set_app_state(EI_APP_STATE_STOP_INFERENCE);
			break;
		case MSG_HANDLER_CMD_START_DATA_FWD:
			ei_set_app_state(EI_APP_STATE_RUN_DATA_FWD);
			break;
		case MSG_HANDLER_CMD_STOP_DATA_FWD:
			ei_set_app_state(EI_APP_STATE_STOP_DATA_FWD);
			break;
		default:
			ei_printf("ERR: Unknown command: %s\n", line);
			break;
		}
	}


}

void msg_handler_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	char rx_line[MSG_HANDLER_RX_LINE_MAX];

	if (!device_is_ready(msg_handler_uart)) {
		return;
	}

	while (1) {
		if (k_msgq_get(&msg_handler_rx_queue, rx_line, K_FOREVER) == 0) {
			msg_handler_process_line(rx_line);
		}
	}
}

void msg_handler_init(void)
{
	if (!device_is_ready(msg_handler_uart)) {
		return;
	}

	msg_handler_isr_idx = 0U;
	k_msgq_purge(&msg_handler_rx_queue);
	uart_irq_callback_user_data_set(msg_handler_uart, msg_handler_uart_isr, NULL);
	uart_irq_rx_enable(msg_handler_uart);

	msg_handler_thread_id = k_thread_create(&msg_handler_thread_data,
											msg_handler_stack,
											K_THREAD_STACK_SIZEOF(msg_handler_stack),
											msg_handler_task,
											NULL,
											NULL,
											NULL,
											MSG_HANDLER_PRIORITY,
											0,
											K_NO_WAIT);
	k_thread_name_set(msg_handler_thread_id, "msg_handler");
}
