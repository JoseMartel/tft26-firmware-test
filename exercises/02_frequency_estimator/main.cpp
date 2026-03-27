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

#include <trac_fw_io.hpp>
#include <cstdint>

int main() {
    trac_fw_io_t io;

    while (true) {
        // TODO: sample ADC channel 0 at a fixed rate using io.millis()
        // TODO: estimate the signal frequency
        // TODO: publish the result with io.write_reg(3, freq_cHz)
    }
}
