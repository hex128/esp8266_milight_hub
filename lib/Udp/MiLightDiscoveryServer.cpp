#include <MiLightDiscoveryServer.h>
#include <Size.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#elif ESP32
  #include <WiFi.h>
#endif

constexpr char V3_SEARCH_STRING[] = "Link_Wi-Fi";
constexpr char V6_SEARCH_STRING[] = "HF-A11ASSISTHREAD";

MiLightDiscoveryServer::MiLightDiscoveryServer(Settings& settings)
  : settings(settings)
{ }

MiLightDiscoveryServer::MiLightDiscoveryServer(const MiLightDiscoveryServer& other)
  : settings(other.settings)
{ }

MiLightDiscoveryServer& MiLightDiscoveryServer::operator=(const MiLightDiscoveryServer &other) {
  this->settings = other.settings;
  this->socket = other.socket;
  return *this;
}

MiLightDiscoveryServer::~MiLightDiscoveryServer() {
  socket.stop();
}

void MiLightDiscoveryServer::begin() {
  socket.begin(settings.discoveryPort);
}

void MiLightDiscoveryServer::handleClient() {
  if (const size_t packetSize = socket.parsePacket()) {
    char buffer[size(V6_SEARCH_STRING) + 1];
    socket.read(buffer, packetSize);
    buffer[packetSize] = 0;

#ifdef MILIGHT_UDP_DEBUG
    printf("Got discovery packet: %s\n", buffer);
#endif

    if (strcmp(buffer, V3_SEARCH_STRING) == 0) {
      handleDiscovery(5);
    } else if (strcmp(buffer, V6_SEARCH_STRING) == 0) {
      handleDiscovery(6);
    }
  }
}

void MiLightDiscoveryServer::handleDiscovery(uint8_t version) {
#ifdef MILIGHT_UDP_DEBUG
  printf_P(PSTR("Handling discovery for version: %u, %d configs to consider\n"), version, settings.gatewayConfigs.size());
#endif

  char buffer[40];

  for (auto & gatewayConfig : settings.gatewayConfigs) {
    const GatewayConfig& config = *gatewayConfig;

    if (config.protocolVersion != version) {
      continue;
    }

    IPAddress addr = WiFi.localIP();
    char* ptr = buffer;
    ptr += sprintf_P(
      buffer,
      PSTR("%d.%d.%d.%d,00000000%02X%02X"),
      addr[0], addr[1], addr[2], addr[3],
      (config.deviceId >> 8), (config.deviceId & 0xFF)
    );

    if (config.protocolVersion == 5) {
      sendResponse(buffer);
    } else {
      sprintf_P(ptr, PSTR(",HF-LPB100"));
      sendResponse(buffer);
    }
  }
}

void MiLightDiscoveryServer::sendResponse(char* buffer) {
#ifdef MILIGHT_UDP_DEBUG
  printf_P(PSTR("Sending response: %s, remote:"), buffer);
  Serial.print(socket.remoteIP());
  Serial.print(":");
  Serial.println(socket.remotePort());
#endif

  socket.beginPacket(socket.remoteIP(), socket.remotePort());
#ifdef ESP8266
    socket.write(buffer);
#elif ESP32
    socket.write(reinterpret_cast<uint8_t*>(buffer), strnlen(buffer, 40));
#endif
  socket.endPacket();
}
