#pragma once

#include <cstdint>
#include <MiLightRemoteType.h>
#include <ArduinoJson.h>

struct BulbId {
  uint16_t deviceId;
  uint8_t groupId;
  MiLightRemoteType deviceType;

  BulbId();
  BulbId(const BulbId& other);
  BulbId(uint16_t deviceId, uint8_t groupId, MiLightRemoteType deviceType);
  bool operator==(const BulbId& other) const;
  void operator=(const BulbId& other);

  [[nodiscard]] uint32_t getCompactId() const;
  [[nodiscard]] String getHexDeviceId() const;
  void serialize(JsonObject json) const;
  void serialize(JsonArray json) const;
  void load(Stream& stream);
  void dump(Stream& stream) const;
};
