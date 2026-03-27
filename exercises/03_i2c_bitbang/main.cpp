// =============================================================================
//  Exercise 03 — I2C Sensors (Bit-bang)
// =============================================================================
//
//  Virtual hardware:
//    P8 (SCL)  →  io.digital_write(8, …) / io.digital_read(8)
//    P9 (SDA)  →  io.digital_write(9, …) / io.digital_read(9)
//
//  PART 1 — TMP64 temperature sensor at I2C address 0x48
//    Register 0x0F  WHO_AM_I   — 1 byte  (expected: 0xA5)
//    Register 0x00  TEMP_RAW   — 4 bytes, big-endian int32_t, milli-Celsius
//
//  PART 2 — Unknown humidity sensor (same register layout, address unknown)
//    Register 0x0F  WHO_AM_I   — 1 byte
//    Register 0x00  HUM_RAW    — 4 bytes, big-endian int32_t, milli-percent
//
//  Goal (Part 1):
//    1. Implement an I2C master via bit-bang on P8/P9.
//    2. Read WHO_AM_I from TMP64 and confirm the sensor is present.
//    3. Read TEMP_RAW in a loop and print the temperature in °C every second.
//    4. Update display registers 6–7 with the formatted temperature string.
//
//  Goal (Part 2):
//    5. Scan the I2C bus (addresses 0x08–0x77) and print every responding address.
//    6. For each unknown device found, read its WHO_AM_I and print it.
//    7. Add the humidity sensor to the 1 Hz loop: read HUM_RAW and print %RH.
//
//  Read README.md before starting.
// =============================================================================

#include <trac_fw_io.hpp>
#include <cstdio>
#include <cstdint>

int main() {
    trac_fw_io_t io;

    // ── Part 1 ────────────────────────────────────────────────────────────────
    // TODO: Implement I2C start / stop / write_byte / read_byte helpers
    // TODO: Read WHO_AM_I from TMP64 (address 0x48, register 0x0F) — verify 0xA5
    // TODO: In a ~1 Hz loop, read the 4-byte temperature from register 0x00,
    //       convert to °C, printf it, and write to display registers 6–7.

    // ── Part 2 ────────────────────────────────────────────────────────────────
    // TODO: Scan addresses 0x08–0x77; print each address that ACKs.
    // TODO: For each unknown address found, read its WHO_AM_I (register 0x0F).
    // TODO: Add the humidity sensor to the loop: read register 0x00 (4 bytes),
    //       convert milli-percent → %RH, printf the result every second,
    //       and write to display registers 4–5 (same format as temperature).

    while (true) {
        io.delay(1000);
    }
}
