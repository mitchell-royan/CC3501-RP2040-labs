#pragma once

#include <stdint.h>
#include <stddef.h>
#include "board.h"
#include "arm_math.h"

extern arm_rfft_instance_q15 fft_instance;

void microphone_init(void);

// Read raw ADC samples into the provided buffer via the blocking FIFO read
void microphone_read(uint16_t *buffer, size_t num_samples);

// Process raw ADC samples into a DC-removed, Q15 fixed-point buffer
void microphone_process(const uint16_t *raw, int16_t *out, size_t num_samples);

// Apply hanning window to a Q15 time-domain buffer in-place
void microphone_apply_window(int16_t *samples, size_t num_samples);

// Perform FFT on a Q15 time-domain buffer
void microphone_fft(int16_t *samples, int16_t *fft_output);

// Compute the squared magnitude of the FFT output
void microphone_magnitude_squared(int16_t *fft_output, q15_t *mag_output);


