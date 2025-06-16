#pragma once

#include <PacketFormatter.h>

#define V2_PACKET_LEN 9

#define V2_PROTOCOL_ID_INDEX 1
#define V2_COMMAND_INDEX 4
#define V2_ARGUMENT_INDEX 5

// Default number of values to allow before and after strictly defined range for V2 scales
#define V2_DEFAULT_RANGE_BUFFER 0x13

class V2PacketFormatter : public PacketFormatter {
public:
  V2PacketFormatter(MiLightRemoteType deviceType, uint8_t protocolId, uint8_t numGroups);

  bool canHandle(const uint8_t* packet, size_t packetLen) override;
  void initializePacket(uint8_t* packet) override;

  void updateStatus(MiLightStatus status, uint8_t group) override;
  void command(uint8_t command, uint8_t arg) override;
  void format(uint8_t const* packet, char* buffer) override;
  void unpair() override;

  void finalizePacket(uint8_t* packet) override;

  uint8_t groupCommandArg(MiLightStatus status, uint8_t groupId);

  /*
   * Some protocols have scales which have the following characteristics:
   *   Start at some value X, goes down to 0, then up to Y.
   * eg:
   *   0x8F, 0x8D, ..., 0, 0x2, ..., 0x20
   * This is a parameterized method to convert from [0, 100] TO this scale
   */
  static uint8_t tov2scale(uint8_t value, uint8_t endValue, uint8_t interval, bool reverse = true);

  /*
   * Method to convert FROM the scale described above to [0, 100].
   * 
   * An extra parameter is exposed: `buffer`, which allows for a range of values before/after the 
   * max that will be mapped to 0 and 100, respectively.
   */
  static uint8_t fromv2scale(uint8_t value, uint8_t endValue, uint8_t interval, bool reverse = true, uint8_t buffer = V2_DEFAULT_RANGE_BUFFER);

protected:
  const uint8_t protocolId;
  const uint8_t numGroups;
  void switchMode(const GroupState& currentState, BulbMode desiredMode);
};
