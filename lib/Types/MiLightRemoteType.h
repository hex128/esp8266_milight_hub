#pragma once

#include <Arduino.h>

enum MiLightRemoteType {
  REMOTE_TYPE_UNKNOWN = 255,
  REMOTE_TYPE_RGBW    = 0,
  REMOTE_TYPE_CCT     = 1,
  REMOTE_TYPE_RGB_CCT = 2,
  REMOTE_TYPE_RGB     = 3,
  REMOTE_TYPE_FUT089  = 4,
  REMOTE_TYPE_FUT091  = 5,
  REMOTE_TYPE_FUT020  = 6
};

class MiLightRemoteTypeHelpers {
public:
  static MiLightRemoteType remoteTypeFromString(const String &type);
  static String remoteTypeToString(MiLightRemoteType type);
  static bool supportsRgb(MiLightRemoteType type);
  static bool supportsRgbw(MiLightRemoteType type);
  static bool supportsColorTemp(MiLightRemoteType type);
};
