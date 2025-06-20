#ifndef UNIT_TEST

#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <cstdlib>
#include <FS.h>
#include <IntParsing.h>
#include <LEDStatus.h>
#include <GroupStateStore.h>
#include <MiLightRadioConfig.h>
#include <MiLightRemoteConfig.h>
#include <MiLightHttpServer.h>
#include <Settings.h>
#include <MiLightUdpServer.h>
#include <MqttClient.h>
#include <MiLightDiscoveryServer.h>
#include <MiLightClient.h>
#include <BulbStateUpdater.h>
#include <RadioSwitchboard.h>
#include <PacketSender.h>
#include <HomeAssistantDiscoveryClient.h>
#include <TransitionController.h>
#include <ProjectWifi.h>

#include <ESPId.h>

#ifndef AP_PASSWORD
#define AP_PASSWORD "milight-hub"
#endif

#ifndef MDNS_HOSTANME
#define MDNS_HOSTNAME "milight-hub"
#endif

#ifdef ESP8266
  #include <ESP8266mDNS.h>
  #include <ESP8266SSDP.h>
#elif ESP32
  #include "ESP32SSDP.h"
  #include <esp_wifi.h>
  #include <SPIFFS.h>
  #include <ESPmDNS.h>
#endif

#include <vector>
#include <memory>
#include "ProjectFS.h"

WiFiManager* wifiManager;
// because of callbacks, these need to be in the higher scope :(
WiFiManagerParameter* wifiStaticIP = nullptr;
WiFiManagerParameter* wifiStaticIPNetmask = nullptr;
WiFiManagerParameter* wifiStaticIPGateway = nullptr;
WiFiManagerParameter* wifiMode = nullptr;

static LEDStatus *ledStatus;

Settings settings;

MiLightClient* milightClient = nullptr;
RadioSwitchboard* radios = nullptr;
PacketSender* packetSender = nullptr;
std::shared_ptr<MiLightRadioFactory> radioFactory;
MiLightHttpServer *httpServer = nullptr;
MqttClient* mqttClient = nullptr;
MiLightDiscoveryServer* discoveryServer = nullptr;
uint8_t currentRadioType = 0;

// For tracking and managing group state
GroupStateStore* stateStore = nullptr;
BulbStateUpdater* bulbStateUpdater = nullptr;
TransitionController transitions;

std::vector<std::shared_ptr<MiLightUdpServer>> udpServers;

/**
 * Set up UDP servers (both v5 and v6).  Clean up old ones if necessary.
 */
void initMilightUdpServers() {
  if (! WiFi.isConnected()) {
    return;
  }

  udpServers.clear();

  for (size_t i = 0; i < settings.gatewayConfigs.size(); ++i) {
    const GatewayConfig& config = *settings.gatewayConfigs[i];

    std::shared_ptr<MiLightUdpServer> server = MiLightUdpServer::fromVersion(
      config.protocolVersion,
      milightClient,
      config.port,
      config.deviceId
    );

    if (server == nullptr) {
      Serial.print(F("Error creating UDP server with protocol version: "));
      Serial.println(config.protocolVersion);
    } else {
      udpServers.push_back(std::move(server));
      udpServers[i]->begin();
    }
  }

  if (discoveryServer) {
    delete discoveryServer;
    discoveryServer = nullptr;
  }
  if (settings.discoveryPort != 0) {
    discoveryServer = new MiLightDiscoveryServer(settings);
    discoveryServer->begin();
  }
}

/**
 * Milight RF packet handler.
 *
 * Called both when a packet is sent locally, and when an intercepted packet
 * is read.
 */
