#include "SerialTransport.h"

#include "config.h"

void SerialTransport::begin(uint32_t baud) { Serial.begin(baud); }

void SerialTransport::send(const uint8_t *data, size_t len) {
  // Two marker bytes before each frame let the PC-side reader stay
  // byte-aligned on this continuous stream even if it connects mid-frame,
  // which a discrete BLE notification never had to worry about.
  Serial.write(SERIAL_SYNC_BYTE_1);
  Serial.write(SERIAL_SYNC_BYTE_2);
  Serial.write(data, len);
}
