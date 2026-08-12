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

#ifndef _EI_LOOP_H_
#define _EI_LOOP_H_

typedef enum ei_app_state {
    EI_APP_STATE_IDLE = 0,
    EI_APP_STATE_RUN_INFERENCE,
    EI_APP_STATE_RUN_INFERENCE_CONT,
    EI_APP_STATE_RUN_INFERENCE_DEBUG,
    EI_APP_STATE_STOP_INFERENCE,
    EI_APP_STATE_RUN_DATA_FWD,
    EI_APP_STATE_STOP_DATA_FWD,
} ei_app_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void ei_loop_init(void);
void ei_loop(void);
void ei_set_app_state(ei_app_state_t new_state);
ei_app_state_t ei_get_app_state(void);

/**
 * @brief Block until the application state changes.
 *
 * For observers (e.g. a status-LED task) that want to react to state
 * transitions without polling. Returns after any ei_set_app_state() call.
 */
void ei_state_wait_change(void);

#ifdef __cplusplus
}
#endif

#endif // EI_LOOP_H
