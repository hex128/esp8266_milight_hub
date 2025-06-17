#pragma once

// This protocol is documented here:
// http://www.limitlessled.com/dev/

#include <MiLightClient.h>
#include <WiFiUdp.h>
#include <MiLightUdpServer.h>
#include <V6CommandHandler.h>

#include <utility>

#define V6_COMMAND_LEN 8
#define V6_MAX_SESSIONS 10

struct V6Session {
  V6Session(IPAddress ipAddr, const uint16_t port, const uint16_t sessionId)
    : ipAddr(std::move(ipAddr)),
      port(port),
      sessionId(sessionId),
      next(nullptr)
  { }

  IPAddress ipAddr;
  uint16_t port;
  uint16_t sessionId;
  V6Session* next;
};

class V6MiLightUdpServer final : public MiLightUdpServer {
public:
  V6MiLightUdpServer(MiLightClient*& client, const uint16_t port, const uint16_t deviceId)
    : MiLightUdpServer(client, port, deviceId),
      sessionId(0),
      numSessions(0),
      firstSession(nullptr)
  { }

  ~V6MiLightUdpServer() override;

  // Should return size of the response packet
  void handlePacket(uint8_t* packet, size_t packetSize) override;

  template <typename T>
  static T readInt(const uint8_t* packet);

  template <typename T>
  static uint8_t* writeInt(const T& value, uint8_t* packet);

protected:
  static V6CommandDemuxer COMMAND_DEMUXER;

  static uint8_t START_SESSION_COMMAND[];
  static uint8_t START_SESSION_RESPONSE[];
  static uint8_t COMMAND_HEADER[];
  static uint8_t COMMAND_RESPONSE[];
  static uint8_t LOCAL_SEARCH_COMMAND[];
  static uint8_t HEARTBEAT_HEADER[];
  static uint8_t HEARTBEAT_HEADER2[];

  static uint8_t SEARCH_COMMAND[];
  static uint8_t SEARCH_RESPONSE[];

  static uint8_t OPEN_COMMAND_RESPONSE[];

  uint16_t sessionId;
  size_t numSessions;
  V6Session* firstSession;

  uint16_t beginSession();
  bool sendResponse(uint16_t sessionId, const uint8_t* responseBuffer, size_t responseSize);
  static bool matchesPacket(const uint8_t* packet1, size_t packet1Len, const uint8_t* packet2, size_t packet2Len);
  void writeMacAddr(uint8_t* packet) const;

  void handleSearch();
  void handleStartSession();
  bool handleOpenCommand(uint16_t sessionId);
  void handleHeartbeat(uint16_t sessionId);
  void handleCommand(
    uint16_t sessionId,
    uint8_t sequenceNum,
    uint8_t* cmd,
    uint8_t group,
    uint8_t checksum
  );
};
