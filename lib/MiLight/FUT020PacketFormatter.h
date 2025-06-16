#pragma once

#include <FUT02xPacketFormatter.h>

enum class FUT020Command {
  ON_OFF             = 0x04,
  MODE_SWITCH        = 0x02,
  COLOR_WHITE_TOGGLE = 0x05,
  BRIGHTNESS_DOWN    = 0x01,
  BRIGHTNESS_UP      = 0x03,
  COLOR              = 0x00
};

class FUT020PacketFormatter final : public FUT02xPacketFormatter {
public:
  FUT020PacketFormatter()
    : FUT02xPacketFormatter(REMOTE_TYPE_FUT020)
  { }

  void updateStatus(MiLightStatus status, uint8_t groupId) override;
  void updateHue(uint16_t value) override;
  void updateColorRaw(uint8_t value) override;
  void updateColorWhite() override;
  void nextMode() override;
  void updateBrightness(uint8_t value) override;
  void increaseBrightness() override;
  void decreaseBrightness() override;

  BulbId parsePacket(const uint8_t* packet, JsonObject result) override;
};
