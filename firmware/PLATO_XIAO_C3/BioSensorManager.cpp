#include "BioSensorManager.h"

#include "config.h"

void BioSensorManager::begin(uint8_t fsrPin, uint8_t ppgPin, uint8_t ppgLedPin,
                              uint8_t ambientPin) {
  fsrPin_ = fsrPin;
  ppgPin_ = ppgPin;
  ppgLedPin_ = ppgLedPin;
  ambientPin_ = ambientPin;

  pinMode(ppgLedPin_, OUTPUT);
  digitalWrite(ppgLedPin_, HIGH); // reflectance LED on while the node is powered

  // Seed the running baselines so they don't have to slew up from zero.
  ppgBaseline_ = analogRead(ppgPin_);
  ambientBaseline_ = analogRead(ambientPin_);
}

void BioSensorManager::sample() {
  uint32_t now = millis();
  if (now - lastSampleMs_ < SAMPLE_INTERVAL_MS) return;
  lastSampleMs_ = now;

  fsrRaw_ = analogRead(fsrPin_);
  gripDetected_ = fsrRaw_ > FSR_GRIP_THRESHOLD;

  int ppgRaw = analogRead(ppgPin_);
  ambientRaw_ = analogRead(ambientPin_);

  ppgBaseline_ += PPG_BASELINE_ALPHA * (ppgRaw - ppgBaseline_);
  ambientBaseline_ += PPG_BASELINE_ALPHA * (ambientRaw_ - ambientBaseline_);

  // Ambient-cancelled AC pulse component: the NJL5513R sees reflectance-LED
  // light plus ambient light, the BPW34 sees ambient only, so subtracting
  // the two AC swings rejects ambient flicker (e.g. indoor lighting) that
  // would otherwise be mistaken for a pulse.
  float ppgAc = (ppgRaw - ppgBaseline_) -
                PPG_AMBIENT_CANCEL_GAIN * (ambientRaw_ - ambientBaseline_);
  ppgEnvelope_ += PPG_ENVELOPE_ALPHA * (ppgAc - ppgEnvelope_);

  bool above = ppgEnvelope_ > PPG_BEAT_THRESHOLD;
  if (above && !ppgAboveThreshold_ && now - lastBeatMs_ > PPG_MIN_BEAT_MS) {
    if (lastBeatMs_ != 0) {
      bpm_ = 60000 / (now - lastBeatMs_);
    }
    lastBeatMs_ = now;
  }
  ppgAboveThreshold_ = above;
}

String BioSensorManager::toJson() const {
  String json = "{";
  json += "\"fsr\":" + String(fsrRaw_) + ",";
  json += "\"grip\":" + String(gripDetected_ ? "true" : "false") + ",";
  json += "\"ambient\":" + String(ambientRaw_) + ",";
  json += "\"bpm\":" + String(bpm_);
  json += "}";
  return json;
}
