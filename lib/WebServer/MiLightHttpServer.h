#pragma once

#include <RichHttpServer.h>
#include <MiLightClient.h>
#include <Settings.h>
#include <WebSocketsServer.h>
#include <GroupStateStore.h>
#include <RadioSwitchboard.h>
#include <PacketSender.h>
#include <TransitionController.h>

#define MAX_DOWNLOAD_ATTEMPTS 3

typedef std::function<void()> SettingsSavedHandler;
typedef std::function<void(const BulbId& id)> GroupDeletedHandler;
typedef std::function<void()> THandlerFunction;
typedef std::function<void(JsonDocument& response)> AboutHandler;

using RichHttpConfig = RichHttp::Generics::Configs::EspressifBuiltin;
using RequestContext = RichHttpConfig::RequestContextType;

constexpr char APPLICATION_OCTET_STREAM[] PROGMEM = "application/octet-stream";
constexpr char TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr char APPLICATION_JSON[] = "application/json";
const std::vector NORMALIZED_GROUP_STATE_FIELDS = {
    GroupStateField::STATE,
    GroupStateField::COLOR_MODE,
    GroupStateField::LEVEL,
    GroupStateField::COLOR,
    GroupStateField::KELVIN,
};

static constexpr uint8_t DEFAULT_PAGE_SIZE = 10;

class MiLightHttpServer {
public:
  MiLightHttpServer(
    Settings& settings,
    MiLightClient*& milightClient,
    GroupStateStore*& stateStore,
    PacketSender*& packetSender,
    RadioSwitchboard*& radios,
    TransitionController& transitions
  )
    : authProvider(settings)
    , server(80, authProvider)
    , wsServer(WebSocketsServer(8000))
    , numWsClients(0)
    , milightClient(milightClient)
    , settings(settings)
    , stateStore(stateStore)
    , packetSender(packetSender)
    , radios(radios)
    , transitions(transitions)
  { }

  void begin();
  void handleClient();
  void onSettingsSaved(const SettingsSavedHandler &handler);
  void onGroupDeleted(const GroupDeletedHandler &handler);
  void onAbout(const AboutHandler &handler);
  void on(const char* path, HTTPMethod method, const THandlerFunction &handler);
  void handlePacketSent(const uint8_t* packet, const MiLightRemoteConfig& config, const BulbId& bulbId, const JsonObject& result);
  WiFiClient client();

protected:

  bool serveFile(const char* file, const char* contentType = "text/html");
  void handleServe_P(const char* data, size_t length, const char* contentType);
  void sendGroupState(bool allowAsync, const BulbId& bulbId, RichHttp::Response& response) const;

  void serveSettings();
  void handleUpdateSettings(RequestContext& request) const;
  void handleUpdateSettingsPost(const RequestContext& request) const;
  void handleUpdateFile(const char* filename);

  static void handleGetRadioConfigs(const RequestContext& request);

  void handleAbout(const RequestContext& request) const;
  void handleSystemPost(RequestContext& request);
  void handleFirmwareUpload();
  void handleFirmwarePost();
  void handleListenGateway(const RequestContext& request);
  void handleSendRaw(RequestContext& request) const;

  void handleUpdateGroup(RequestContext& request) const;
  void handleUpdateGroupAlias(RequestContext& request) const;

  void handleListGroups();
  void handleGetGroup(const RequestContext& request) const;
  void handleGetGroupAlias(const RequestContext& request) const;
  void _handleGetGroup(bool allowAsync, const BulbId &bulbId, const RequestContext& request) const;
  void handleBatchUpdateGroups(RequestContext& request) const;

  void handleDeleteGroup(const RequestContext& request) const;
  void handleDeleteGroupAlias(const RequestContext& request) const;
  void _handleDeleteGroup(const BulbId &bulbId, const RequestContext& request) const;

  void handleGetTransition(const RequestContext& request) const;
  void handleDeleteTransition(const RequestContext& request) const;
  void handleCreateTransition(RequestContext& request) const;
  void handleListTransitions(const RequestContext& request) const;

  // CRUD methods for /aliases
  void handleListAliases(const RequestContext& request) const;
  void handleCreateAlias(RequestContext& request) const;
  void handleDeleteAlias(const RequestContext& request) const;
  void handleUpdateAlias(RequestContext& request) const;
  void handleDeleteAliases(const RequestContext& request) const;
  void handleUpdateAliases(const RequestContext& request) const;

  void handleCreateBackup(const RequestContext& request);
  void handleRestoreBackup(const RequestContext& request) const;

  void handleRequest(const JsonObject& request) const;
  void handleWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

  void saveSettings() const;

  File updateFile;

  PassthroughAuthProvider<Settings> authProvider;
  RichHttpServer<RichHttp::Generics::Configs::EspressifBuiltin> server;
  WebSocketsServer wsServer;
  size_t numWsClients;
  MiLightClient*& milightClient;
  Settings& settings;
  GroupStateStore*& stateStore;
  SettingsSavedHandler settingsSavedHandler;
  GroupDeletedHandler groupDeletedHandler;
  THandlerFunction _handleRootPage;
  PacketSender*& packetSender;
  RadioSwitchboard*& radios;
  TransitionController& transitions;
  AboutHandler aboutHandler;
};
