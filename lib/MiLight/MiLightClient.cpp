#include <MiLightClient.h>
#include <Arduino.h>
#include <Units.h>
#include <TokenIterator.h>
#include <ParsedColor.h>
#include <MiLightCommands.h>
#include <functional>

using namespace std::placeholders;

static constexpr uint8_t STATUS_UNDEFINED = 255;

const char* MiLightClient::FIELD_ORDERINGS[] = {
  // These are handled manually
  // GroupStateFieldNames::STATE,
  // GroupStateFieldNames::STATUS,
  GroupStateFieldNames::HUE,
  GroupStateFieldNames::SATURATION,
  GroupStateFieldNames::KELVIN,
  GroupStateFieldNames::TEMPERATURE,
  GroupStateFieldNames::COLOR_TEMP,
  GroupStateFieldNames::MODE,
  GroupStateFieldNames::EFFECT,
  GroupStateFieldNames::COLOR,
  // Level/Brightness must be processed last because they're specific to a particular bulb mode.
  // So make sure bulb mode is set before applying level/brightness.
  GroupStateFieldNames::LEVEL,
  GroupStateFieldNames::BRIGHTNESS,
  GroupStateFieldNames::COMMAND,
  GroupStateFieldNames::COMMANDS
};

const std::map<const char*, std::function<void(MiLightClient*, JsonVariant)>, MiLightClient::cmp_str> MiLightClient::FIELD_SETTERS = {
  {
    GroupStateFieldNames::STATUS,
    [](const MiLightClient* client, const JsonVariant val) {
      client->updateStatus(parseMilightStatus(val));
    }
  },
  {GroupStateFieldNames::LEVEL, &MiLightClient::updateBrightness},
  {
    GroupStateFieldNames::BRIGHTNESS,
    [](const MiLightClient* client, const uint16_t arg) {
      client->updateBrightness(Units::rescale<uint16_t, uint16_t>(arg, 100, 255));
    }
  },
  {GroupStateFieldNames::HUE, &MiLightClient::updateHue},
  {GroupStateFieldNames::SATURATION, &MiLightClient::updateSaturation},
  {GroupStateFieldNames::KELVIN, &MiLightClient::updateTemperature},
  {GroupStateFieldNames::TEMPERATURE, &MiLightClient::updateTemperature},
  {
    GroupStateFieldNames::COLOR_TEMP,
    [](const MiLightClient* client, const uint16_t arg) {
      client->updateTemperature(Units::miredsToWhiteVal(arg, 100));
    }
  },
  {GroupStateFieldNames::MODE, &MiLightClient::updateMode},
  {GroupStateFieldNames::COLOR, &MiLightClient::updateColor},
  {GroupStateFieldNames::EFFECT, &MiLightClient::handleEffect},
  {GroupStateFieldNames::COMMAND, &MiLightClient::handleCommand},
  {GroupStateFieldNames::COMMANDS, &MiLightClient::handleCommands}
};

MiLightClient::MiLightClient(
  RadioSwitchboard& radioSwitchboard,
  PacketSender& packetSender,
  GroupStateStore* stateStore,
  Settings& settings,
  TransitionController& transitions
) : radioSwitchboard(radioSwitchboard)
    , currentRemote(nullptr)
    , updateBeginHandler(nullptr)
    , updateEndHandler(nullptr)
    , stateStore(stateStore)
    , currentState(nullptr), settings(settings)
    , packetSender(packetSender)
    , transitions(transitions)
    , repeatsOverride(0) {
}

void MiLightClient::setHeld(const bool held) const {
  currentRemote->packetFormatter->setHeld(held);
}

void MiLightClient::prepare(
  const MiLightRemoteConfig* remoteConfig,
  const uint16_t deviceId,
  const uint8_t groupId
) {
  this->currentRemote = remoteConfig;
  currentRemote->packetFormatter->prepare(deviceId, groupId);
  this->currentState = stateStore->get(deviceId, groupId, remoteConfig->type);
}

void MiLightClient::prepare(
  const MiLightRemoteType type,
  const uint16_t deviceId,
  const uint8_t groupId
) {
  prepare(MiLightRemoteConfig::fromType(type), deviceId, groupId);
}

void MiLightClient::updateColorRaw(const uint8_t color) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateColorRaw: Change color to %d\n"), color);
#endif
  currentRemote->packetFormatter->updateColorRaw(color);
  flushPacket();
}

void MiLightClient::updateHue(const uint16_t hue) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateHue: Change hue to %d\n"), hue);
#endif
  currentRemote->packetFormatter->updateHue(hue);
  flushPacket();
}

