#include "FlexMuxManager.h"

#include "config.h"

void FlexMuxManager::begin(uint8_t adcPin, uint8_t s0Pin, uint8_t s1Pin,
                            uint8_t s2Pin, uint8_t s3Pin, uint8_t enAPin,
                            uint8_t enBPin) {
  adcPin_ = adcPin;
  s0_ = s0Pin;
  s1_ = s1Pin;
  s2_ = s2Pin;
  s3_ = s3Pin;
  enA_ = enAPin;
  enB_ = enBPin;

  pinMode(s0_, OUTPUT);
  pinMode(s1_, OUTPUT);
  pinMode(s2_, OUTPUT);
  pinMode(s3_, OUTPUT);
  pinMode(enA_, OUTPUT);
  pinMode(enB_, OUTPUT);
  digitalWrite(enA_, HIGH); // both muxes disabled until a channel is selected
  digitalWrite(enB_, HIGH);

  // Uncalibrated defaults (full ADC range) until 'calibrate()' is run.
  for (uint8_t i = 0; i < kChannelCount; i++) {
    calMin_[i] = 0;
    calMax_[i] = 4095;
  }
}

void FlexMuxManager::selectChannel(uint8_t globalIndex) {
  uint8_t localIndex = globalIndex % kChannelsPerMux;
  digitalWrite(s0_, (localIndex >> 0) & 0x01);
  digitalWrite(s1_, (localIndex >> 1) & 0x01);
  digitalWrite(s2_, (localIndex >> 2) & 0x01);
  digitalWrite(s3_, (localIndex >> 3) & 0x01);

  bool useMuxA = globalIndex < kChannelsPerMux;
  digitalWrite(enA_, useMuxA ? LOW : HIGH);
  digitalWrite(enB_, useMuxA ? HIGH : LOW);
}

int FlexMuxManager::readChannel(uint8_t globalIndex) {
  selectChannel(globalIndex);
  delayMicroseconds(MUX_SETTLE_US);
  return analogRead(adcPin_);
}

void FlexMuxManager::scan() {
  foldBitmask_ = 0;
  for (uint8_t i = 0; i < kChannelCount; i++) {
    raw_[i] = readChannel(i);
    int range = calMax_[i] - calMin_[i];
    float normalized =
        range > 0 ? (float)(raw_[i] - calMin_[i]) / range : 0.0f;
    normalized = constrain(normalized, 0.0f, 1.0f);
    if (normalized > FOLD_THRESHOLD) {
      foldBitmask_ |= (1UL << i);
    }
  }
}

void FlexMuxManager::calibrate(uint32_t durationMs) {
  for (uint8_t i = 0; i < kChannelCount; i++) {
    calMin_[i] = 4095;
    calMax_[i] = 0;
  }

  Serial.println("[Calibration] Fold/unfold every hinge through its full range...");
  uint32_t start = millis();
  while (millis() - start < durationMs) {
    for (uint8_t i = 0; i < kChannelCount; i++) {
      int v = readChannel(i);
      calMin_[i] = min(calMin_[i], v);
      calMax_[i] = max(calMax_[i], v);
    }
  }

  Serial.println("[Calibration] Done. Per-hinge range:");
  for (uint8_t i = 0; i < kChannelCount; i++) {
    char line[64];
    snprintf(line, sizeof(line), "  hinge %2u: min=%4d max=%4d range=%4d", i,
             calMin_[i], calMax_[i], calMax_[i] - calMin_[i]);
    Serial.println(line);
  }
}
