#pragma once

#include <functional>
#include <Arduino.h>
#include <MiLightRadio.h>
#include <MiLightRadioFactory.h>
#include <MiLightRemoteConfig.h>
#include <Settings.h>
#include <GroupStateStore.h>
#include <PacketSender.h>
#include <TransitionController.h>
#include <cstring>
#include <map>

//#define DEBUG_PRINTF
//#define DEBUG_CLIENT_COMMANDS // enable to show each individual change command (like hue, brightness, etc)

namespace RequestKeys {
  static constexpr char TRANSITION[] = "transition";
};

namespace TransitionParams {
  static constexpr char FIELD[] PROGMEM = "field";
  static constexpr char START_VALUE[] PROGMEM = "start_value";
  static constexpr char END_VALUE[] PROGMEM = "end_value";
  static constexpr char DURATION[] PROGMEM = "duration";
  static constexpr char PERIOD[] PROGMEM = "period";
}

// Used to determine RGB colros that are approximately white
#define RGB_WHITE_THRESHOLD 10

class MiLightClient {
public:
  // Used to indicate that the start value for a transition should be fetched from current state
  static constexpr int16_t FETCH_VALUE_FROM_STATE = -1;

  MiLightClient(
    RadioSwitchboard& radioSwitchboard,
    PacketSender& packetSender,
    GroupStateStore* stateStore,
    Settings& settings,
    TransitionController& transitions
  );

  ~MiLightClient() { }

  typedef std::function<void(void)> EventHandler;

  void prepare(const MiLightRemoteConfig* remoteConfig, uint16_t deviceId = -1, uint8_t groupId = -1);
  void prepare(MiLightRemoteType type, uint16_t deviceId = -1, uint8_t groupId = -1);

  // void setResendCount(const unsigned int resendCount);
  // bool available();
  // size_t read(uint8_t packet[]);
  // void write(uint8_t packet[]);

  void setHeld(bool held) const;

  // Common methods
  void updateStatus(MiLightStatus status) const;
  void updateStatus(MiLightStatus status, uint8_t groupId) const;
  void pair() const;
  void unpair() const;
  void command(uint8_t command, uint8_t arg) const;
  void updateMode(uint8_t mode) const;
  void nextMode() const;
  void previousMode() const;
  void modeSpeedDown() const;
  void modeSpeedUp() const;
  void toggleStatus() const;

  // RGBW methods
  void updateHue(uint16_t hue) const;
  void updateBrightness(uint8_t brightness) const;
  void updateColorWhite() const;
  void updateColorRaw(uint8_t color) const;
  void enableNightMode() const;
  void updateColor(JsonVariant json) const;

  // CCT methods
  void updateTemperature(uint8_t colorTemperature) const;
  void decreaseTemperature() const;
  void increaseTemperature() const;
  void increaseBrightness() const;
  void decreaseBrightness() const;

  void updateSaturation(uint8_t saturation) const;

  void update(JsonObject object);
  void handleCommand(JsonVariant command) const;
  void handleCommands(JsonArray commands) const;
  bool handleTransition(JsonObject args, JsonDocument& responseObj) const;
  void handleTransition(GroupStateField field, JsonVariant value, float duration, int16_t startValue = FETCH_VALUE_FROM_STATE) const;
  void handleEffect(const String& effect) const;

  void onUpdateBegin(const EventHandler &handler);
  void onUpdateEnd(const EventHandler &handler);

  size_t getNumRadios() const;
  std::shared_ptr<MiLightRadio> switchRadio(size_t radioIx) const;
  std::shared_ptr<MiLightRadio> switchRadio(const MiLightRemoteConfig* remoteConfig) const;
  // MiLightRemoteConfig& currentRemoteConfig() const;

  // Call to override the number of packet repeats that are sent.  Clear with clearRepeatsOverride
  void setRepeatsOverride(size_t repeatsOverride);

  // Clear the repeats override so that the default is used
  void clearRepeatsOverride();

  static uint8_t parseStatus(JsonVariant object);
  static JsonVariant extractStatus(JsonObject object);

protected:
  struct cmp_str {
    bool operator()(char const *a, char const *b) const {
        return std::strcmp(a, b) < 0;
    }
  };
  static const std::map<const char*, std::function<void(MiLightClient*, JsonVariant)>, cmp_str> FIELD_SETTERS;
  static const char* FIELD_ORDERINGS[];

  RadioSwitchboard& radioSwitchboard;
  std::vector<std::shared_ptr<MiLightRadio>> radios;
  std::shared_ptr<MiLightRadio> currentRadio;
  const MiLightRemoteConfig* currentRemote;

  EventHandler updateBeginHandler;
  EventHandler updateEndHandler;

  GroupStateStore* stateStore;
  const GroupState* currentState;
  Settings& settings;
  PacketSender& packetSender;
  TransitionController& transitions;

  // If set, override the number of packet repeats used.
  size_t repeatsOverride;

  void flushPacket() const;
};