void MiLightClient::updateBrightness(const uint8_t brightness) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateBrightness: Change brightness to %d\n"), brightness);
#endif
  currentRemote->packetFormatter->updateBrightness(brightness);
  flushPacket();
}

void MiLightClient::updateMode(uint8_t mode) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateMode: Change mode to %d\n"), mode);
#endif
  currentRemote->packetFormatter->updateMode(mode);
  flushPacket();
}

void MiLightClient::nextMode() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::nextMode: Switch to next mode"));
#endif
  currentRemote->packetFormatter->nextMode();
  flushPacket();
}

void MiLightClient::previousMode() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::previousMode: Switch to previous mode"));
#endif
  currentRemote->packetFormatter->previousMode();
  flushPacket();
}

void MiLightClient::modeSpeedDown() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::modeSpeedDown: Speed down\n"));
#endif
  currentRemote->packetFormatter->modeSpeedDown();
  flushPacket();
}
void MiLightClient::modeSpeedUp() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::modeSpeedUp: Speed up"));
#endif
  currentRemote->packetFormatter->modeSpeedUp();
  flushPacket();
}

void MiLightClient::updateStatus(MiLightStatus status, uint8_t groupId) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateStatus: Status %s, groupId %d\n"), status == MiLightStatus::OFF ? "OFF" : "ON", groupId);
#endif
  currentRemote->packetFormatter->updateStatus(status, groupId);
  flushPacket();
}

void MiLightClient::updateStatus(MiLightStatus status) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateStatus: Status %s\n"), status == MiLightStatus::OFF ? "OFF" : "ON");
#endif
  currentRemote->packetFormatter->updateStatus(status);
  flushPacket();
}

void MiLightClient::updateSaturation(const uint8_t value) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateSaturation: Saturation %d\n"), value);
#endif
  currentRemote->packetFormatter->updateSaturation(value);
  flushPacket();
}

void MiLightClient::updateColorWhite() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::updateColorWhite: Color white"));
#endif
  currentRemote->packetFormatter->updateColorWhite();
  flushPacket();
}

void MiLightClient::enableNightMode() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::enableNightMode: Night mode"));
#endif
  currentRemote->packetFormatter->enableNightMode();
  flushPacket();
}

void MiLightClient::pair() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::pair: Pair"));
#endif
  currentRemote->packetFormatter->pair();
  flushPacket();
}

void MiLightClient::unpair() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::unpair: Unpair"));
#endif
  currentRemote->packetFormatter->unpair();
  flushPacket();
}

void MiLightClient::increaseBrightness() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::increaseBrightness: Increase brightness"));
#endif
  currentRemote->packetFormatter->increaseBrightness();
  flushPacket();
}

void MiLightClient::decreaseBrightness() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::decreaseBrightness: Decrease brightness"));
#endif
  currentRemote->packetFormatter->decreaseBrightness();
  flushPacket();
}

void MiLightClient::increaseTemperature() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::increaseTemperature: Increase temperature"));
#endif
  currentRemote->packetFormatter->increaseTemperature();
  flushPacket();
}

void MiLightClient::decreaseTemperature() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::decreaseTemperature: Decrease temperature"));
#endif
  currentRemote->packetFormatter->decreaseTemperature();
  flushPacket();
}

void MiLightClient::updateTemperature(const uint8_t temperature) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateTemperature: Set temperature to %d\n"), temperature);
#endif
  currentRemote->packetFormatter->updateTemperature(temperature);
  flushPacket();
}

void MiLightClient::command(uint8_t command, uint8_t arg) const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::command: Execute command %d, argument %d\n"), command, arg);
#endif
  currentRemote->packetFormatter->command(command, arg);
  flushPacket();
}

void MiLightClient::toggleStatus() const {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::toggleStatus"));
#endif
  currentRemote->packetFormatter->toggleStatus();
  flushPacket();
}

void MiLightClient::updateColor(const JsonVariant json) const {
  const ParsedColor color = ParsedColor::fromJson(json);

  if (!color.success) {
    Serial.println(F("Error parsing color field, unrecognized format"));
    return;
  }

  // We consider an RGB color "white" if all color intensities are roughly the
  // same value.  An unscientific value of 10 (~4%) is chosen.
  if ( abs(color.r - color.g) < RGB_WHITE_THRESHOLD
    && abs(color.g - color.b) < RGB_WHITE_THRESHOLD
    && abs(color.r - color.b) < RGB_WHITE_THRESHOLD) {
      this->updateColorWhite();
  } else {
    this->updateHue(color.hue);
    this->updateSaturation(color.saturation);
  }
}

