#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GroupStateField.h>
#include <RF24PowerLevel.h>
#include <RF24Channel.h>
#include <LEDStatus.h>
#include <GroupAlias.h>

#include <MiLightRemoteType.h>
#include <BulbId.h>

#include <vector>
#include <memory>
#include <map>

#ifndef MILIGHT_HUB_SETTINGS_BUFFER_SIZE
#define MILIGHT_HUB_SETTINGS_BUFFER_SIZE 4096
#endif

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

#ifndef FIRMWARE_NAME
#define FIRMWARE_NAME unknown
#endif

#ifndef FIRMWARE_VARIANT
#define FIRMWARE_VARIANT unknown
#endif

#ifndef MILIGHT_HUB_VERSION
#define MILIGHT_HUB_VERSION unknown
#endif

#ifdef ESP8266
  #define CSN_DEFAULT_PIN 15
#elif ESP32
  #define CSN_DEFAULT_PIN 5
#endif

#ifndef MILIGHT_MAX_STATE_ITEMS
#define MILIGHT_MAX_STATE_ITEMS 100
#endif

#ifndef MILIGHT_MAX_STALE_MQTT_GROUPS
#define MILIGHT_MAX_STALE_MQTT_GROUPS 10
#endif

#define SETTINGS_FILE  "/config.json"
#define SETTINGS_TERMINATOR '\0'
#define ALIASES_FILE "/aliases.bin"
#define BACKUP_FILE "/backup.bin"

#define WEB_INDEX_FILENAME "/web/index.html"

#define MILIGHT_GITHUB_USER "sidoh"
#define MILIGHT_GITHUB_REPO "esp8266_milight_hub"
#define MILIGHT_REPO_WEB_PATH "/data/web/index.html"

#define MINIMUM_RESTART_PERIOD 1
#define DEFAULT_MQTT_PORT 1883
#define MAX_IP_ADDR_LEN 15

enum RadioInterfaceType {
  nRF24 = 0,
  LT8900 = 1,
};

enum class WifiMode {
  B, G, N
};

static const std::vector DEFAULT_GROUP_STATE_FIELDS({
  GroupStateField::STATE,
  GroupStateField::BRIGHTNESS,
  GroupStateField::COMPUTED_COLOR,
  GroupStateField::MODE,
  GroupStateField::COLOR_TEMP,
  GroupStateField::COLOR_MODE
});

struct GatewayConfig {
  GatewayConfig(uint16_t deviceId, uint16_t port, uint8_t protocolVersion);

  const uint16_t deviceId;
  const uint16_t port;
  const uint8_t protocolVersion;
};

