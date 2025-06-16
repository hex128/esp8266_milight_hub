#pragma once

#include <PacketFormatter.h>

#define RGB_COMMAND_INDEX 4
#define RGB_COLOR_INDEX 3
#define RGB_INTERVALS 10

enum MiLightRgbButton {
  RGB_OFF             = 0x01,
  RGB_ON              = 0x02,
  RGB_BRIGHTNESS_UP   = 0x03,
  RGB_BRIGHTNESS_DOWN = 0x04,
  RGB_SPEED_UP        = 0x05,
  RGB_SPEED_DOWN      = 0x06,
  RGB_MODE_UP         = 0x07,
  RGB_MODE_DOWN       = 0x08,
  RGB_PAIR            = RGB_SPEED_UP
};

class RgbPacketFormatter : public PacketFormatter {
public:
  RgbPacketFormatter()
    : PacketFormatter(REMOTE_TYPE_RGB, 6, 20)
  { }

  void updateStatus(MiLightStatus status, uint8_t groupId) override;
  void updateBrightness(uint8_t value) override;
  void increaseBrightness() override;
  void decreaseBrightness() override;
  void command(uint8_t command, uint8_t arg) override;
  void updateHue(uint16_t value) override;
  void updateColorRaw(uint8_t value) override;
  void format(uint8_t const* packet, char* buffer) override;
  void pair() override;
  void unpair() override;
  void modeSpeedDown() override;
  void modeSpeedUp() override;
  void nextMode() override;
  void previousMode() override;
  BulbId parsePacket(const uint8_t* packet, JsonObject result) override;

  void initializePacket(uint8_t* packet) override;
};