void MiLightClient::update(const JsonObject object) {
  if (this->updateBeginHandler) {
    this->updateBeginHandler();
  }

  const JsonVariant status = this->extractStatus(object);
  const uint8_t parsedStatus = this->parseStatus(status);
  const JsonVariant jsonTransition = object[RequestKeys::TRANSITION];
  float transition = 0;

  if (!jsonTransition.isNull()) {
    if (jsonTransition.is<float>()) {
      transition = jsonTransition.as<float>();
    } else if (jsonTransition.is<size_t>()) {
      transition = jsonTransition.as<size_t>();
    } else {
      Serial.println(F("MiLightClient - WARN: unsupported transition type.  Must be float or int."));
    }
  }

  const JsonVariant brightness = object[GroupStateFieldNames::BRIGHTNESS];
  const JsonVariant level = object[GroupStateFieldNames::LEVEL];
  const bool isBrightnessDefined = !brightness.isNull() || !level.isNull();

  // Always turn on first
  if (parsedStatus == ON) {
    if (transition == 0) {
      this->updateStatus(ON);
    }
    // Don't do an "On" transition if the bulb is already on.  The reasons for this are:
    //   * Ambiguous what the behavior should be.  Should it ramp to full brightness?
    //   * HomeAssistant is only capable of sending transitions via the `light.turn_on`
    //     service call, which ends up sending `{"status":"ON"}`.  So transitions which
    //     have nothing to do with the status will include an "ON" command.
    // If the user wants to transition brightness, they can just specify a brightness in
    // the same command.  This avoids the need to make arbitrary calls on what the
    // behavior should be.
    else if (!currentState->isSetState() || !currentState->isOn()) {
      // If brightness is defined, we'll want to transition to that.  Status
      // transitions only ramp up/down to the max/min. Otherwise, turn the bulb on
      // and let field transitions handle the rest.
      if (!isBrightnessDefined) {
        handleTransition(GroupStateField::STATUS, status, transition, 0);
      } else {
        this->updateStatus(ON);

        if (! brightness.isNull()) {
          handleTransition(GroupStateField::BRIGHTNESS, brightness, transition, 0);
        } else if (! level.isNull()) {
          handleTransition(GroupStateField::LEVEL, level, transition, 0);
        }
      }
    }
  }

  for (const char* fieldName : FIELD_ORDERINGS) {
    if (object.containsKey(fieldName)) {
      auto handler = FIELD_SETTERS.find(fieldName);
      const JsonVariant value = object[fieldName];

      if (handler != FIELD_SETTERS.end()) {
        // No transition -- set the field directly
        if (transition == 0) {
          handler->second(this, value);
        } else {
          const GroupStateField field = GroupStateFieldHelpers::getFieldByName(fieldName);

          if (   !GroupStateFieldHelpers::isBrightnessField(field)  // If the field isn't brightness
               || parsedStatus == STATUS_UNDEFINED                  // or if there was not a status field
               || currentState->isOn()                              // or if the bulb was already on
          ) {
            handleTransition(field, value, transition);
          }
        }
      }
    }
  }

  // Raw packet command/args
  if (object.containsKey("button_id") && object.containsKey("argument")) {
    this->command(object["button_id"], object["argument"]);
  }

  // Always turn off last
  if (parsedStatus == OFF) {
    if (transition == 0) {
      this->updateStatus(OFF);
    } else {
      handleTransition(GroupStateField::STATUS, status, transition);
    }
  }

  if (this->updateEndHandler) {
    this->updateEndHandler();
  }
}

void MiLightClient::handleCommands(const JsonArray commands) const {
  if (! commands.isNull()) {
    for (size_t i = 0; i < commands.size(); i++) {
      this->handleCommand(commands[i]);
    }
  }
}

