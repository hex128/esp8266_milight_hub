#pragma once

#include <Arduino.h>
#include <string.h>

class LEDStatus {
  public:
    enum class LEDMode {
      Off,
      SlowToggle,
      FastToggle,
      SlowBlip,
      FastBlip,
      Flicker,
      On,
      Unknown
    };
    LEDStatus(uint8_t ledPin);
    void changePin(uint8_t ledPin);
    void continuous(LEDMode mode);
    void continuous(uint16_t ledOffMs, uint16_t ledOnMs);
    void oneshot(LEDMode mode, uint8_t count = 1);
    void oneshot(uint16_t ledOffMs, uint16_t ledOnMs, uint8_t count = 1);

    static String LEDModeToString(LEDMode mode);
    static LEDMode stringToLEDMode(const String &mode);

    void handle();

  private:
    static void _modeToTime(LEDMode mode, uint16_t& ledOffMs, uint16_t& ledOnMs);
    uint8_t _pinState(uint8_t val) const;
    uint8_t _ledPin;
    bool _inverse;

    uint16_t _continuousOffMs = 1000;
    uint16_t _continuousOnMs = 0;
    bool _continuousCurrentlyOn = false;

    uint16_t _oneshotOffMs{};
    uint16_t _oneshotOnMs{};
    uint8_t _oneshotCountRemaining = 0;
    bool _oneshotCurrentlyOn = false;

    unsigned long _timer = 0;
};
