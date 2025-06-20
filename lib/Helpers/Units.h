#pragma once

#include <Arduino.h>
#include <cinttypes>

// MiLight CCT bulbs range from 2700K-6500K, or ~370.3-153.8 mireds.
#define COLOR_TEMP_MAX_MIREDS 370
#define COLOR_TEMP_MIN_MIREDS 153

class Units {
public:
  template <typename T, typename V>
  static T rescale(T value, V newMax, float oldMax = 255.0) {
    return round(value * (newMax / oldMax));
  }

  static uint8_t miredsToWhiteVal(uint16_t mireds, const uint8_t maxValue = 255) {
      return rescale<uint16_t, uint16_t>(
        constrain(mireds, COLOR_TEMP_MIN_MIREDS, COLOR_TEMP_MAX_MIREDS) - COLOR_TEMP_MIN_MIREDS,
        maxValue,
        (COLOR_TEMP_MAX_MIREDS - COLOR_TEMP_MIN_MIREDS)
      );
  }

  static uint16_t whiteValToMireds(const uint8_t value, const uint8_t maxValue = 255) {
    const auto scaled = rescale<uint16_t, uint16_t>(value, (COLOR_TEMP_MAX_MIREDS - COLOR_TEMP_MIN_MIREDS), maxValue);
    return COLOR_TEMP_MIN_MIREDS + scaled;
  }
};