void MiLightClient::handleCommand(const JsonVariant command) const {
  String cmdName;
  JsonObject args;

  if (command.is<JsonObject>()) {
    const JsonObject cmdObj = command.as<JsonObject>();
    cmdName = cmdObj[GroupStateFieldNames::COMMAND].as<const char*>();
    args = cmdObj["args"];
  } else if (command.is<const char*>()) {
    cmdName = command.as<const char*>();
  }

  if (cmdName == MiLightCommandNames::UNPAIR) {
    this->unpair();
  } else if (cmdName == MiLightCommandNames::PAIR) {
    this->pair();
  } else if (cmdName == MiLightCommandNames::SET_WHITE) {
    this->updateColorWhite();
  } else if (cmdName == MiLightCommandNames::NIGHT_MODE) {
    this->enableNightMode();
  } else if (cmdName == MiLightCommandNames::LEVEL_UP) {
    this->increaseBrightness();
  } else if (cmdName == MiLightCommandNames::LEVEL_DOWN) {
    this->decreaseBrightness();
  } else if (cmdName == "brightness_up") {
    this->increaseBrightness();
  } else if (cmdName == "brightness_down") {
    this->decreaseBrightness();
  } else if (cmdName == MiLightCommandNames::TEMPERATURE_UP) {
    this->increaseTemperature();
  } else if (cmdName == MiLightCommandNames::TEMPERATURE_DOWN) {
    this->decreaseTemperature();
  } else if (cmdName == MiLightCommandNames::NEXT_MODE) {
    this->nextMode();
  } else if (cmdName == MiLightCommandNames::PREVIOUS_MODE) {
    this->previousMode();
  } else if (cmdName == MiLightCommandNames::MODE_SPEED_DOWN) {
    this->modeSpeedDown();
  } else if (cmdName == MiLightCommandNames::MODE_SPEED_UP) {
    this->modeSpeedUp();
  } else if (cmdName == MiLightCommandNames::TOGGLE) {
    this->toggleStatus();
  } else if (cmdName == MiLightCommandNames::TRANSITION) {
    StaticJsonDocument<100> obj;
    this->handleTransition(args, obj);
  }
}

void MiLightClient::handleTransition(const GroupStateField field, const JsonVariant value, const float duration, const int16_t startValue) const {
  const BulbId bulbId = currentRemote->packetFormatter->currentBulbId();
  std::shared_ptr<Transition::Builder> transitionBuilder = nullptr;

  if (currentState == nullptr) {
    Serial.println(F("Error planning transition: could not find current bulb state."));
    return;
  }

  if (!currentState->isSetField(field)) {
    Serial.println(F("Error planning transition: current state for field could not be determined"));
    return;
  }

  if (field == GroupStateField::COLOR) {
    const ParsedColor currentColor = currentState->getColor();
    const ParsedColor endColor = ParsedColor::fromJson(value);

    transitionBuilder = transitions.buildColorTransition(
      bulbId,
      currentColor,
      endColor
    );
  } else if (field == GroupStateField::STATUS || field == GroupStateField::STATE) {
    uint8_t startLevel;
    const MiLightStatus status = parseMilightStatus(value);

    if (startValue == FETCH_VALUE_FROM_STATE || currentState->isOn()) {
      startLevel = currentState->getBrightness();
    } else {
      startLevel = startValue;
    }

    transitionBuilder = transitions.buildStatusTransition(bulbId, status, startLevel);
  } else {
    uint16_t currentValue;
    const uint16_t endValue = value;

    if (startValue == FETCH_VALUE_FROM_STATE || currentState->isOn()) {
      currentValue = currentState->getParsedFieldValue(field);
    } else {
      currentValue = startValue;
    }

    transitionBuilder = transitions.buildFieldTransition(
      bulbId,
      field,
      currentValue,
      endValue
    );
  }

  if (transitionBuilder == nullptr) {
    Serial.printf_P(PSTR("Unsupported transition field: %s\n"), GroupStateFieldHelpers::getFieldName(field));
    return;
  }

  transitionBuilder->setDuration(duration);
  transitions.addTransition(transitionBuilder->build());
}

