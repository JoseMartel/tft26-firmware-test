// =============================================================================
//  Exercise 01 — Parts Counter
// =============================================================================
//
//  Virtual hardware:
//    SW 0        →  io.digital_read(0)        Inductive sensor input
//    Display     →  io.write_reg(6, …)        LCD debug (see README for format)
//                   io.write_reg(7, …)
//
//  Goal:
//    Count every part that passes the sensor and show the total on the display.
//
//  Read README.md before starting.
// =============================================================================

#include <trac_fw_io.hpp>
#include <cstdint>

int main() {
    trac_fw_io_t io;

    uint32_t count = 0;

    while (true) {
        // TODO: detect each part passing the sensor and increment count
        // TODO: update the display with the current count
        (void)count;
    }
}
