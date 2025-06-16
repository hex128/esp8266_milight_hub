#pragma once

#include <WiFiUdp.h>
#include <Settings.h>

class MiLightDiscoveryServer {
public:
  MiLightDiscoveryServer(Settings& settings);
  MiLightDiscoveryServer(MiLightDiscoveryServer&);
  MiLightDiscoveryServer& operator=(MiLightDiscoveryServer other);
  ~MiLightDiscoveryServer();

  void begin();
  void handleClient();

private:
  Settings& settings;
  WiFiUDP socket;

  void handleDiscovery(uint8_t version);
  void sendResponse(char* buffer);
};
