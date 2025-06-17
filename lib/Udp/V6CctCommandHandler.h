#pragma once

#include <V6CommandHandler.h>

enum CctCommandIds {
  V2_CCT_COMMAND_PREFIX   = 0x01,

  V2_CCT_BRIGHTNESS_UP    = 0x01,
  V2_CCT_BRIGHTNESS_DOWN  = 0x02,
  V2_CCT_TEMPERATURE_UP   = 0x03,
  V2_CCT_TEMPERATURE_DOWN = 0x04,
  V2_CCT_NIGHT_LIGHT      = 0x06,
  V2_CCT_ON               = 0x07,
  V2_CCT_OFF              = 0x08
};

class V6CctCommandHandler final : public V6CommandHandler {
public:
  V6CctCommandHandler()
    : V6CommandHandler(0x0100, FUT007Config)
  { }

  bool handleCommand(
    MiLightClient* client,
    uint32_t command,
    uint32_t commandArg
  ) override;

  bool handlePreset(
    MiLightClient* client,
    uint8_t commandLsb,
    uint32_t commandArg
  ) override;

};
