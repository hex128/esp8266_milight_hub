#include <FS.h>
#include <WiFiUdp.h>
#include <IntParsing.h>
#include <Settings.h>
#include <MiLightHttpServer.h>
#include <MiLightRadioConfig.h>
#include <cstring>
#include <TokenIterator.h>
#include <AboutHelper.h>
#include <GroupAlias.h>
#include <ProjectFS.h>
#include <StreamUtils.h>

#include <index.html.h>
#include <style.css.h>
#include <script.js.h>
#include <BackupManager.h>

#ifdef ESP32
  #include <SPIFFS.h>
  #include <Update.h>
#endif

using namespace std::placeholders;

void MiLightHttpServer::begin() {
  server
    .buildHandler("/")
    .onSimple(HTTP_GET, [this](auto *) { handleServe_P(index_html, index_html_len, "text/html"); });

  server
    .buildHandler(style_css_filename)
    .onSimple(HTTP_GET, [this](auto *) { handleServe_P(style_css, style_css_len, "text/css"); });

  server
    .buildHandler(script_js_filename)
    .onSimple(HTTP_GET, [this](auto *) { handleServe_P(script_js, script_js_len, "application/javascript"); });

  server
    .buildHandler("/settings")
    .on(HTTP_GET, [this](auto&) { serveSettings(); })
    .on(HTTP_PUT, [this](auto && PH1) { handleUpdateSettings(std::forward<decltype(PH1)>(PH1)); })
    .on(
      HTTP_POST,
      [this](auto && PH1) { handleUpdateSettingsPost(std::forward<decltype(PH1)>(PH1)); },
      [this](auto&) { handleUpdateFile(SETTINGS_FILE); }
    );

  server
    .buildHandler("/backup")
    .on(HTTP_GET, [this](auto && PH1) { handleCreateBackup(std::forward<decltype(PH1)>(PH1)); })
    .on(
        HTTP_POST,
        [this](auto && PH1) { handleRestoreBackup(std::forward<decltype(PH1)>(PH1)); },
        [this](auto&) { handleUpdateFile(BACKUP_FILE); });

  server
    .buildHandler("/remote_configs")
    .on(HTTP_GET, [this](auto && PH1) { handleGetRadioConfigs(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/gateway_traffic")
    .on(HTTP_GET, [this](auto && PH1) { handleListenGateway(std::forward<decltype(PH1)>(PH1)); });
  server
    .buildHandler("/gateway_traffic/:type")
    .on(HTTP_GET, [this](auto && PH1) { handleListenGateway(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/gateways/:device_id/:type/:group_id")
    .on(HTTP_PUT, [this](auto && PH1) { handleUpdateGroup(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_POST, [this](auto && PH1) { handleUpdateGroup(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_DELETE, [this](auto && PH1) { handleDeleteGroup(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_GET, [this](auto && PH1) { handleGetGroup(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/gateways/:device_alias")
    .on(HTTP_PUT, [this](auto && PH1) { handleUpdateGroupAlias(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_POST, [this](auto && PH1) { handleUpdateGroupAlias(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_DELETE, [this](auto && PH1) { handleDeleteGroupAlias(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_GET, [this](auto && PH1) { handleGetGroupAlias(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/gateways")
    .onSimple(HTTP_GET, [this](auto *) { handleListGroups(); })
    .on(HTTP_PUT, [this](auto && PH1) { handleBatchUpdateGroups(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/transitions/:id")
    .on(HTTP_GET, [this](auto && PH1) { handleGetTransition(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_DELETE, [this](auto && PH1) { handleDeleteTransition(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/transitions")
    .on(HTTP_GET, [this](auto && PH1) { handleListTransitions(std::forward<decltype(PH1)>(PH1)); })
    .on(HTTP_POST, [this](auto && PH1) { handleCreateTransition(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/raw_commands/:type")
    .on(HTTP_ANY, [this](auto && PH1) { handleSendRaw(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/about")
    .on(HTTP_GET, [this](auto && PH1) { handleAbout(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/system")
    .on(HTTP_POST, [this](auto && PH1) { handleSystemPost(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/aliases")
    .on(HTTP_GET, std::bind(&MiLightHttpServer::handleListAliases, this, _1))
    .on(HTTP_POST, [this](auto && PH1) { handleCreateAlias(std::forward<decltype(PH1)>(PH1)); });

  server
    .buildHandler("/aliases.bin")
    .on(HTTP_GET, [this](auto&) { return serveFile(ALIASES_FILE, APPLICATION_OCTET_STREAM); })
    .on(HTTP_DELETE, [this](auto && PH1) { handleDeleteAliases(std::forward<decltype(PH1)>(PH1)); })
    .on(
        HTTP_POST,
        [this](auto && PH1) { handleUpdateAliases(std::forward<decltype(PH1)>(PH1)); },
        [this](auto&) { handleUpdateFile(ALIASES_FILE); }
    );

  server
    .buildHandler("/aliases/:id")
    .on(HTTP_PUT, std::bind(&MiLightHttpServer::handleUpdateAlias, this, _1))
    .on(HTTP_DELETE, std::bind(&MiLightHttpServer::handleDeleteAlias, this, _1));

  server
    .buildHandler("/firmware")
    .handleOTA();

  server.clearBuilders();

  // set up web socket server
  wsServer.onEvent(
    [this](const uint8_t num, const WStype_t type, uint8_t * payload, const size_t length) {
      handleWsEvent(num, type, payload, length);
    }
  );
  wsServer.begin();

  server.begin();
}

void MiLightHttpServer::onAbout(const AboutHandler &handler) {
  this->aboutHandler = handler;
}

void MiLightHttpServer::handleClient() {
  server.handleClient();
  wsServer.loop();
}

WiFiClient MiLightHttpServer::client() {
  return server.client();
}

void MiLightHttpServer::on(const char *path, const HTTPMethod method, const THandlerFunction &handler) {
  server.on(path, method, handler);
}

void MiLightHttpServer::handleSystemPost(RequestContext& request) {
  const auto requestBody = request.getJsonBody().as<JsonObject>();

  bool handled = false;

  if (requestBody.containsKey(GroupStateFieldNames::COMMAND)) {
    if (requestBody[GroupStateFieldNames::COMMAND] == "restart") {
      Serial.println(F("Restarting..."));
      server.send_P(200, TEXT_PLAIN, PSTR("{\"success\": true}"));

      delay(100);

      ESP.restart();

      handled = true;
    } else if (requestBody[GroupStateFieldNames::COMMAND] == "clear_wifi_config") {
      Serial.println(F("Resetting Wifi and then Restarting..."));
      server.send_P(200, TEXT_PLAIN, PSTR("{\"success\": true}"));

      delay(100);
#ifdef ESP8266
      ESP.eraseConfig();
#elif ESP32
      Serial.println(F("Wifi reset..."));
      WiFi.disconnect(true, true);
      delay(1000);
#endif
      delay(100);
      ESP.restart();

      handled = true;
    }
  }

  if (handled) {
    request.response.json["success"] = true;
  } else {
    request.response.json["success"] = false;
    request.response.json["error"] = "Unhandled command";
    request.response.setCode(400);
  }
}

void MiLightHttpServer::serveSettings() {
  // Save first to set defaults
  settings.save();
  serveFile(SETTINGS_FILE, APPLICATION_JSON);
}

void MiLightHttpServer::onSettingsSaved(const SettingsSavedHandler &handler) {
  this->settingsSavedHandler = handler;
}

void MiLightHttpServer::onGroupDeleted(const GroupDeletedHandler &handler) {
  this->groupDeletedHandler = handler;
}

void MiLightHttpServer::handleAbout(const RequestContext& request) const {
  AboutHelper::generateAboutObject(request.response.json);

  if (this->aboutHandler) {
    this->aboutHandler(request.response.json);
  }

  const JsonObject queueStats = request.response.json.createNestedObject("queue_stats");
  queueStats[F("length")] = packetSender->queueLength();
  queueStats[F("dropped_packets")] = packetSender->droppedPackets();
}

void MiLightHttpServer::handleGetRadioConfigs(const RequestContext& request) {
  const auto arr = request.response.json.to<JsonArray>();

  for (size_t i = 0; i < MiLightRemoteConfig::NUM_REMOTES; i++) {
    const MiLightRemoteConfig* config = MiLightRemoteConfig::ALL_REMOTES[i];
    arr.add(config->name);
  }
}

bool MiLightHttpServer::serveFile(const char* file, const char* contentType) {
  if (ProjectFS.exists(file)) {
    File f = ProjectFS.open(file, "r");
    server.streamFile(f, contentType);
    f.close();
    return true;
  }

  return false;
}

void MiLightHttpServer::handleUpdateFile(const char* filename) {
  const HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    updateFile = ProjectFS.open(filename, "w");
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if (updateFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println(F("Error updating web file"));
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    updateFile.close();
  }
}

void MiLightHttpServer::handleUpdateSettings(RequestContext& request) const {
  if (const JsonObject parsedSettings = request.getJsonBody().as<JsonObject>(); ! parsedSettings.isNull()) {
    settings.patch(parsedSettings);
    saveSettings();

    request.response.json["success"] = true;
    Serial.println(F("Settings successfully updated"));
  }
}

void MiLightHttpServer::handleUpdateSettingsPost(const RequestContext& request) const {
  Settings::load(settings);

  if (this->settingsSavedHandler) {
    this->settingsSavedHandler();
  }

  request.response.json["success"] = true;
}

void MiLightHttpServer::handleFirmwarePost() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");

  if (Update.hasError()) {
    server.send_P(
      500,
      TEXT_PLAIN,
      PSTR("Failed updating firmware. Check serial logs for more information. You may need to re-flash the device.")
    );
  } else {
    server.send_P(
      200,
      TEXT_PLAIN,
      PSTR("Success. Device will now reboot.")
    );
  }

  delay(1000);

  ESP.restart();
}

void MiLightHttpServer::handleFirmwareUpload() {
#ifdef ESP8266
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    WiFiUDP::stopAll();
    //start with max available size
    if(const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000; !Update.begin(maxSketchSpace)){
      Update.printError(Serial);
    }
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
      Update.printError(Serial);
    }
  } else if(upload.status == UPLOAD_FILE_END){
    if(Update.end(true)){ //true to set the size to the current progress
    } else {
      Update.printError(Serial);
    }
  }
  yield();
#elif ESP32
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { // true to set the size to the current progress
      Serial.println("Update Success: Rebooting...");
      ESP.restart();
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    Serial.println("Update was aborted");
  }
  delay(0);
#endif
}


void MiLightHttpServer::handleListenGateway(const RequestContext& request) {
  const bool listenAll = !request.pathVariables.hasBinding("type");
  size_t configIx = 0;
  std::shared_ptr<MiLightRadio> radio = nullptr;
  const MiLightRemoteConfig* remoteConfig = nullptr;
  const MiLightRemoteConfig* tmpRemoteConfig = nullptr;

  uint8_t packet[MILIGHT_MAX_PACKET_LENGTH];

  if (!listenAll) {
    const String strType(request.pathVariables.get("type"));
    tmpRemoteConfig = MiLightRemoteConfig::fromType(strType);
    milightClient->prepare(tmpRemoteConfig, 0, 0);
  }

  if (tmpRemoteConfig == nullptr && !listenAll) {
    request.response.setCode(400);
    request.response.json["error"] = "Unknown device type supplied";
    return;
  }

  if (tmpRemoteConfig != nullptr) {
    radio = radios->switchRadio(tmpRemoteConfig);
  }

  while (remoteConfig == nullptr) {
    if (!server.client().connected()) {
      return;
    }

    if (listenAll) {
      radio = radios->switchRadio(configIx++ % radios->getNumRadios());
    } else {
      radio->configure();
    }

    if (radios->available()) {
      const size_t packetLen = radios->read(packet);
      remoteConfig = MiLightRemoteConfig::fromReceivedPacket(
        radio->config(),
        packet,
        packetLen
      );
    }

    yield();
  }

  char responseBody[200];
  char* responseBuffer = responseBody;

  responseBuffer += sprintf_P(
    responseBuffer,
    PSTR("\n%s packet received (%d bytes):\n"),
    remoteConfig->name.c_str(),
    remoteConfig->packetFormatter->getPacketLength()
  );
  remoteConfig->packetFormatter->format(packet, responseBuffer);

  request.response.json["packet_info"] = responseBody;
}

void MiLightHttpServer::sendGroupState(const bool allowAsync, const BulbId& bulbId, RichHttp::Response& response) const {
  const bool blockOnQueue = server.arg("blockOnQueue").equalsIgnoreCase("true");
  const bool normalizedFormat = server.arg("fmt").equalsIgnoreCase("normalized");

  // Wait for packet queue to flush out.  State will not have been updated before that.
  // Bit hacky to call loop outside of main loop, but should be fine.
  while (blockOnQueue && packetSender->isSending()) {
    packetSender->loop();
  }

  const JsonObject obj = response.json.to<JsonObject>();
  const GroupState* state = stateStore->get(bulbId);

  if (blockOnQueue || allowAsync) {
    if (state == nullptr) {
      obj[F("error")] = F("not found");
      response.setCode(404);
    } else {
      state->applyState(obj, bulbId, normalizedFormat ? NORMALIZED_GROUP_STATE_FIELDS : settings.groupStateFields);
    }
  } else {
    obj[F("success")] = true;
  }
}

void MiLightHttpServer::_handleGetGroup(const bool allowAsync, const BulbId &bulbId, const RequestContext& request) const {
  sendGroupState(allowAsync, bulbId, request.response);
}

void MiLightHttpServer::handleGetGroupAlias(const RequestContext& request) const {
  const String alias = request.pathVariables.get("device_alias");

  const auto it = settings.groupIdAliases.find(alias);

  if (it == settings.groupIdAliases.end()) {
    request.response.setCode(404);
    request.response.json[F("error")] = F("Device alias not found");
    return;
  }

  _handleGetGroup(true, it->second.bulbId, request);
}

void MiLightHttpServer::handleGetGroup(const RequestContext& request) const {
  const String _deviceId = request.pathVariables.get(GroupStateFieldNames::DEVICE_ID);
  const uint8_t _groupId = atoi(request.pathVariables.get(GroupStateFieldNames::GROUP_ID));
  const MiLightRemoteConfig* _remoteType = MiLightRemoteConfig::fromType(request.pathVariables.get("type"));

  if (_remoteType == nullptr) {
    char buffer[40];
    sprintf_P(buffer, PSTR("Unknown device type\n"));
    request.response.setCode(400);
    request.response.json["error"] = buffer;
    return;
  }

  const BulbId bulbId(parseInt<uint16_t>(_deviceId), _groupId, _remoteType->type);
  _handleGetGroup(true, bulbId, request);
}

void MiLightHttpServer::handleDeleteGroup(const RequestContext& request) const {
  const char* _deviceId = request.pathVariables.get("device_id");
  const uint8_t _groupId = atoi(request.pathVariables.get(GroupStateFieldNames::GROUP_ID));
  const MiLightRemoteConfig* _remoteType = MiLightRemoteConfig::fromType(request.pathVariables.get("type"));

  if (_remoteType == nullptr) {
    char buffer[40];
    sprintf_P(buffer, PSTR("Unknown device type\n"));
    request.response.setCode(400);
    request.response.json["error"] = buffer;
    return;
  }

  const BulbId bulbId(parseInt<uint16_t>(_deviceId), _groupId, _remoteType->type);
  _handleDeleteGroup(bulbId, request);
}

void MiLightHttpServer::handleDeleteGroupAlias(const RequestContext& request) const {
  const String alias = request.pathVariables.get("device_alias");

  const auto it = settings.groupIdAliases.find(alias);

  if (it == settings.groupIdAliases.end()) {
    request.response.setCode(404);
    request.response.json[F("error")] = F("Device alias not found");
    return;
  }

  _handleDeleteGroup(it->second.bulbId, request);
}

void MiLightHttpServer::_handleDeleteGroup(const BulbId &bulbId, const RequestContext& request) const {
  stateStore->clear(bulbId);

  if (groupDeletedHandler != nullptr) {
    this->groupDeletedHandler(bulbId);
  }

  request.response.json["success"] = true;
}

void MiLightHttpServer::handleUpdateGroupAlias(RequestContext& request) const {
  const String alias = request.pathVariables.get("device_alias");

  const auto it = settings.groupIdAliases.find(alias);

  if (it == settings.groupIdAliases.end()) {
    request.response.setCode(404);
    request.response.json[F("error")] = F("Device alias not found");
    return;
  }

  BulbId& bulbId = it->second.bulbId;
  const MiLightRemoteConfig* config = MiLightRemoteConfig::fromType(bulbId.deviceType);

  if (config == nullptr) {
    char buffer[40];
    sprintf_P(buffer, PSTR("Unknown device type: %s"), bulbId.deviceType);
    request.response.setCode(400);
    request.response.json["error"] = buffer;
    return;
  }

  milightClient->prepare(config, bulbId.deviceId, bulbId.groupId);
  handleRequest(request.getJsonBody().as<JsonObject>());
  sendGroupState(false, bulbId, request.response);
}

void MiLightHttpServer::handleUpdateGroup(RequestContext& request) const {
  const JsonObject reqObj = request.getJsonBody().as<JsonObject>();

  const String _deviceIds = request.pathVariables.get(GroupStateFieldNames::DEVICE_ID);
  const String _groupIds = request.pathVariables.get(GroupStateFieldNames::GROUP_ID);
  const String _remoteTypes = request.pathVariables.get("type");
  char deviceIds[_deviceIds.length()];
  char groupIds[_groupIds.length()];
  char remoteTypes[_remoteTypes.length()];
  strcpy(remoteTypes, _remoteTypes.c_str());
  strcpy(groupIds, _groupIds.c_str());
  strcpy(deviceIds, _deviceIds.c_str());

  TokenIterator deviceIdItr(deviceIds, _deviceIds.length());
  TokenIterator groupIdItr(groupIds, _groupIds.length());
  TokenIterator remoteTypesItr(remoteTypes, _remoteTypes.length());

  BulbId foundBulbId;
  size_t groupCount = 0;

  while (remoteTypesItr.hasNext()) {
    const char* _remoteType = remoteTypesItr.nextToken();
    const MiLightRemoteConfig* config = MiLightRemoteConfig::fromType(_remoteType);

    if (config == nullptr) {
      char buffer[40];
      sprintf_P(buffer, PSTR("Unknown device type: %s"), _remoteType);
      request.response.setCode(400);
      request.response.json["error"] = buffer;
      return;
    }

    deviceIdItr.reset();
    while (deviceIdItr.hasNext()) {
      const uint16_t deviceId = parseInt<uint16_t>(deviceIdItr.nextToken());

      groupIdItr.reset();
      while (groupIdItr.hasNext()) {
        const uint8_t groupId = atoi(groupIdItr.nextToken());

        milightClient->prepare(config, deviceId, groupId);
        handleRequest(reqObj);
        foundBulbId = BulbId(deviceId, groupId, config->type);
        groupCount++;
      }
    }
  }

  if (groupCount == 1) {
    sendGroupState(false, foundBulbId, request.response);
  } else {
    request.response.json["success"] = true;
  }
}

void MiLightHttpServer::handleRequest(const JsonObject& request) const {
  milightClient->setRepeatsOverride(
    settings.httpRepeatFactor * settings.packetRepeats
  );
  milightClient->update(request);
  milightClient->clearRepeatsOverride();
}

void MiLightHttpServer::handleSendRaw(RequestContext& request) const {
  const JsonObject requestBody = request.getJsonBody().as<JsonObject>();
  const MiLightRemoteConfig* config = MiLightRemoteConfig::fromType(request.pathVariables.get("type"));

  if (config == nullptr) {
    char buffer[50];
    sprintf_P(buffer, PSTR("Unknown device type: %s"), request.pathVariables.get("type"));
    request.response.setCode(400);
    request.response.json["error"] = buffer;
    return;
  }

  uint8_t packet[MILIGHT_MAX_PACKET_LENGTH];
  const String& hexPacket = requestBody["packet"];
  hexStrToBytes<uint8_t>(hexPacket.c_str(), hexPacket.length(), packet, MILIGHT_MAX_PACKET_LENGTH);

  size_t numRepeats = settings.packetRepeats;
  if (requestBody.containsKey("num_repeats")) {
    numRepeats = requestBody["num_repeats"];
  }

  packetSender->enqueue(packet, config, numRepeats);

  // To make this response synchronous, wait for packet to be flushed
  while (packetSender->isSending()) {
    packetSender->loop();
  }

  request.response.json["success"] = true;
}

void MiLightHttpServer::handleWsEvent(uint8_t num, const WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      if (numWsClients > 0) {
        numWsClients--;
      }
      break;

    case WStype_CONNECTED:
      numWsClients++;
      break;

    default:
      Serial.printf_P(PSTR("Unhandled websocket event: %d\n"), static_cast<uint8_t>(type));
      break;
  }
}

void MiLightHttpServer::handlePacketSent(const uint8_t *packet, const MiLightRemoteConfig& config, const BulbId& bulbId, const JsonObject& result) {
  if (numWsClients > 0) {
    DynamicJsonDocument output(1024);

    output[F("t")] = F("packet");
    if (!output[F("u")].set(result)) {
      Serial.println(F("Failed to set packet sent result"));
    }

    const JsonObject device = output.createNestedObject(F("d"));
    device[F("di")] = bulbId.deviceId;
    device[F("gi")] = bulbId.groupId;
    device[F("rt")] = MiLightRemoteTypeHelpers::remoteTypeToString(bulbId.deviceType);

    const JsonArray responsePacket = output.createNestedArray(F("p"));
    for (size_t i = 0; i < config.packetFormatter->getPacketLength(); ++i) {
      if (!responsePacket.add(packet[i])) {
        Serial.println(F("Failed to add packet sent packet"));
      }
    }

    if (const GroupState* bulbState = this->stateStore->get(bulbId); bulbState != nullptr) {
      const JsonObject state = output.createNestedObject(F("s"));
      bulbState->applyState(state, bulbId, NORMALIZED_GROUP_STATE_FIELDS);
    }

    char responseBuffer[300];
    serializeJson(output, responseBuffer);
    wsServer.broadcastTXT(reinterpret_cast<uint8_t*>(responseBuffer));
  }
}

void MiLightHttpServer::handleServe_P(const char* data, const size_t length, const char* contentType) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "public, max-age=31536000");
  server.send(200, contentType);

  WiFiClient client = server.client();

  size_t remaining = length;
  while (remaining > 0) {
    constexpr size_t CHUNK_SIZE = 4096;
    const size_t chunk = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

    // Send chunk size in hexadecimal format
    client.printf("%X\r\n", chunk);

    // Send chunk data
    client.write_P(data, chunk);
    client.print("\r\n");

    data += chunk;
    remaining -= chunk;
  }

  // Send the terminal chunk
  client.print("0\r\n\r\n");
  client.stop();
}

void MiLightHttpServer::handleGetTransition(const RequestContext& request) const {
  const size_t id = atoi(request.pathVariables.get("id"));

  if (const auto transition = transitions.getTransition(id); transition == nullptr) {
    request.response.setCode(404);
    request.response.json["error"] = "Not found";
  } else {
    auto response = request.response.json.to<JsonObject>();
    transition->serialize(response);
  }
}

void MiLightHttpServer::handleDeleteTransition(const RequestContext& request) const {
  if (const size_t id = atoi(request.pathVariables.get("id")); transitions.deleteTransition(id)) {
    request.response.json["success"] = true;
  } else {
    request.response.setCode(404);
    request.response.json["error"] = "Not found";
  }
}

void MiLightHttpServer::handleListTransitions(const RequestContext& request) const {
  auto current = transitions.getTransitions();
  const JsonArray transitions = request.response.json.to<JsonObject>().createNestedArray(F("transitions"));

  while (current != nullptr) {
    JsonObject json = transitions.createNestedObject();
    current->data->serialize(json);
    current = current->next;
  }
}

void MiLightHttpServer::handleCreateTransition(RequestContext& request) const {
  const JsonObject body = request.getJsonBody().as<JsonObject>();

  if (! body.containsKey(GroupStateFieldNames::DEVICE_ID)
    || ! body.containsKey(GroupStateFieldNames::GROUP_ID)
    || (!body.containsKey(F("remote_type")) && !body.containsKey(GroupStateFieldNames::DEVICE_TYPE))) {
    char buffer[200];
    sprintf_P(buffer, PSTR("Must specify required keys: device_id, group_id, device_type"));

    request.response.setCode(400);
    request.response.json[F("error")] = buffer;
    return;
  }

  const String _deviceId = body[GroupStateFieldNames::DEVICE_ID];
  const uint8_t _groupId = body[GroupStateFieldNames::GROUP_ID];
  const MiLightRemoteConfig* _remoteType = nullptr;

  if (body.containsKey(GroupStateFieldNames::DEVICE_TYPE)) {
    _remoteType = MiLightRemoteConfig::fromType(body[GroupStateFieldNames::DEVICE_TYPE].as<const char*>());
  } else if (body.containsKey(F("remote_type"))) {
    _remoteType = MiLightRemoteConfig::fromType(body[F("remote_type")].as<const char*>());
  }

  if (_remoteType == nullptr) {
    char buffer[40];
    sprintf_P(buffer, PSTR("Unknown device type\n"));
    request.response.setCode(400);
    request.response.json[F("error")] = buffer;
    return;
  }

  milightClient->prepare(_remoteType, parseInt<uint16_t>(_deviceId), _groupId);

  if (milightClient->handleTransition(request.getJsonBody().as<JsonObject>(), request.response.json)) {
    request.response.json[F("success")] = true;
  } else {
    request.response.setCode(400);
  }
}

void MiLightHttpServer::handleListAliases(const RequestContext& request) const {
  const uint8_t page = request.server.hasArg("page") ? request.server.arg("page").toInt() : 1;

  // at least 1 per page
  uint8_t perPage = request.server.hasArg("page_size") ? request.server.arg("page_size").toInt() : DEFAULT_PAGE_SIZE;
  perPage = perPage > 0 ? perPage : 1;

  const uint8_t numPages = settings.groupIdAliases.empty() ? 1 : ceil(settings.groupIdAliases.size() / (float) perPage);

  // check bounds
  if (page < 1 || page > numPages) {
    request.response.setCode(404);
    request.response.json[F("error")] = F("Page out of bounds");
    request.response.json[F("page")] = page;
    request.response.json[F("num_pages")] = numPages;
    return;
  }

  const JsonArray aliases = request.response.json.to<JsonObject>().createNestedArray(F("aliases"));
  request.response.json[F("page")] = page;
  request.response.json[F("count")] = settings.groupIdAliases.size();
  request.response.json[F("num_pages")] = numPages;

  // Skip iterator to start of page
  auto it = settings.groupIdAliases.begin();
  std::advance(it, (page - 1) * perPage);

  for (size_t i = 0; i < perPage && it != settings.groupIdAliases.end(); i++, it++) {
    JsonObject alias = aliases.createNestedObject();
    alias[F("alias")] = it->first;
    alias[F("id")] = it->second.id;

    const BulbId& bulbId = it->second.bulbId;
    alias[F("device_id")] = bulbId.deviceId;
    alias[F("group_id")] = bulbId.groupId;
    alias[F("device_type")] = MiLightRemoteTypeHelpers::remoteTypeToString(bulbId.deviceType);

  }
}

void MiLightHttpServer::handleCreateAlias(RequestContext& request) const {
  const JsonObject body = request.getJsonBody().as<JsonObject>();

  if (! body.containsKey(F("alias"))
    || ! body.containsKey(GroupStateFieldNames::DEVICE_ID)
    || ! body.containsKey(GroupStateFieldNames::GROUP_ID)
    || ! body.containsKey(GroupStateFieldNames::DEVICE_TYPE)) {
    char buffer[200];
    sprintf_P(buffer, PSTR("Must specify required keys: alias, device_id, group_id, device_type"));

    request.response.setCode(400);
    request.response.json[F("error")] = buffer;
    return;
  }

  const String alias = body[F("alias")];
  const uint16_t deviceId = body[GroupStateFieldNames::DEVICE_ID];
  const uint8_t groupId = body[GroupStateFieldNames::GROUP_ID];
  const MiLightRemoteType deviceType = MiLightRemoteTypeHelpers::remoteTypeFromString(body[GroupStateFieldNames::DEVICE_TYPE].as<const char*>());

  if (settings.groupIdAliases.find(alias) != settings.groupIdAliases.end()) {
    char buffer[200];
    sprintf_P(buffer, PSTR("Alias already exists: %s"), alias.c_str());

    request.response.setCode(400);
    request.response.json[F("error")] = buffer;
    return;
  }

  settings.addAlias(alias.c_str(), BulbId(deviceId, groupId, deviceType));
  saveSettings();

  request.response.json[F("success")] = true;
  request.response.json[F("id")] = settings.groupIdAliases[alias].id;
}

void MiLightHttpServer::handleDeleteAlias(const RequestContext& request) const {
  if (const size_t id = atoi(request.pathVariables.get("id")); settings.deleteAlias(id)) {
    saveSettings();
    request.response.json[F("success")] = true;
  } else {
    request.response.setCode(404);
    request.response.json[F("error")] = F("Alias not found");
  }
}

void MiLightHttpServer::handleUpdateAlias(RequestContext& request) const {
  const size_t id = atoi(request.pathVariables.get("id"));

  if (const auto alias = settings.findAliasById(id); alias == settings.groupIdAliases.end()) {
    request.response.setCode(404);
    request.response.json[F("error")] = F("Alias not found");
  } else {
    const JsonObject body = request.getJsonBody().as<JsonObject>();
    GroupAlias updatedAlias(alias->second);

    if (body.containsKey(F("alias"))) {
      strncpy(updatedAlias.alias, body[F("alias")].as<const char*>(), MAX_ALIAS_LEN);
    }

    if (body.containsKey(GroupStateFieldNames::DEVICE_ID)) {
      updatedAlias.bulbId.deviceId = body[GroupStateFieldNames::DEVICE_ID];
    }

    if (body.containsKey(GroupStateFieldNames::GROUP_ID)) {
      updatedAlias.bulbId.groupId = body[GroupStateFieldNames::GROUP_ID];
    }

    if (body.containsKey(GroupStateFieldNames::DEVICE_TYPE)) {
      updatedAlias.bulbId.deviceType = MiLightRemoteTypeHelpers::remoteTypeFromString(body[GroupStateFieldNames::DEVICE_TYPE].as<const char*>());
    }

    // If alias was updated, delete the old mapping
    if (strcmp(alias->second.alias, updatedAlias.alias) != 0) {
      settings.deleteAlias(id);
    }

    settings.groupIdAliases[updatedAlias.alias] = updatedAlias;
    saveSettings();

    request.response.json[F("success")] = true;
  }
}

void MiLightHttpServer::handleDeleteAliases(const RequestContext &request) const {
  // buffer current aliases so we can mark them all as deleted
  std::vector<GroupAlias> aliases;
  for (auto &[fst, snd] : settings.groupIdAliases) {
    aliases.push_back(snd);
  }

  ProjectFS.remove(ALIASES_FILE);
  Settings::load(settings);

  // mark all aliases as deleted
  for (auto & alias : aliases) {
    settings.deletedGroupIdAliases[alias.bulbId.getCompactId()] = alias.bulbId;
  }

  if (this->settingsSavedHandler) {
    this->settingsSavedHandler();
  }

  request.response.json[F("success")] = true;
}

void MiLightHttpServer::handleUpdateAliases(const RequestContext& request) const {
  // buffer current aliases so we can mark any that were removed as deleted
  std::vector<GroupAlias> aliases;
  for (auto &[fst, snd] : settings.groupIdAliases) {
    aliases.push_back(snd);
  }

  Settings::load(settings);

  // mark any aliases that were removed as deleted
  for (auto & alias : aliases) {
    if (settings.groupIdAliases.find(alias.alias) == settings.groupIdAliases.end()) {
      settings.deletedGroupIdAliases[alias.bulbId.getCompactId()] = alias.bulbId;
    }
  }

  saveSettings();

  request.response.json[F("success")] = true;
}

void MiLightHttpServer::saveSettings() const {
  settings.save();

  if (this->settingsSavedHandler) {
    this->settingsSavedHandler();
  }
}

void MiLightHttpServer::handleRestoreBackup(const RequestContext &request) const {
  File backupFile = ProjectFS.open(BACKUP_FILE, "r");

  if (auto status = BackupManager::restoreBackup(settings, backupFile); status == BackupManager::RestoreStatus::OK) {
    request.response.json[F("success")] = true;
    request.response.json[F("message")] = F("Backup restored successfully");
  } else {
    request.response.setCode(400);
    request.response.json[F("error")] = static_cast<uint8_t>(status);
  }
}

void MiLightHttpServer::handleCreateBackup(const RequestContext &request) {
  File backupFile = ProjectFS.open(BACKUP_FILE, "w");

  if (!backupFile) {
    Serial.println(F("Failed to open backup file"));
    request.response.setCode(500);
    request.response.json[F("error")] = F("Failed to open backup file");
  }

  WriteBufferingStream bufferedStream(backupFile, 64);
  BackupManager::createBackup(settings, bufferedStream);
  bufferedStream.flush();
  backupFile.close();

  backupFile = ProjectFS.open(BACKUP_FILE, "r");
  Serial.printf_P(PSTR("Sending backup file of size %d\n"), backupFile.size());
  server.streamFile(backupFile, APPLICATION_OCTET_STREAM);

  ProjectFS.remove(BACKUP_FILE);
}


void MiLightHttpServer::handleListGroups() {
  this->stateStore->flush();

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json");

  StaticJsonDocument<1024> stateBuffer;
  WiFiClient client = server.client();

  // open array
  server.sendContent("[");

  bool firstGroup = true;
  for (auto &[fst, snd] : settings.groupIdAliases) {
    stateBuffer.clear();

    JsonObject device = stateBuffer.createNestedObject(F("device"));

    device[F("alias")] = fst;
    device[F("id")] = snd.id;
    device[F("device_id")] = snd.bulbId.deviceId;
    device[F("group_id")] = snd.bulbId.groupId;
    device[F("device_type")] = MiLightRemoteTypeHelpers::remoteTypeToString(snd.bulbId.deviceType);

    const GroupState* state = this->stateStore->get(snd.bulbId);
    const JsonObject outputState = stateBuffer.createNestedObject(F("state"));

    if (state != nullptr) {
      state->applyState(outputState, snd.bulbId, NORMALIZED_GROUP_STATE_FIELDS);
    }

    client.printf("%zx\r\n", measureJson(stateBuffer)+(firstGroup ? 0 : 1));

    if (!firstGroup) {
      client.print(',');
    }
    serializeJson(stateBuffer, client);
    client.printf_P(PSTR("\r\n"));

    firstGroup = false;
    yield();
  }

  // close array
  server.sendContent("]");

  // stop chunked streaming
  server.sendContent("");
  server.client().stop();
}

void MiLightHttpServer::handleBatchUpdateGroups(RequestContext& request) const {
  const JsonArray body = request.getJsonBody().as<JsonArray>();

  if (body.size() == 0) {
    request.response.setCode(400);
    request.response.json[F("error")] = F("Must specify an array of gateways and updates");
    return;
  }

  for (auto update : body) {
    auto gateways = update[F("gateways")].as<JsonArray>();
    auto stateUpdate = update[F("update")].as<JsonObject>();

    for (auto gateway : gateways) {
      const BulbId bulbId(
        gateway[F("device_id")],
        gateway[F("group_id")],
        MiLightRemoteTypeHelpers::remoteTypeFromString(gateway[F("device_type")].as<const char*>())
      );

      this->milightClient->prepare(
        bulbId.deviceType,
        bulbId.deviceId,
        bulbId.groupId
      );
      handleRequest(stateUpdate);
    }
  }

  request.response.json[F("success")] = true;
}
