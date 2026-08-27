#pragma once

#include <Arduino.h>

// Streams the fold-state frame over the native USB serial port instead of
// BLE (SAMD21 has no on-board radio). This is an interim transport pending
// a decision on adding a BLE UART module for wireless logging - only this
// file would need to change if/when that happens, since PLATO_XIAO_SAMD21.ino
// only calls send(), not anything BLE- or Serial-specific.
class SerialTransport {
public:
  void begin(uint32_t baud);
  void send(const uint8_t *data, size_t len);
};
