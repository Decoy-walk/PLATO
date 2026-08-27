#pragma once

#include <Arduino.h>

// Reads the NJL5513R reflectance PPG sensor (ambient-cancelled against the
// BPW34 reference photodiode) and the FSR402 grip/press sensor, and derives
// simple hand/structure interaction features from them.
class BioSensorManager {
public:
  void begin(uint8_t fsrPin, uint8_t ppgPin, uint8_t ppgLedPin,
             uint8_t ambientPin);

  // Call every loop(); internally rate-limited to SAMPLE_INTERVAL_MS so the
  // PPG beat detector sees a steady sample rate.
  void sample();

  bool gripDetected() const { return gripDetected_; }
  int bpm() const { return bpm_; }
  int fsrRaw() const { return fsrRaw_; }
  int ambientRaw() const { return ambientRaw_; }

  String toJson() const;

private:
  uint8_t fsrPin_ = 0;
  uint8_t ppgPin_ = 0;
  uint8_t ppgLedPin_ = 0;
  uint8_t ambientPin_ = 0;

  uint32_t lastSampleMs_ = 0;

  // PPG beat detection state
  float ppgBaseline_ = 0.0f;
  float ambientBaseline_ = 0.0f;
  float ppgEnvelope_ = 0.0f;
  bool ppgAboveThreshold_ = false;
  uint32_t lastBeatMs_ = 0;
  int bpm_ = 0;

  int fsrRaw_ = 0;
  int ambientRaw_ = 0;
  bool gripDetected_ = false;
};