void onPacketSentHandler(const uint8_t* packet, const MiLightRemoteConfig& config) {
  StaticJsonDocument<200> buffer;
  const JsonObject result = buffer.to<JsonObject>();

  const BulbId bulbId = config.packetFormatter->parsePacket(packet, result);

  // set LED mode for a packet movement
  ledStatus->oneshot(settings.ledModePacket, settings.ledModePacketCount);

  if (bulbId == DEFAULT_BULB_ID) {
    Serial.println(F("Skipping packet handler because packet was not decoded"));
    return;
  }

  const MiLightRemoteConfig& remoteConfig =
    *MiLightRemoteConfig::fromType(bulbId.deviceType);

  // update state to reflect changes from this packet
  GroupState* groupState = stateStore->get(bulbId);

  // pass in the previous scratch state as well
  const GroupState stateUpdates(groupState, result);

  if (groupState != nullptr) {
    groupState->patch(stateUpdates);

    // Copy state before setting it to avoid group 0 re-initialization clobbering it
    stateStore->set(bulbId, stateUpdates);
  }

  if (mqttClient) {
    // Sends the state delta derived from the raw packet
    char output[200];
    serializeJson(result, output);
    mqttClient->sendUpdate(remoteConfig, bulbId.deviceId, bulbId.groupId, output);

    // Sends the entire state
    if (groupState != nullptr) {
      bulbStateUpdater->enqueueUpdate(bulbId, *groupState);
    }
  }

  httpServer->handlePacketSent(packet, remoteConfig, bulbId, result);
}

/**
 * Listen for packets on one radio config. Cycles through all configs as it's called.
 */
void handleListen() {
  // Do not handle listens while there are packets enqueued to be sent
  // Doing so causes the radio module to need to be reinitialized inbetween
  // repeats, which slows things down.
  if (! settings.listenRepeats || packetSender->isSending()) {
    return;
  }

  const std::shared_ptr<MiLightRadio> radio = radios->switchRadio(currentRadioType++ % radios->getNumRadios());

  for (size_t i = 0; i < settings.listenRepeats; i++) {
    if (radios->available()) {
      uint8_t readPacket[MILIGHT_MAX_PACKET_LENGTH];
      const size_t packetLen = radios->read(readPacket);

      const MiLightRemoteConfig* remoteConfig = MiLightRemoteConfig::fromReceivedPacket(
        radio->config(),
        readPacket,
        packetLen
      );

      if (remoteConfig == nullptr) {
        // This can happen under normal circumstances, so not an error condition
#ifdef DEBUG_PRINTF
        Serial.println(F("WARNING: Couldn't find remote for received packet"));
#endif
        return;
      }

      // update state to reflect this packet
      onPacketSentHandler(readPacket, *remoteConfig);
    }
  }
}

/**
 * Called when MqttClient#update is first being processed.  Stop sending updates
 * and aggregate state changes until the update is finished.
 */
void onUpdateBegin() {
  if (bulbStateUpdater) {
    bulbStateUpdater->disable();
  }
}

/**
 * Called when MqttClient#update is finished processing.  Re-enable state
 * updates, which will flush accumulated state changes.
 */
void onUpdateEnd() {
  if (bulbStateUpdater) {
    bulbStateUpdater->enable();
  }
}

/**
 * Apply what's in the Settings object.
 */
void applySettings() {
  delete milightClient;

  if (mqttClient) {
    delete mqttClient;
    delete bulbStateUpdater;

    mqttClient = nullptr;
    bulbStateUpdater = nullptr;
  }

  delete stateStore;
  delete packetSender;
  delete radios;

  transitions.setDefaultPeriod(settings.defaultTransitionPeriod);

  radioFactory = MiLightRadioFactory::fromSettings(settings);

  if (radioFactory == nullptr) {
    Serial.println(F("ERROR: unable to construct radio factory"));
  }

  stateStore = new GroupStateStore(MILIGHT_MAX_STATE_ITEMS, settings.stateFlushInterval);

  radios = new RadioSwitchboard(radioFactory, stateStore, settings);
  packetSender = new PacketSender(*radios, settings, onPacketSentHandler);

  milightClient = new MiLightClient(
    *radios,
    *packetSender,
    stateStore,
    settings,
    transitions
  );
  milightClient->onUpdateBegin(onUpdateBegin);
  milightClient->onUpdateEnd(onUpdateEnd);

  if (settings.mqttServer().length() > 0) {
    mqttClient = new MqttClient(settings, milightClient);
    mqttClient->begin();
    mqttClient->onConnect([]() {
      if (settings.homeAssistantDiscoveryPrefix.length() > 0) {
        HomeAssistantDiscoveryClient discoveryClient(settings, mqttClient);
        discoveryClient.sendDiscoverableDevices(settings.groupIdAliases);
        discoveryClient.removeOldDevices(settings.deletedGroupIdAliases);

        settings.deletedGroupIdAliases.clear();
      }
    });

    bulbStateUpdater = new BulbStateUpdater(settings, *mqttClient, *stateStore);
  }

  initMilightUdpServers();

  // update LED pin and operating mode
  if (ledStatus) {
    ledStatus->changePin(settings.ledPin);
    ledStatus->continuous(settings.ledModeOperating);
  }

  WiFi.hostname(settings.hostname);
#ifdef ESP8266
  WiFiPhyMode_t wifiPhyMode;
switch (settings.wifiMode) {
  case WifiMode::B:
    wifiPhyMode = WIFI_PHY_MODE_11B;
    break;
  case WifiMode::G:
    wifiPhyMode = WIFI_PHY_MODE_11G;
    break;
  default:
  case WifiMode::N:
    wifiPhyMode = WIFI_PHY_MODE_11N;
    break;
}
  WiFi.setPhyMode(wifiPhyMode);
#elif ESP32
  switch (settings.wifiMode) {
    case WifiMode::B:
      esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
      break;
    case WifiMode::G:
      esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11G);
      break;
    default:
    case WifiMode::N:
      esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11N);
      break;
  }
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
#endif
}