// all keys that appear in JSON
namespace SettingsKeys {
  static constexpr char ADMIN_USERNAME[] PROGMEM = "admin_username";
  static constexpr char ADMIN_PASSWORD[] PROGMEM = "admin_password";
  static constexpr char CE_PIN[] PROGMEM = "ce_pin";
  static constexpr char CSN_PIN[] PROGMEM = "csn_pin";
  static constexpr char RESET_PIN[] PROGMEM = "reset_pin";
  static constexpr char LED_PIN[] PROGMEM = "led_pin";
  static constexpr char PACKET_REPEATS[] PROGMEM = "packet_repeats";
  static constexpr char HTTP_REPEAT_FACTOR[] PROGMEM = "http_repeat_factor";
  static constexpr char AUTO_RESTART_PERIOD[] PROGMEM = "auto_restart_period";
  static constexpr char MQTT_SERVER[] PROGMEM = "mqtt_server";
  static constexpr char MQTT_USERNAME[] PROGMEM = "mqtt_username";
  static constexpr char MQTT_PASSWORD[] PROGMEM = "mqtt_password";
  static constexpr char MQTT_TOPIC_PATTERN[] PROGMEM = "mqtt_topic_pattern";
  static constexpr char MQTT_UPDATE_TOPIC_PATTERN[] PROGMEM = "mqtt_update_topic_pattern";
  static constexpr char MQTT_STATE_TOPIC_PATTERN[] PROGMEM = "mqtt_state_topic_pattern";
  static constexpr char MQTT_CLIENT_STATUS_TOPIC[] PROGMEM = "mqtt_client_status_topic";
  static constexpr char SIMPLE_MQTT_CLIENT_STATUS[] PROGMEM = "simple_mqtt_client_status";
  static constexpr char DISCOVERY_PORT[] PROGMEM = "discovery_port";
  static constexpr char LISTEN_REPEATS[] PROGMEM = "listen_repeats";
  static constexpr char STATE_FLUSH_INTERVAL[] PROGMEM = "state_flush_interval";
  static constexpr char MQTT_STATE_RATE_LIMIT[] PROGMEM = "mqtt_state_rate_limit";
  static constexpr char MQTT_DEBOUNCE_DELAY[] PROGMEM = "mqtt_debounce_delay";
  static constexpr char MQTT_RETAIN[] PROGMEM = "mqtt_retain";
  static constexpr char PACKET_REPEAT_THROTTLE_THRESHOLD[] PROGMEM = "packet_repeat_throttle_threshold";
  static constexpr char PACKET_REPEAT_THROTTLE_SENSITIVITY[] PROGMEM = "packet_repeat_throttle_sensitivity";
  static constexpr char PACKET_REPEAT_MINIMUM[] PROGMEM = "packet_repeat_minimum";
  static constexpr char ENABLE_AUTOMATIC_MODE_SWITCHING[] PROGMEM = "enable_automatic_mode_switching";
  static constexpr char LED_MODE_PACKET_COUNT[] PROGMEM = "led_mode_packet_count";
  static constexpr char HOSTNAME[] PROGMEM = "hostname";
  static constexpr char WIFI_STATIC_IP[] PROGMEM = "wifi_static_ip";
  static constexpr char WIFI_STATIC_IP_GATEWAY[] PROGMEM = "wifi_static_ip_gateway";
  static constexpr char WIFI_STATIC_IP_NETMASK[] PROGMEM = "wifi_static_ip_netmask";
  static constexpr char PACKET_REPEATS_PER_LOOP[] PROGMEM = "packet_repeats_per_loop";
  static constexpr char HOME_ASSISTANT_DISCOVERY_PREFIX[] PROGMEM = "home_assistant_discovery_prefix";
  static constexpr char DEFAULT_TRANSITION_PERIOD[] PROGMEM = "default_transition_period";
  static constexpr char WIFI_MODE[] PROGMEM = "wifi_mode";
  static constexpr char RF24_CHANNELS[] PROGMEM = "rf24_channels";
  static constexpr char RF24_LISTEN_CHANNEL[] PROGMEM = "rf24_listen_channel";
  static constexpr char RF24_POWER_LEVEL[] PROGMEM = "rf24_power_level";
  static constexpr char LED_MODE_WIFI_CONFIG[] PROGMEM = "led_mode_wifi_config";
  static constexpr char LED_MODE_WIFI_FAILED[] PROGMEM = "led_mode_wifi_failed";
  static constexpr char LED_MODE_OPERATING[] PROGMEM = "led_mode_operating";
  static constexpr char LED_MODE_PACKET[] PROGMEM = "led_mode_packet";
  static constexpr char RADIO_INTERFACE_TYPE[] PROGMEM = "radio_interface_type";
  static constexpr char DEVICE_IDS[] PROGMEM = "device_ids";
  static constexpr char GATEWAY_CONFIGS[] PROGMEM = "gateway_configs";
  static constexpr char GROUP_STATE_FIELDS[] PROGMEM = "group_state_fields";
  static constexpr char GROUP_ID_ALIASES[] PROGMEM = "group_id_aliases";
}

class Settings {
public:

  Settings() :
    adminUsername(""),
    adminPassword(""),
    // CE and CSN pins from nrf24l01
    cePin(4),
    csnPin(CSN_DEFAULT_PIN),
    resetPin(0),
    ledPin(-2),
    radioInterfaceType(nRF24),
    packetRepeats(50),
    httpRepeatFactor(1),
    listenRepeats(3),
    discoveryPort(48899),
    mqttTopicPattern("milight/commands/:device_id/:device_type/:group_id"),
    mqttStateTopicPattern("milight/state/:device_id/:device_type/:group_id"),
    mqttClientStatusTopic("milight/client_status"),
    simpleMqttClientStatus(true),
    stateFlushInterval(10000),
    mqttStateRateLimit(500),
    mqttDebounceDelay(500),
    mqttRetain(true),
    packetRepeatThrottleThreshold(200),
    packetRepeatThrottleSensitivity(0),
    packetRepeatMinimum(3),
    enableAutomaticModeSwitching(false),
    ledModeWifiConfig(LEDStatus::LEDMode::FastToggle),
    ledModeWifiFailed(LEDStatus::LEDMode::On),
    ledModeOperating(LEDStatus::LEDMode::SlowBlip),
    ledModePacket(LEDStatus::LEDMode::Flicker),
    ledModePacketCount(3),
    hostname("milight-hub"),
    rf24PowerLevel(RF24PowerLevelHelpers::defaultValue()),
    rf24Channels(RF24ChannelHelpers::allValues()),
    groupStateFields(DEFAULT_GROUP_STATE_FIELDS),
    rf24ListenChannel(RF24Channel::RF24_LOW),
    packetRepeatsPerLoop(10),
    homeAssistantDiscoveryPrefix("homeassistant/"),
    wifiMode(WifiMode::G),
    defaultTransitionPeriod(500),
    groupIdAliasNextId(0),
    _autoRestartPeriod(0)
  { }

