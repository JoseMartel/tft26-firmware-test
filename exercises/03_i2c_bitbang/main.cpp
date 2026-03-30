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
//    5. Scan the I2C bus (addresses 0x08–0x77) and print every responding
//    address.
//    6. For each unknown device found, read its WHO_AM_I and print it.
//    7. Add the humidity sensor to the 1 Hz loop: read HUM_RAW and print %RH.
//
//  Read README.md before starting.
// =============================================================================

/*
================================================================
APPROACH & DESIGN OVERVIEW
================================================================

Problem:
Implement I2C communication via software (bit-banging) to:
 - Detect devices on the bus
 - Read sensor data (temperature & humidity)
 - Process and output values

Approach:
  The solution is structured in layers

   1. Low-Level (GPIO control SDA/SCL)

   2. I2C Basics Sequences (Bit-banging)
      - Implements Start/Stop conditions
      - Byte write/read + ACK handling

   3. I2C HAL
      - High-level register access read/write

   4. Application Layer
      - Bus scan to discover devices
      - Periodic sensor reading
      - Data conversion and output

Flow:
   - Initialize pull-ups for SDA/SCL
   - Validate known device (TMP64 via WHO_AM_I)
   - Scan I2C bus to detect additional devices
   - Loop:
        -> Read temperature
        -> Read humidity (if detected)
        -> Format and display results

================================================================
*/

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <trac_fw_io.hpp>

// ================= CONFIG =================
#define SCL_PORT 8
#define SDA_PORT 9

#define I2C_DELAY_MS 1

#define TMP64_ADDR 0x48
#define REG_WHO_AM_I 0x0F
#define REG_DATA 0x00

trac_fw_io_t io;

// ================= LOW LEVEL =================

void scl_high() { io.digital_write(SCL_PORT, 1); }
void scl_low() { io.digital_write(SCL_PORT, 0); }

void sda_high() { io.digital_write(SDA_PORT, 1); }
void sda_low() { io.digital_write(SDA_PORT, 0); }

bool sda_read() { return io.digital_read(SDA_PORT); }

void i2c_delay() { io.delay(I2C_DELAY_MS); }

// ================= I2C BASICS =================

void i2c_start() {
  sda_high();
  scl_high();
  i2c_delay();
  sda_low();
  i2c_delay();
  scl_low();
}

void i2c_stop() {
  sda_low();
  scl_high();
  i2c_delay();
  sda_high();
  i2c_delay();
}

bool i2c_write_byte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    if (data & 0x80)
      sda_high();
    else
      sda_low();

    scl_high();
    i2c_delay();
    scl_low();
    i2c_delay();

    data <<= 1;
  }

  // ACK bit
  sda_high(); // liberar SDA
  scl_high();
  i2c_delay();

  bool ack = !sda_read(); // ACK = 0

  scl_low();
  return ack;
}

uint8_t i2c_read_byte(bool ack) {
  uint8_t data = 0;

  sda_high(); // liberar línea

  for (int i = 0; i < 8; i++) {
    data <<= 1;

    scl_high();
    i2c_delay();

    if (sda_read())
      data |= 1;

    scl_low();
    i2c_delay();
  }

  // enviar ACK/NACK
  if (ack)
    sda_low();
  else
    sda_high();

  scl_high();
  i2c_delay();
  scl_low();
  sda_high();

  return data;
}

// ================= I2C HAL =================

bool i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, int len) {
  i2c_start(); // Start condition

  // Send address byte + write bit
  if (!i2c_write_byte((addr << 1) | 0)) {
    i2c_stop();
    return false;
  }

  // Send register address
  if (!i2c_write_byte(reg)) {
    i2c_stop();
    return false;
  }

  i2c_start(); // Repeated Start

  // Send address byte + read bit
  if (!i2c_write_byte((addr << 1) | 1)) {
    i2c_stop();
    return false;
  }

  // Read data bytes
  for (int i = 0; i < len; i++) {
    buf[i] = i2c_read_byte(i < (len - 1));
  }

  i2c_stop();
  return true;
}

// ================= BUS SCAN =================

void scan_bus(uint8_t *hum_addr) {
  printf("Scanning I2C bus...\n");

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    i2c_start();

    bool ack = i2c_write_byte((addr << 1) | 0);
    i2c_stop();

    if (ack) {
      printf("Found device at 0x%02X\n", addr);

      uint8_t buffer = 0;
      if (i2c_read_reg(addr, REG_WHO_AM_I, &buffer, 1)) {
        printf("  WHO_AM_I = 0x%02X\n", buffer);

        if (addr != TMP64_ADDR) {
          *hum_addr = addr;
        }
      }
    }
  }
}

// ================= MAIN =================

int main() {

  // Set Pull Up for SDA & SCL Pins
  io.set_pullup(SCL_PORT, true);
  io.set_pullup(SDA_PORT, true);

  // Check TMP64 WHO_AM_I
  uint8_t buffer = 0;
  if (i2c_read_reg(TMP64_ADDR, REG_WHO_AM_I, &buffer, 1)) {
    printf("TMP64 WHO_AM_I = 0x%02X\n", buffer);
  }

  // Scan I2C bus for unknown device
  uint8_t hum_addr = 0;
  scan_bus(&hum_addr);

  // ---- LOOP ----
  uint32_t last = 0;

  while (true) {
    uint32_t now = io.millis();

    // Check each second
    if (now - last >= 1000) {
      last = now;

      // Read Temperature
      uint8_t buf[4];

      if (i2c_read_reg(TMP64_ADDR, REG_DATA, buf, 4)) {
        // Convert to float
        int32_t raw = ((int32_t)buf[0] << 24) | ((int32_t)buf[1] << 16) |
                      ((int32_t)buf[2] << 8) | ((int32_t)buf[3]);
        float temp = raw / 1000.0f;

        printf("Temp: %.3f C\n", temp);

        // Show in LCD
        char buf[9] = {};
        std::snprintf(buf, sizeof(buf), "%8.3f", temp);

        uint32_t r6, r7;
        std::memcpy(&r6, buf + 0, 4);
        std::memcpy(&r7, buf + 4, 4);

        io.write_reg(6, r6);
        io.write_reg(7, r7);
      }

      // Read Humidity
      if (hum_addr != 0) {
        if (i2c_read_reg(hum_addr, REG_DATA, buf, 4)) {
          int32_t raw = ((int32_t)buf[0] << 24) | ((int32_t)buf[1] << 16) |
                        ((int32_t)buf[2] << 8) | ((int32_t)buf[3]);
          float hum = raw / 1000.0f;

          printf("Hum: %.3f %%\n", hum);

          // Show in LCD
          char buf[9] = {};
          std::snprintf(buf, sizeof(buf), "%7.3f%%", hum);

          uint32_t r4, r5;
          std::memcpy(&r4, buf + 0, 4);
          std::memcpy(&r5, buf + 4, 4);

          io.write_reg(4, r4);
          io.write_reg(5, r5);
        }
      }
    }

    io.delay(1);
  }
}