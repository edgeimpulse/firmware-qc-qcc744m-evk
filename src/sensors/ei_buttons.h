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

#ifndef EI_BUTTONS_H
#define EI_BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Handling for the Arduino Modulino Buttons board (three buttons A/B/C on the
 * Qwiic/I2C bus). The in-tree Zephyr driver (arduino,modulino-buttons) polls the
 * module over I2C and emits key events through the input subsystem; this wrapper
 * runs a task that wakes on each *release* and acts on it:
 *
 *   A -> start inference
 *   B -> start data forwarding (streams the selected sensor)
 *   C -> switch the data-forward sensor (only while data forwarding is stopped)
 *
 * Key codes match the devicetree "zephyr,codes" and are the standard Zephyr
 * INPUT_KEY_A / INPUT_KEY_B / INPUT_KEY_C values.
 */

/**
 * @brief Initialise the Modulino Buttons board and start the button task.
 *
 * The task runs a while(1) loop that sleeps with zero CPU cost until a button
 * is released, then dispatches the action bound to that button.
 *
 * @return true if the device is present and the task was started, false if the
 *         board is absent / not ready.
 */
bool ei_buttons_init(void);

#ifdef __cplusplus
}
#endif

#endif /* EI_BUTTONS_H */
