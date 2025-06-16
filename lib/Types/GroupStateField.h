#ifndef _GROUP_STATE_FIELDS_H
#define _GROUP_STATE_FIELDS_H

namespace GroupStateFieldNames {
  static constexpr char UNKNOWN[] = "unknown";
  static constexpr char STATE[] = "state";
  static constexpr char STATUS[] = "status";
  static constexpr char BRIGHTNESS[] = "brightness";
  static constexpr char LEVEL[] = "level";
  static constexpr char HUE[] = "hue";
  static constexpr char SATURATION[] = "saturation";
  static constexpr char COLOR[] = "color";
  static constexpr char MODE[] = "mode";
  static constexpr char KELVIN[] = "kelvin";
  static constexpr char TEMPERATURE[] = "temperature"; //alias for kelvin
  static constexpr char COLOR_TEMP[] = "color_temp";
  static constexpr char BULB_MODE[] = "bulb_mode";
  static constexpr char COMPUTED_COLOR[] = "computed_color";
  static constexpr char EFFECT[] = "effect";
  static constexpr char DEVICE_ID[] = "device_id";
  static constexpr char GROUP_ID[] = "group_id";
  static constexpr char DEVICE_TYPE[] = "device_type";
  static constexpr char OH_COLOR[] = "oh_color";
  static constexpr char HEX_COLOR[] = "hex_color";
  static constexpr char COMMAND[] = "command";
  static constexpr char COMMANDS[] = "commands";

  // For use with HomeAssistant
  static constexpr char COLOR_MODE[] = "color_mode";
};

enum class GroupStateField {
  UNKNOWN,
  STATE,
  STATUS,
  BRIGHTNESS,
  LEVEL,
  HUE,
  SATURATION,
  COLOR,
  MODE,
  KELVIN,
  COLOR_TEMP,
  BULB_MODE,
  COMPUTED_COLOR,
  EFFECT,
  DEVICE_ID,
  GROUP_ID,
  DEVICE_TYPE,
  OH_COLOR,
  HEX_COLOR,
  COLOR_MODE,
};

class GroupStateFieldHelpers {
public:
  static const char* getFieldName(GroupStateField field);
  static GroupStateField getFieldByName(const char* name);
  static bool isBrightnessField(GroupStateField field);
};

#endif