/**
 *
 */
bool shouldRestart() {
  if (! settings.isAutoRestartEnabled()) {
    return false;
  }

  return settings.getAutoRestartPeriod()*60*1000 < millis();
}

void wifiExtraSettingsChange() {
  settings.wifiStaticIP = wifiStaticIP->getValue();
  settings.wifiStaticIPNetmask = wifiStaticIPNetmask->getValue();
  settings.wifiStaticIPGateway = wifiStaticIPGateway->getValue();
  settings.wifiMode = Settings::wifiModeFromString(wifiMode->getValue());
  settings.save();

  // Restart the device
  delay(100);
  EspClass::restart();
}

void aboutHandler(JsonDocument& json) {
  const JsonObject mqtt = json.createNestedObject(FPSTR("mqtt"));
  mqtt[FPSTR("configured")] = (mqttClient != nullptr);

  if (mqttClient) {
    mqtt[FPSTR("connected")] = mqttClient->isConnected();
    mqtt[FPSTR("status")] = mqttClient->getConnectionStatusString();
  }
}

// Called when a group is deleted via the REST API.  Will publish an empty message to
// the MQTT topic to delete the retained state
void onGroupDeleted(const BulbId& id) {
  if (mqttClient != nullptr) {
    mqttClient->sendState(
      *MiLightRemoteConfig::fromType(id.deviceType),
      id.deviceId,
      id.groupId,
      ""
    );
  }
}

bool initialized = false;
void postConnectSetup() {
  if (initialized) return;
  initialized = true;

  delete wifiManager;
  wifiManager = nullptr;

  MDNS.addService("http", "tcp", 80);

  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("ESP8266 MiLight Gateway");
  SSDP.setSerialNumber(getESPId());
  SSDP.setURL("/");
  SSDP.setDeviceType("upnp:rootdevice");
  SSDP.begin();

  httpServer = new MiLightHttpServer(settings, milightClient, stateStore, packetSender, radios, transitions);
  httpServer->onSettingsSaved(applySettings);
  httpServer->onGroupDeleted(onGroupDeleted);
  httpServer->onAbout(aboutHandler);
  httpServer->on("/description.xml", HTTP_GET, []() { SSDP.schema(httpServer->client()); });
  httpServer->begin();

  transitions.addListener(
      [](const BulbId& bulbId, const GroupStateField field, const uint16_t value) {
          StaticJsonDocument<100> buffer;

          const char* fieldName = GroupStateFieldHelpers::getFieldName(field);
          buffer[fieldName] = value;

          milightClient->prepare(bulbId.deviceType, bulbId.deviceId, bulbId.groupId);
          milightClient->update(buffer.as<JsonObject>());
      }
  );

  initMilightUdpServers();

  Serial.printf_P(PSTR("Setup complete (version %s)\n"), QUOTE(MILIGHT_HUB_VERSION));
}

