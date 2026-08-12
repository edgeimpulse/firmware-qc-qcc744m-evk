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

#ifndef EI_PIXELS_H
#define EI_PIXELS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Control interface for the Arduino Modulino Pixels board (8 addressable RGB
 * LEDs on the Qwiic/I2C bus, driven through Zephyr's LED-strip API).
 *
 * LEDs are addressed by a bitmask: bit i selects LED i.
 *   led0 = 1<<0 = 0x01, led1 = 1<<1 = 0x02, led2 = 1<<2 = 0x04, ...
 * Use EI_PIXEL(i) to build a bit, or OR several together, e.g. turn on LED 0
 * and LED 2 with (EI_PIXEL(0) | EI_PIXEL(2)) == 0x05.
 */

/** Number of LEDs on the Modulino Pixels board. */
#define EI_PIXELS_COUNT 8U

/** Bit selecting LED @p i (0-based). */
#define EI_PIXEL(i) (1U << (i))

/** Mask selecting every LED. */
#define EI_PIXELS_ALL ((1U << EI_PIXELS_COUNT) - 1U)

/**
 * @brief Initialise the pixels board.
 * @return true if the device is present and ready, false if not found.
 */
bool ei_pixels_init(void);

/**
 * @brief Turn on the selected LEDs using the current on-colour (white by
 *        default; change it with ei_pixels_set_color()).
 * @param mask bitmask of LEDs to light (EI_PIXEL(i) ...)
 * @return true on success
 */
bool ei_pixels_on(uint32_t mask);

/**
 * @brief Turn off the selected LEDs.
 * @param mask bitmask of LEDs to clear
 * @return true on success
 */
bool ei_pixels_off(uint32_t mask);

/**
 * @brief Toggle the selected LEDs (off ones turn on with the on-colour, lit
 *        ones turn off), each judged individually.
 * @param mask bitmask of LEDs to toggle
 * @return true on success
 */
bool ei_pixels_toggle(uint32_t mask);

/**
 * @brief Set the selected LEDs to an explicit colour. A colour of (0,0,0) is
 *        equivalent to turning them off. When any channel is non-zero the
 *        colour also becomes the on-colour used by ei_pixels_on()/toggle().
 * @param mask bitmask of LEDs to set
 * @return true on success
 */
bool ei_pixels_set_color(uint32_t mask, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Turn every LED off.
 * @return true on success
 */
bool ei_pixels_clear(void);

/**
 * @brief Start the status-indicator task.
 *
 * Spawns a task that blocks on application-state changes and colours the whole
 * strip to reflect the current state:
 *   - idle .................... green
 *   - inference running ....... yellow
 *   - data forward (inertial) . blue
 *   - data forward (adc) ...... magenta
 *
 * @return true if the task was started, false if the board is absent.
 */
bool ei_pixels_start_status_task(void);

#ifdef __cplusplus
}
#endif

#endif /* EI_PIXELS_H */
