/**
 * Enqueues updated bulb states and flushes them at the configured interval.
 */
#pragma once

#include <cstddef>
#include <MqttClient.h>
#include <CircularBuffer.h>
#include <Settings.h>

#ifndef MILIGHT_MQTT_JSON_BUFFER_SIZE
#define MILIGHT_MQTT_JSON_BUFFER_SIZE 1024
#endif

class BulbStateUpdater {
public:
  BulbStateUpdater(Settings& settings, MqttClient& mqttClient, GroupStateStore& stateStore);

  void enqueueUpdate(const BulbId &bulbId, GroupState& groupState);
  void loop();
  void enable();
  void disable();

private:
  Settings& settings;
  MqttClient& mqttClient;
  GroupStateStore& stateStore;
  CircularBuffer<BulbId, MILIGHT_MAX_STALE_MQTT_GROUPS> staleGroups;
  unsigned long lastFlush;
  unsigned long lastQueue;
  bool enabled;

  inline void flushGroup(const BulbId &bulbId, const GroupState& state);
  [[nodiscard]] inline bool canFlush() const;
};
