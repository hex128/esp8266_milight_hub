#pragma once

#include <MiLightClient.h>
#include <MiLightRadioConfig.h>

enum V6CommandTypes {
  V6_PAIR = 0x3D,
  V6_UNPAIR = 0x3E,
  V6_PRESET = 0x3F,
  V6_COMMAND = 0x31
};

class V6CommandHandler {
public:
  virtual ~V6CommandHandler() = default;
  static V6CommandHandler* ALL_HANDLERS[];
  static const size_t NUM_HANDLERS;

  V6CommandHandler(const uint16_t commandId, const MiLightRemoteConfig& remoteConfig)
    : commandId(commandId),
      remoteConfig(remoteConfig)
  { }

  virtual bool handleCommand(
    MiLightClient* client,
    uint16_t deviceId,
    uint8_t group,
    uint8_t commandType,
    uint32_t command,
    uint32_t commandArg
  );

  const uint16_t commandId;
  const MiLightRemoteConfig& remoteConfig;

protected:

  virtual bool handleCommand(
    MiLightClient* client,
    uint32_t command,
    uint32_t commandArg
  ) = 0;

  virtual bool handlePreset(
    MiLightClient* client,
    uint8_t commandLsb,
    uint32_t commandArg
  ) = 0;
};

class V6CommandDemuxer final : public V6CommandHandler {
public:
  V6CommandDemuxer(V6CommandHandler* handlers[], const size_t numHandlers)
    : V6CommandHandler(0, FUT096Config),
      handlers(handlers),
      numHandlers(numHandlers)
  { }

  bool handleCommand(
    MiLightClient* client,
    uint16_t deviceId,
    uint8_t group,
    uint8_t commandType,
    uint32_t command,
    uint32_t commandArg
  ) override;

protected:
  V6CommandHandler** handlers;
  size_t numHandlers;

  bool handleCommand(
    MiLightClient* client,
    uint32_t command,
    uint32_t commandArg
  ) override;

  bool handlePreset(
    MiLightClient* client,
    uint8_t commandLsb,
    uint32_t commandArg
  ) override;
};