void setup() {
  Serial.begin(9600);
  char ssid[20];
  sprintf(ssid, "MiLight-%06X", getESPId());

  // load up our persistent settings from the file system
  // ESP8266 doesn't support the formatOnFail parameter
  #ifdef ESP8266
    if (! ProjectFS.begin()) {
      Serial.println(F("Failed to mount file system"));
    }
  #else
    if (! ProjectFS.begin(true)) {
      Serial.println(F("Failed to mount file system"));
    }
  #endif

  Settings::load(settings);
  ESPMH_SETUP_WIFI(settings);
  applySettings();

  // set up the LED status for Wi-Fi configuration
  ledStatus = new LEDStatus(settings.ledPin);
  ledStatus->continuous(settings.ledModeWifiConfig);

  if (!MDNS.begin(MDNS_HOSTNAME)) {
    Serial.println(F("Error setting up MDNS responder"));
  }

  // Allows us to have static IP config in the captive portal. Yucky pointers to pointers, just to have the settings carry through
  wifiManager = new WiFiManager();

  // Setting breakAfterConfig to true causes wifiExtraSettingsChange to be called whenever config params are changed
  // (even when connection fails or user is just changing settings and not network)
  wifiManager->setBreakAfterConfig(true);
  wifiManager->setSaveConfigCallback(wifiExtraSettingsChange);

  wifiManager->setConfigPortalBlocking(false);
  // wifiManager->setConnectTimeout(20);
  // wifiManager->setConnectRetries(5);

  wifiStaticIP = new WiFiManagerParameter(
    "staticIP",
    "Static IP address (leave blank to use DHCP)",
    settings.wifiStaticIP.c_str(),
    MAX_IP_ADDR_LEN
  );
  wifiManager->addParameter(wifiStaticIP);

  wifiStaticIPNetmask = new WiFiManagerParameter(
    "netmask",
    "Netmask (required if IP given)",
    settings.wifiStaticIPNetmask.c_str(),
    MAX_IP_ADDR_LEN
  );
  wifiManager->addParameter(wifiStaticIPNetmask);

  wifiStaticIPGateway = new WiFiManagerParameter(
    "gateway",
    "Default Gateway (optional, only used if static IP)",
    settings.wifiStaticIPGateway.c_str(),
    MAX_IP_ADDR_LEN
  );
  wifiManager->addParameter(wifiStaticIPGateway);

  wifiMode = new WiFiManagerParameter(
    "wifiMode",
    "WiFi Mode (b/g/n)",
    settings.wifiMode == WifiMode::B ? "b" : settings.wifiMode == WifiMode::G ? "g" : "n",
    1
  );
  wifiManager->addParameter(wifiMode);

  // We have a saved static IP, let's try and use it.
  if (settings.wifiStaticIP.length() > 0) {
    Serial.printf_P(PSTR("We have a static IP: %s\n"), settings.wifiStaticIP.c_str());

    IPAddress _ip, _subnet, _gw;
    _ip.fromString(settings.wifiStaticIP);
    _subnet.fromString(settings.wifiStaticIPNetmask);
    _gw.fromString(settings.wifiStaticIPGateway);

    wifiManager->setSTAStaticIPConfig(_ip,_gw,_subnet);
  }

  // wifiManager->setConfigPortalTimeout(180);
  // wifiManager->setConfigPortalTimeoutCallback([]() {
  //     ledStatus->continuous(settings.ledModeWifiFailed);
  //
  //     Serial.println(F("Wi-Fi config portal timed out.  Restarting..."));
  // });

  if (wifiManager->autoConnect(ssid, AP_PASSWORD)) {
    // set LED mode for successful operation
    ledStatus->continuous(settings.ledModeOperating);
    Serial.println(F("Wifi connected successfully\n"));

    // if the config portal was started, make sure to turn off the config AP
    WiFi.mode(WIFI_STA);

    postConnectSetup();
  } else {
    wifiManager->startConfigPortal(ssid, AP_PASSWORD);
  }
}

size_t i = 0;

void loop() {
  // update LED with status
  ledStatus->handle();

  if (shouldRestart()) {
    Serial.println(F("Auto-restart triggered. Restarting..."));
    EspClass::restart();
  }

  if (wifiManager) {
    wifiManager->process();
  }

  if (WiFi.getMode() == WIFI_STA && WiFi.isConnected()) {
    postConnectSetup();

    MDNS.update();

    httpServer->handleClient();
    if (mqttClient) {
      mqttClient->handleClient();
      bulbStateUpdater->loop();
    }

    for (const auto & udpServer : udpServers) {
      udpServer->handleClient();
    }

    if (discoveryServer) {
      discoveryServer->handleClient();
    }

    handleListen();

    stateStore->limitedFlush();
    packetSender->loop();

    transitions.loop();
  }
}

#endif
