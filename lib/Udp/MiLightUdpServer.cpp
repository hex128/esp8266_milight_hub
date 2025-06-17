#include <MiLightUdpServer.h>
#include <V5MiLightUdpServer.h>
#include <V6MiLightUdpServer.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#elif ESP32
  #include <WiFi.h>
#endif

MiLightUdpServer::MiLightUdpServer(MiLightClient*& client, const uint16_t port, const uint16_t deviceId)
  : client(client),
    port(port),
    deviceId(deviceId),
    lastGroup(0),
    packetBuffer{},
    responseBuffer{}
{}

MiLightUdpServer::~MiLightUdpServer() {
  stop();
}

void MiLightUdpServer::begin() {
  socket.begin(port);
}

void MiLightUdpServer::stop() {
  socket.stop();
}

void MiLightUdpServer::handleClient() {
  if (const size_t packetSize = socket.parsePacket()) {
    socket.read(packetBuffer, packetSize);

#ifdef MILIGHT_UDP_DEBUG
    printf("[MiLightUdpServer port %d] - Handling packet: ", port);
    for (size_t i = 0; i < packetSize; i++) {
      printf("%02X ", packetBuffer[i]);
    }
    printf("\n");
#endif

    handlePacket(packetBuffer, packetSize);
  }
}

std::shared_ptr<MiLightUdpServer> MiLightUdpServer::fromVersion(const uint8_t version, MiLightClient*& client, uint16_t port, uint16_t deviceId) {
  if (version == 0 || version == 5) {
    return std::make_shared<V5MiLightUdpServer>(client, port, deviceId);
  }
  if (version == 6) {
    return std::make_shared<V6MiLightUdpServer>(client, port, deviceId);
  }

  return nullptr;
}
