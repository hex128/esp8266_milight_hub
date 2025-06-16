#pragma once

#include <PacketFormatter.h>

#define CCT_COMMAND_INDEX 4
#define CCT_INTERVALS 10

enum MiLightCctButton {
  CCT_ALL_ON            = 0x05,
  CCT_ALL_OFF           = 0x09,
  CCT_GROUP_1_ON        = 0x08,
  CCT_GROUP_1_OFF       = 0x0B,
  CCT_GROUP_2_ON        = 0x0D,
  CCT_GROUP_2_OFF       = 0x03,
  CCT_GROUP_3_ON        = 0x07,
  CCT_GROUP_3_OFF       = 0x0A,
  CCT_GROUP_4_ON        = 0x02,
  CCT_GROUP_4_OFF       = 0x06,
  CCT_BRIGHTNESS_DOWN   = 0x04,
  CCT_BRIGHTNESS_UP     = 0x0C,
  CCT_TEMPERATURE_UP    = 0x0E,
  CCT_TEMPERATURE_DOWN  = 0x0F
};

class CctPacketFormatter final : public PacketFormatter {
public:
  CctPacketFormatter()
    : PacketFormatter(REMOTE_TYPE_CCT, 7, 20)
  { }

  bool canHandle(const uint8_t* packet, size_t len) override;

  void updateStatus(MiLightStatus status, uint8_t groupId) override;
  void command(uint8_t command, uint8_t arg) override;

  void updateTemperature(uint8_t value) override;
  void increaseTemperature() override;
  void decreaseTemperature() override;

  void updateBrightness(uint8_t value) override;
  void increaseBrightness() override;
  void decreaseBrightness() override;
  void enableNightMode() override;

  void format(uint8_t const* packet, char* buffer) override;
  void initializePacket(uint8_t* packet) override;
  void finalizePacket(uint8_t* packet) override;
  BulbId parsePacket(const uint8_t* packet, JsonObject result) override;

  static uint8_t getCctStatusButton(uint8_t groupId, MiLightStatus status);
  static uint8_t cctCommandIdToGroup(uint8_t command);
  static MiLightStatus cctCommandToStatus(uint8_t command);
};
