#pragma once

#include <BulbId.h>
#include <MqttClient.h>
#include <map>

class HomeAssistantDiscoveryClient {
public:
  HomeAssistantDiscoveryClient(Settings& settings, MqttClient* mqttClient);

  void addConfig(const char* alias, const BulbId& bulbId) const;
  void removeConfig(const BulbId& bulbId) const;

  void sendDiscoverableDevices(const std::map<String, GroupAlias>& aliases) const;
  void removeOldDevices(const std::map<uint32_t, BulbId>& aliases) const;

private:
  Settings& settings;
  MqttClient* mqttClient;

  [[nodiscard]] String buildTopic(const BulbId& bulbId) const;
  static String bindTopicVariables(const String& topic, const char* alias, const BulbId& bulbId);
  static void addNumberedEffects(const JsonArray& effectList, uint8_t start, uint8_t end);
};
