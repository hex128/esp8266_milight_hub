#pragma once

#include <V2PacketFormatter.h>

#define FUT089_COLOR_OFFSET 0

enum MiLightFUT089Command {
  FUT089_ON = 0x01,
  FUT089_OFF = 0x01,
  FUT089_COLOR = 0x02,
  FUT089_BRIGHTNESS = 0x05,
  FUT089_MODE = 0x06,
  FUT089_KELVIN = 0x07,     // Controls Kelvin when in White mode
  FUT089_SATURATION = 0x07  // Controls Saturation when in Color mode
};

enum MiLightFUT089Arguments {
  FUT089_MODE_SPEED_UP   = 0x12,
  FUT089_MODE_SPEED_DOWN = 0x13,
  FUT089_WHITE_MODE = 0x14
};

class FUT089PacketFormatter final : public V2PacketFormatter {
public:
  FUT089PacketFormatter()
    : V2PacketFormatter(REMOTE_TYPE_FUT089, 0x25, 8) // protocol is 0x25, and there are 8 groups
  { }

  void updateBrightness(uint8_t value) override;
  void updateHue(uint16_t value) override;
  void updateColorRaw(uint8_t value) override;
  void updateColorWhite() override;
  void updateTemperature(uint8_t value) override;
  void updateSaturation(uint8_t value) override;
  void enableNightMode() override;

  void modeSpeedDown() override;
  void modeSpeedUp() override;
  void updateMode(uint8_t mode) override;

  BulbId parsePacket(const uint8_t* packet, JsonObject result) override;
};
