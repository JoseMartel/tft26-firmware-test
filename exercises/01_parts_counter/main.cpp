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
/*

================================================================
APPROACH & DESIGN OVERVIEW
================================================================

After analyzing the problem and signal behavior, an interrupt-driven
approach was selected to ensure accurate pulse detection with minimal
CPU usage. A minimum pulse width filter was applied to improve robustness
by rejecting noise.

Approach:
The solution is event-driven using interrupts and pulse validation:

  1. Interrupt-based Detection
     - GPIO interrupt on CHANGE (rising & falling edges)
     - Avoids polling and reduces CPU usage

  2. Pulse Tracking
     - Detect rising edge → start timing
     - Detect falling edge → compute pulse duration

  3. Pulse Validation
     - Minimum pulse width threshold (MIN_PULSE_MS)
     - Filters out noise and spurious transitions

  4. Output Layer
     - Periodic read of pulse count
     - Formatted and written to output registers

Flow:
 - Interrupt triggers on signal change
 - On rising edge -> store timestamp
 - On falling edge -> compute pulse duration
 - Validate pulse width and update counter
 - Main loop reads counter and publishes result

================================================================
*/

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <trac_fw_io.hpp>

constexpr uint32_t MIN_PULSE_MS = 30;

static trac_fw_io_t *_io = nullptr;
static std::atomic<uint32_t> g_count{0};

void pulse_isr() {
  static uint32_t rising_time = 0;
  static bool is_pulse_active = false;

  uint32_t now = _io->millis();
  bool level = _io->digital_read(0);

  if (level && !is_pulse_active) {
    rising_time = now;
    is_pulse_active = true;
  } else if (!level && is_pulse_active) {
    uint32_t duration = now - rising_time;

    if (duration >= MIN_PULSE_MS) {
      g_count.fetch_add(1, std::memory_order_relaxed);
    }

    is_pulse_active = false;
  }
}

int main() {
  trac_fw_io_t io;

  _io = &io;
  io.attach_interrupt(0, pulse_isr, InterruptMode::CHANGE);

  while (true) {
    uint32_t current_count = g_count.load(std::memory_order_relaxed);

    char buf[9] = {};
    std::snprintf(buf, sizeof(buf), "%8u", current_count);

    uint32_t registers[2];
    std::memcpy(registers, buf, 8);

    io.write_reg(6, registers[0]);
    io.write_reg(7, registers[1]);

    io.delay(1);
  }
}