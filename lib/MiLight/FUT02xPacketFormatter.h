#pragma once

#include <PacketFormatter.h>

class FUT02xPacketFormatter : public PacketFormatter {
public:
  static constexpr uint8_t FUT02X_COMMAND_INDEX = 4;
  static constexpr uint8_t FUT02X_ARGUMENT_INDEX = 3;
  static constexpr uint8_t NUM_BRIGHTNESS_INTERVALS = 10;

  explicit FUT02xPacketFormatter(const MiLightRemoteType type)
    : PacketFormatter(type, 6, 10)
  { }

  bool canHandle(const uint8_t* packet, size_t len) override;

  void command(uint8_t command, uint8_t arg) override;

  void pair() override;
  void unpair() override;

  void initializePacket(uint8_t* packet) override;
  void format(uint8_t const* packet, char* buffer) override;
};
