#pragma once

#include <V6CommandHandler.h>

enum RgbwCommandIds {
  V2_RGBW_COLOR_PREFIX      = 0x01,
  V2_RGBW_BRIGHTNESS_PREFIX = 0x02,
  V2_RGBW_COMMAND_PREFIX    = 0x03,
  V2_RGBW_MODE_PREFIX       = 0x04,

  V2_RGBW_ON                = 0x01,
  V2_RGBW_OFF               = 0x02,
  V2_RGBW_SPEED_DOWN        = 0x03,
  V2_RGBW_SPEED_UP          = 0x04,
  V2_RGBW_WHITE_ON          = 0x05,
  V2_RGBW_NIGHT_LIGHT       = 0x06
};

class V6RgbwCommandHandler final : public V6CommandHandler {
public:
  V6RgbwCommandHandler()
    : V6CommandHandler(0x0700, FUT096Config)
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
