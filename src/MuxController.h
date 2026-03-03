#pragma once
/**
 * MuxController.h
 *
 * Centralized logic for switching channels on I2C mux
 */
#include <Arduino.h>
#include <Wire.h>

#include "Config.h"

class MuxController {
public:
  static void selectChannel(uint8_t channel) {
    if (channel > 7)
      return;
    Wire.beginTransmission(config::MUX_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();

    delay(config::CHANNEL_SWITCH_SETTLE_MS);
  }
  static void disableChannel() {
    Wire.beginTransmission(config::MUX_ADDR);
    Wire.write(0);
    Wire.endTransmission();

    delay(config::CHANNEL_SWITCH_SETTLE_MS);
  }
};
