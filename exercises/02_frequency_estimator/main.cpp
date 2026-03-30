// =============================================================================
//  Challenge 02 — Frequency Estimator
// =============================================================================
//
//  Virtual hardware:
//    ADC Ch 0  →  io.analog_read(0)      Process sensor signal (0–4095)
//    OUT reg 3 →  io.write_reg(3, …)     Frequency estimate in centiHz
//                                        e.g. write_reg(3, 4733) = 47.33 Hz
//
//  Goal:
//    Measure the frequency of the signal on ADC channel 0 and publish your
//    estimate continuously via register 3.
//
//  Read README.md before starting.
// =============================================================================

/*
================================================================
APPROACH & DESIGN OVERVIEW
================================================================

After analyzing the problem and the signal behavior in the simulator,
a solution focused on simplicity and robustness was chosen, ensuring
full functionality while minimizing potential failures.

Approach:
The solution is structured as a signal-processing pipeline:

  1. Signal Conditioning
     - EMA filter applied to raw ADC samples to reduce noise

  2. Rising Edge Detection (Zero-Crossing with Hysteresis)
     - Dual thresholds to avoid false triggers due to noise
     - Detect rising crossings only

  3. Period Measurement
     - Time between consecutive crossings using system time
     - Validation using min/max period constraints considering

  4. Noise Reduction
     - Median filter (small window) to reject outliers
     - EMA smoothing for stable frequency estimation

  5. Output Layer
     - Frequency converted to centiHz
     - Updated only on value change to reduce writes

Flow:
 - Read ADC sample
 - Apply EMA filtering
 - Detect threshold crossing with hysteresis
 - Compute period between crossings
 - Validate and convert to frequency
 - Apply median + EMA smoothing
 - Publish result via register

Improvements (real hardware):
On real hardware, this approach could be enhanced by leveraging
peripherals such as timer-triggered ADC sampling, DMA, or comparators,
improving timing accuracy, reducing CPU usage, and ensuring more
deterministic behavior.
================================================================
*/

#include <cstdint>
#include <trac_fw_io.hpp>

// EMA ADC
#define ADC_ALPHA 0.1f

// Hysteresis
#define THRESHOLD_UPPER (2048 + 512)
#define THRESHOLD_LOWER (2048 - 512)

// Median
#define MEDIAN_WINDOW 3 // Samples

// EMA Output
#define EMA_ALPHA 0.2f

// Period validation (ms)
#define MIN_PERIOD_MS 5
#define MAX_PERIOD_MS 1000

// ==========================================

int main() {
  trac_fw_io_t io;

  // EMA
  float adc_filtered = 0.0f;
  bool ema_initialized = false;

  // Hysteresis state
  bool state_high = false;

  // Timestamps
  uint32_t last_cross_time = 0;

  // Buffer for median
  float freq_buffer[MEDIAN_WINDOW] = {0};
  int buf_index = 0;
  int buf_count = 0;

  // Final estimation
  float freq_est = 0.0f;

  uint32_t reported_freq_cHz = 0xFFFFFFFF;

  while (true) {
    uint32_t now = io.millis();

    uint16_t adc_raw = io.analog_read(0);

    // EMA Filter over input signal
    if (!ema_initialized) {
      adc_filtered = adc_raw;
      ema_initialized = true;
    } else {
      adc_filtered = ADC_ALPHA * adc_raw + (1.0f - ADC_ALPHA) * adc_filtered;
    }

    // Zero-crossing with hysteresis (using filtered signal)
    bool crossing = false;

    if (!state_high && adc_filtered > THRESHOLD_UPPER) {
      state_high = true;
      crossing = true;
    } else if (state_high && adc_filtered < THRESHOLD_LOWER) {
      state_high = false;
    }

    // Period estimation
    if (crossing) {
      if (last_cross_time != 0) {
        uint32_t period = now - last_cross_time;

        // Physical validation
        if (period >= MIN_PERIOD_MS && period <= MAX_PERIOD_MS) {

          float freq = 1000.0f / period; // Hz

          // Median filter
          freq_buffer[buf_index] = freq;
          buf_index = (buf_index + 1) % MEDIAN_WINDOW;
          if (buf_count < MEDIAN_WINDOW)
            buf_count++;

          float freq_med = freq;

          if (buf_count == MEDIAN_WINDOW) {
            // copy and sort (N small -> simple)
            float temp[MEDIAN_WINDOW];
            for (int i = 0; i < MEDIAN_WINDOW; i++)
              temp[i] = freq_buffer[i];

            // bubble sort (enough for N small)
            for (int i = 0; i < MEDIAN_WINDOW - 1; i++) {
              for (int j = i + 1; j < MEDIAN_WINDOW; j++) {
                if (temp[j] < temp[i]) {
                  float t = temp[i];
                  temp[i] = temp[j];
                  temp[j] = t;
                }
              }
            }

            freq_med = temp[MEDIAN_WINDOW / 2];
          }

          // EMA Output Filter
          if (freq_est == 0.0f)
            freq_est = freq_med;
          else
            freq_est = EMA_ALPHA * freq_med + (1.0f - EMA_ALPHA) * freq_est;
        }
      }

      last_cross_time = now;
    }

    // Output
    uint32_t freq_cHz = (uint32_t)(freq_est * 100.0f);

    if (freq_cHz != reported_freq_cHz) {
      io.write_reg(3, freq_cHz);
      reported_freq_cHz = freq_cHz;
    }

    io.delay(1);
  }
}