bool MiLightClient::handleTransition(const JsonObject args, JsonDocument& responseObj) const {
  if (! args.containsKey(FPSTR(TransitionParams::FIELD))
    || ! args.containsKey(FPSTR(TransitionParams::END_VALUE))) {
    responseObj[F("error")] = F("Ignoring transition missing required arguments");
    return false;
  }

  const BulbId& bulbId = currentRemote->packetFormatter->currentBulbId();
  const char* fieldName = args[FPSTR(TransitionParams::FIELD)];
  const JsonVariant startValue = args[FPSTR(TransitionParams::START_VALUE)];
  const JsonVariant endValue = args[FPSTR(TransitionParams::END_VALUE)];
  const GroupStateField field = GroupStateFieldHelpers::getFieldByName(fieldName);
  std::shared_ptr<Transition::Builder> transitionBuilder = nullptr;

  if (field == GroupStateField::UNKNOWN) {
    char errorMsg[30];
    sprintf_P(errorMsg, PSTR("Unknown transition field: %s\n"), fieldName);
    responseObj[F("error")] = errorMsg;
    return false;
  }

  // These fields can be transitioned directly.
  switch (field) {
    case GroupStateField::HUE:
    case GroupStateField::SATURATION:
    case GroupStateField::BRIGHTNESS:
    case GroupStateField::LEVEL:
    case GroupStateField::KELVIN:
    case GroupStateField::COLOR_TEMP:

      transitionBuilder = transitions.buildFieldTransition(
        bulbId,
        field,
        startValue.isNull()
          ? currentState->getParsedFieldValue(field)
          : startValue.as<uint16_t>(),
        endValue
      );
      break;

    default:
      break;
  }

  // Color can be decomposed into hue/saturation, and these can be transitioned separately
  if (field == GroupStateField::COLOR) {
    const ParsedColor _startValue = startValue.isNull()
      ? currentState->getColor()
      : ParsedColor::fromJson(startValue);
    const ParsedColor endColor = ParsedColor::fromJson(endValue);

    if (! _startValue.success) {
      responseObj[F("error")] = F("Transition - error parsing start color");
      return false;
    }
    if (! endColor.success) {
      responseObj[F("error")] = F("Transition - error parsing end color");
      return false;
    }

    transitionBuilder = transitions.buildColorTransition(
      bulbId,
      _startValue,
      endColor
    );
  }

  // Status is handled a little differently
  if (field == GroupStateField::STATUS || field == GroupStateField::STATE) {
    const MiLightStatus toStatus = parseMilightStatus(endValue);
    uint8_t startLevel;
    if (currentState->isSetBrightness()) {
      startLevel = currentState->getBrightness();
    } else if (toStatus == ON) {
      startLevel = 0;
    } else {
      startLevel = 100;
    }

    transitionBuilder = transitions.buildStatusTransition(bulbId, toStatus, startLevel);
  }

  if (transitionBuilder == nullptr) {
    char errorMsg[30];
    sprintf_P(errorMsg, PSTR("Recognized, but unsupported transition field: %s\n"), fieldName);
    responseObj[F("error")] = errorMsg;
    return false;
  }

  if (args.containsKey(FPSTR(TransitionParams::DURATION))) {
    transitionBuilder->setDuration(args[FPSTR(TransitionParams::DURATION)]);
  }
  if (args.containsKey(FPSTR(TransitionParams::PERIOD))) {
    transitionBuilder->setPeriod(args[FPSTR(TransitionParams::PERIOD)]);
  }

  transitions.addTransition(transitionBuilder->build());
  return true;
}

void MiLightClient::handleEffect(const String& effect) const {
  if (effect == MiLightCommandNames::NIGHT_MODE) {
    this->enableNightMode();
  } else if (effect == "white" || effect == "white_mode") {
    this->updateColorWhite();
  } else { // assume we're trying to set mode
    this->updateMode(effect.toInt());
  }
}

JsonVariant MiLightClient::extractStatus(const JsonObject object) {
  if (object.containsKey(FPSTR(GroupStateFieldNames::STATUS))) {
    return object[FPSTR(GroupStateFieldNames::STATUS)];
  }
  return object[FPSTR(GroupStateFieldNames::STATE)];
}

uint8_t MiLightClient::parseStatus(const JsonVariant object) {
  if (object.isNull()) {
    return STATUS_UNDEFINED;
  }

  return parseMilightStatus(object);
}

void MiLightClient::setRepeatsOverride(const size_t repeatsOverride) {
  this->repeatsOverride = repeatsOverride;
}

void MiLightClient::clearRepeatsOverride() {
  this->repeatsOverride = PacketSender::DEFAULT_PACKET_SENDS_VALUE;
}

void MiLightClient::flushPacket() const {
  PacketStream& stream = currentRemote->packetFormatter->buildPackets();

  while (stream.hasNext()) {
    packetSender.enqueue(stream.next(), currentRemote, repeatsOverride);
  }

  currentRemote->packetFormatter->reset();
}

void MiLightClient::onUpdateBegin(const EventHandler &handler) {
  this->updateBeginHandler = handler;
}

void MiLightClient::onUpdateEnd(const EventHandler &handler) {
  this->updateEndHandler = handler;
}

size_t MiLightClient::getNumRadios() const {
  return this->radioSwitchboard.getNumRadios();
}

std::shared_ptr<MiLightRadio> MiLightClient::switchRadio(const size_t radioIx) const {
  return this->radioSwitchboard.switchRadio(radioIx);
}

std::shared_ptr<MiLightRadio> MiLightClient::switchRadio(const MiLightRemoteConfig *remoteConfig) const {
  return this->radioSwitchboard.switchRadio(remoteConfig);
}
