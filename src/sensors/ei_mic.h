#ifndef EI_MIC_H
#define EI_MIC_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ei_mic_init(void);
bool ei_mic_start(void);
bool ei_mic_stop(void);
bool ei_mic_read(float *out_samples, size_t max_samples, size_t *samples_read);

#ifdef __cplusplus
}
#endif

#endif /* EI_MIC_H */
