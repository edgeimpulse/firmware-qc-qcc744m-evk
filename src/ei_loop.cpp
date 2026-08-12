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
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "ei_loop.h"
#include "data-fwd/data_fw.h"

#include "inference/inferencing.h"

#define EI_STATE_CHANGED_EVENT BIT(0)

static atomic_t g_app_state = ATOMIC_INIT(EI_APP_STATE_IDLE);
static struct k_event g_state_event;   /* drives the main loop */
static struct k_event g_notify_event;  /* broadcast to observers (e.g. LEDs) */

/**
 * Set the application state.
 */
void ei_set_app_state(enum ei_app_state new_state)
{
    atomic_set(&g_app_state, (atomic_val_t)new_state);
    k_event_post(&g_state_event, EI_STATE_CHANGED_EVENT);
    k_event_post(&g_notify_event, EI_STATE_CHANGED_EVENT);
}

/**
 * Block until the application state changes (for observers such as a status
 * LED). Clears the notification after waiting rather than before, so a change
 * that lands while the observer is busy is never lost.
 */
void ei_state_wait_change(void)
{
    k_event_wait(&g_notify_event, EI_STATE_CHANGED_EVENT, false, K_FOREVER);
    k_event_clear(&g_notify_event, EI_STATE_CHANGED_EVENT);
}

/**
 * Get the current application state.
 */
ei_app_state_t ei_get_app_state(void)
{
    return (ei_app_state_t)atomic_get(&g_app_state);
}

/**
 * Initialize the main loop and state event.
 */
void ei_loop_init(void)
{
    k_event_init(&g_state_event);
    k_event_init(&g_notify_event);
    k_event_post(&g_state_event, EI_STATE_CHANGED_EVENT);
}

/**
 * Main loop of the application.
 */
void ei_loop(void)
{
    ei_printf("Edge Impulse Zephyr SDK QCC744x EVK\n");

    while (1) {
        k_event_wait(&g_state_event, EI_STATE_CHANGED_EVENT, true, K_FOREVER);

        switch ((enum ei_app_state)atomic_get(&g_app_state)) {
        case EI_APP_STATE_IDLE:
            ei_printf("state: IDLE\n");
            break;
        case EI_APP_STATE_RUN_INFERENCE:
            if (ei_start_impulse()) {
                ei_printf("state: RUN INFERENCE\n");
            }
            else {
                ei_printf("ERR: Failed to start inference\n");
                ei_set_app_state(EI_APP_STATE_IDLE);
            }
            break;
        case EI_APP_STATE_RUN_INFERENCE_CONT:
            if (ei_start_impulse(true)) {
                ei_printf("state: RUN INFERENCE CONT\n");
            }
            else {
                ei_printf("ERR: Failed to start inference\n");
                ei_set_app_state(EI_APP_STATE_IDLE);
            }
            break;
        case EI_APP_STATE_RUN_INFERENCE_DEBUG:
            if (ei_start_impulse(false, true)) {
                ei_printf("state: RUN INFERENCE DEBUG\n");
            }
            else {
                ei_printf("ERR: Failed to start inference\n");
                ei_set_app_state(EI_APP_STATE_IDLE);
            }
            break;
        case EI_APP_STATE_STOP_INFERENCE:
            ei_printf("state: STOP INFERENCE\n");
            ei_stop_impulse();
            ei_set_app_state(EI_APP_STATE_IDLE);
            break;
        case EI_APP_STATE_RUN_DATA_FWD:
            if (data_fw_init()) {
                ei_printf("state: RUN DATA FORWARD\n");
            }
            else {
                ei_printf("ERR: Failed to start data forwarding\n");
                ei_set_app_state(EI_APP_STATE_IDLE);
            }
            break;
        case EI_APP_STATE_STOP_DATA_FWD:
            if (data_fw_deinit()) {
                ei_printf("state: STOP DATA FORWARD\n");
            }
            else {
                ei_printf("ERR: Failed to stop data forwarding\n");
                ei_set_app_state(EI_APP_STATE_IDLE);
            }
            break;
        default:
            ei_printf("state: UNKNOWN\n");
            break;
        }
    }
}
