#pragma once

#include <MiLightRadioConfig.h>
#include <PacketFormatter.h>

class MiLightRemoteConfig {
public:
  MiLightRemoteConfig(
    PacketFormatter* packetFormatter,
    MiLightRadioConfig& radioConfig,
    const MiLightRemoteType type,
    String name,
    const size_t numGroups
  ) : packetFormatter(packetFormatter),
      radioConfig(radioConfig),
      type(type),
      name(std::move(name)),
      numGroups(numGroups)
  { }

  PacketFormatter* const packetFormatter;
  const MiLightRadioConfig& radioConfig;
  const MiLightRemoteType type;
  const String name;
  const size_t numGroups;

  static const MiLightRemoteConfig* fromType(MiLightRemoteType type);
  static const MiLightRemoteConfig* fromType(const String& type);
  static const MiLightRemoteConfig* fromReceivedPacket(const MiLightRadioConfig& radioConfig, const uint8_t* packet, size_t len);

  static const size_t NUM_REMOTES;
  static const MiLightRemoteConfig* ALL_REMOTES[];
};

extern const MiLightRemoteConfig FUT096Config; //rgbw
extern const MiLightRemoteConfig FUT007Config; //cct
extern const MiLightRemoteConfig FUT092Config; //rgb+cct
extern const MiLightRemoteConfig FUT089Config; //rgb+cct B8 / FUT089
extern const MiLightRemoteConfig FUT098Config; //rgb
extern const MiLightRemoteConfig FUT091Config; //v2 cct
extern const MiLightRemoteConfig FUT020Config;
