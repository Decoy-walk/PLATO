#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Scans 20 flex sensors (folding-block hinges) through two CD74HC4067
// analog muxes sharing a single ADC pin, applies a per-hinge calibrated
// threshold, and reports which hinges currently read as "folded".
class FlexMuxManager {
public:
  static constexpr uint8_t kChannelCount = 20;
  static constexpr uint8_t kChannelsPerMux = 16;

  void begin(uint8_t adcPin, uint8_t s0Pin, uint8_t s1Pin, uint8_t s2Pin,
             uint8_t s3Pin, uint8_t enAPin, uint8_t enBPin);

  // Scans all 20 channels once; updates raw()/foldBitmask().
  void scan();

  uint32_t foldBitmask() const { return foldBitmask_; }
  int raw(uint8_t channel) const { return raw_[channel]; }

  // Blocking: tracks per-channel min/max for durationMs while the operator
  // folds/unfolds every hinge through its full range, then saves the
  // result to NVS so it survives a reboot.
  void calibrate(uint32_t durationMs);

private:
  void selectChannel(uint8_t globalIndex);
  int readChannel(uint8_t globalIndex);
  void loadCalibration();
  void saveCalibration();

  uint8_t adcPin_ = 0;
  uint8_t s0_ = 0, s1_ = 0, s2_ = 0, s3_ = 0;
  uint8_t enA_ = 0, enB_ = 0;

  int raw_[kChannelCount] = {0};
  int calMin_[kChannelCount];
  int calMax_[kChannelCount];
  uint32_t foldBitmask_ = 0;

  Preferences prefs_;
};