  ~Settings() = default;

  bool isAuthenticationEnabled() const;
  const String& getUsername() const;
  const String& getPassword() const;

  bool isAutoRestartEnabled() const;
  size_t getAutoRestartPeriod() const;

  static bool load(Settings& settings);
  static bool loadAliases(Settings& settings);

  static RadioInterfaceType typeFromString(const String& s);
  static String typeToString(RadioInterfaceType type);
  static std::vector<RF24Channel> defaultListenChannels();

  void save() const;
  void serialize(Print& stream, bool prettyPrint = false) const;
  void updateDeviceIds(JsonArray arr);
  void updateGatewayConfigs(JsonArray arr);
  void patch(JsonObject obj);
  String mqttServer();
  uint16_t mqttPort() const;
  std::map<String, GroupAlias>::const_iterator findAlias(MiLightRemoteType deviceType, uint16_t deviceId, uint8_t groupId);
  std::map<String, GroupAlias>::const_iterator findAliasById(size_t id);
  void addAlias(const char* alias, const BulbId& bulbId);
  bool deleteAlias(size_t id);

  String adminUsername;
  String adminPassword;
  uint8_t cePin;
  uint8_t csnPin;
  uint8_t resetPin;
  int8_t ledPin;
  RadioInterfaceType radioInterfaceType;
  size_t packetRepeats;
  size_t httpRepeatFactor;
  uint8_t listenRepeats;
  uint16_t discoveryPort;
  String _mqttServer;
  String mqttUsername;
  String mqttPassword;
  String mqttTopicPattern;
  String mqttUpdateTopicPattern;
  String mqttStateTopicPattern;
  String mqttClientStatusTopic;
  bool simpleMqttClientStatus;
  size_t stateFlushInterval;
  size_t mqttStateRateLimit;
  size_t mqttDebounceDelay;
  bool mqttRetain;
  size_t packetRepeatThrottleThreshold;
  size_t packetRepeatThrottleSensitivity;
  size_t packetRepeatMinimum;
  bool enableAutomaticModeSwitching;
  LEDStatus::LEDMode ledModeWifiConfig;
  LEDStatus::LEDMode ledModeWifiFailed;
  LEDStatus::LEDMode ledModeOperating;
  LEDStatus::LEDMode ledModePacket;
  size_t ledModePacketCount;
  String hostname;
  RF24PowerLevel rf24PowerLevel;
  std::vector<uint16_t> deviceIds;
  std::vector<RF24Channel> rf24Channels;
  std::vector<GroupStateField> groupStateFields;
  std::vector<std::shared_ptr<GatewayConfig>> gatewayConfigs;
  RF24Channel rf24ListenChannel;
  String wifiStaticIP;
  String wifiStaticIPNetmask;
  String wifiStaticIPGateway;
  size_t packetRepeatsPerLoop;
  std::map<String, GroupAlias> groupIdAliases;
  std::map<uint32_t, BulbId> deletedGroupIdAliases;
  String homeAssistantDiscoveryPrefix;
  WifiMode wifiMode;
  uint16_t defaultTransitionPeriod;
  size_t groupIdAliasNextId;

  static WifiMode wifiModeFromString(const String& mode);
  static String wifiModeToString(WifiMode mode);

protected:
  size_t _autoRestartPeriod;

  void parseGroupIdAliases(JsonObject json);
  void dumpGroupIdAliases(JsonObject json) const;

  template <typename T>
  void setIfPresent(JsonObject obj, const __FlashStringHelper* key, T& var) {
    if (obj.containsKey(key)) {
      JsonVariant val = obj[key];

      // For booleans, parse string/int

#ifdef ESP8266
      if (std::is_same_v<bool, T>) {
                  if (val.is<bool>()) {
          var = val.as<bool>();
        } else if (val.is<const char*>()) {
          var = strcmp(val.as<const char*>(), "true") == 0;
        } else if (val.is<int>()) {
          var = val.as<int>() == 1;
        } else {
          var = false;
        }
      } else {
        var = val.as<T>();
      }
#elif ESP32
        if (std::is_same<bool, T>::value) {
            if (val.is<bool>()) {
                var = val.as<bool>();
            } else if (val.is<const char*>()) {
                var = strcmp(val.as<const char*>(), "true") == 0;
            } else if (val.is<int>()) {
                var = val.as<int>() == 1;
            } else {
                var = false;
            }
        } else {
            var = val.as<T>();
        }
#endif

    }
  }
};
