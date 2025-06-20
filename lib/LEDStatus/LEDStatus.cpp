#include <LEDStatus.h>

// constructor defines which pin the LED is attached to
LEDStatus::LEDStatus(uint8_t ledPin) {
  // if pin negative, reverse and set inverse on pin outputs
  if (ledPin < 0) {
    ledPin = -ledPin;
    _inverse = true;
  } else {
    _inverse = false;
  }
  // set up the pin
  _ledPin = ledPin;
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, _pinState(LOW));
  _timer = millis();
}

// change pin at runtime
void LEDStatus::changePin(uint8_t ledPin) {
  bool inverse;
  // if pin negative, reverse and set inverse on pin outputs
  if (ledPin < 0) {
    ledPin = -ledPin;
    inverse = true;
  } else {
    inverse = false;
  }

  if ((ledPin != _ledPin) || (inverse != _inverse)) {
    // make sure old pin is off
    digitalWrite(_ledPin, _pinState(LOW));
    _ledPin = ledPin;
    _inverse = inverse;
    // and make sure new pin is also off
    pinMode(_ledPin, OUTPUT);
    digitalWrite(_ledPin, _pinState(LOW));
  }
}


// identify how to flash the LED by mode, continuously until changed
void LEDStatus::continuous(const LEDMode mode) {
  uint16_t ledOffMs, ledOnMs;
  _modeToTime(mode, ledOffMs, ledOnMs);
  continuous(ledOffMs, ledOnMs);
}

// identify how to flash the LED by on/off times (in ms), continuously until changed
void LEDStatus::continuous(const uint16_t ledOffMs, const uint16_t ledOnMs) {
  _continuousOffMs = ledOffMs;
  _continuousOnMs = ledOnMs;
  _continuousCurrentlyOn = false;
  // reset LED to off
  if (_ledPin > 0) {
    digitalWrite(_ledPin, _pinState(LOW));
  }
  // restart timer
  _timer = millis();
}

// identify a one-shot LED action (overrides continuous until done) by mode
void LEDStatus::oneshot(const LEDMode mode, const uint8_t count) {
  uint16_t ledOffMs, ledOnMs;
  _modeToTime(mode, ledOffMs, ledOnMs);
  oneshot(ledOffMs, ledOnMs, count);
}

// identify a one-shot LED action (overrides continuous until done) by times (in ms)
void LEDStatus::oneshot(const uint16_t ledOffMs, const uint16_t ledOnMs, const uint8_t count) {
  _oneshotOffMs = ledOffMs;
  _oneshotOnMs = ledOnMs;
  _oneshotCountRemaining = count;
  _oneshotCurrentlyOn = false;
  // reset LED to off
  if (_ledPin > 0) {
    digitalWrite(_ledPin, _pinState(LOW));
  }
  // restart timer
  _timer = millis();
}

// call this function in your loop - it will return quickly after calculating if any changes need to
// be made to the pin to flash the LED
void LEDStatus::handle() {
  // is a pin defined?
  if (_ledPin == 0) {
    return;
  }

  // are we currently running a one-shot?
  if (_oneshotCountRemaining > 0) {
      if (_oneshotCurrentlyOn) {
          if ((_timer + _oneshotOnMs) < millis()) {
              if (_oneshotOffMs > 0) {
                  digitalWrite(_ledPin, _pinState(LOW));
              }
              _oneshotCurrentlyOn = false;
              --_oneshotCountRemaining;
              if (_oneshotCountRemaining == 0) {
                  _continuousCurrentlyOn = false;
              }
              _timer += _oneshotOnMs;
          }
      } else {
          if ((_timer + _oneshotOffMs) < millis()) {
            if (_oneshotOnMs > 0) {
                digitalWrite(_ledPin, _pinState(HIGH));
            }
            _oneshotCurrentlyOn = true;
            _timer += _oneshotOffMs;
          }
      }
  } else {
    // operate using continuous
    if (_continuousCurrentlyOn) {
      if ((_timer + _continuousOnMs) < millis()) {
        if (_continuousOffMs > 0) {
          digitalWrite(_ledPin, _pinState(LOW));
        }
        _continuousCurrentlyOn = false;
        _timer += _continuousOnMs;
      }
    } else {
      if ((_timer + _continuousOffMs) < millis()) {
        if (_continuousOnMs > 0) {
          digitalWrite(_ledPin, _pinState(HIGH));
        }
        _continuousCurrentlyOn = true;
        _timer += _continuousOffMs;
      }
    }
  }
}

// helper function to convert an LEDMode enum to a string
String LEDStatus::LEDModeToString(const LEDMode mode) {
  switch (mode) {
    case LEDMode::Off:
      return "Off";
    case LEDMode::SlowToggle:
      return "Slow toggle";
    case LEDMode::FastToggle:
      return "Fast toggle";
    case LEDMode::SlowBlip:
      return "Slow blip";
    case LEDMode::FastBlip:
      return "Fast blip";
    case LEDMode::Flicker:
      return "Flicker";
    case LEDMode::On:
      return "On";
    default:
      return "Unknown";
  }
}

// helper function to convert a string to an LEDMode enum (note, mismatch returns LedMode::Unknown)
LEDStatus::LEDMode LEDStatus::stringToLEDMode(const String &mode) {
  if (mode == "Off")
    return LEDMode::Off;
  if (mode == "Slow toggle")
    return LEDMode::SlowToggle;
  if (mode == "Fast toggle")
    return LEDMode::FastToggle;
  if (mode == "Slow blip")
    return LEDMode::SlowBlip;
  if (mode == "Fast blip")
    return LEDMode::FastBlip;
  if (mode == "Flicker")
    return LEDMode::Flicker;
  if (mode == "On")
    return LEDMode::On;
  // unable to match...
  return LEDMode::Unknown;
}


// private helper converts mode to on/off times in ms
void LEDStatus::_modeToTime(const LEDMode mode, uint16_t& ledOffMs, uint16_t& ledOnMs) {
  switch (mode) {
    case LEDMode::Off:
      ledOffMs = 1000;
      ledOnMs = 0;
      break;
    case LEDMode::SlowToggle:
      ledOffMs = 1000;
      ledOnMs = 1000;
      break;
    case LEDMode::FastToggle:
      ledOffMs = 100;
      ledOnMs = 100;
      break;
    case LEDMode::SlowBlip:
      ledOffMs = 1500;
      ledOnMs = 50;
      break;
    case LEDMode::FastBlip:
      ledOffMs = 333;
      ledOnMs = 50;
      break;
    case LEDMode::On:
      ledOffMs = 0;
      ledOnMs = 1000;
      break;
    case LEDMode::Flicker:
      ledOffMs = 50;
      ledOnMs = 30;
      break;
    default:
      Serial.printf_P(PSTR("LEDStatus::_modeToTime: Uknown LED mode %d\n"), mode);
      ledOffMs = 500;
      ledOnMs = 2000;
      break;
  }
}

// private helper to optionally inverse the LED
uint8_t LEDStatus::_pinState(const uint8_t val) const {
  if (_inverse) {
    return (val == LOW) ? HIGH : LOW;
  }
  return val;
}
