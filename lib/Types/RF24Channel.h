#pragma once

#include <Arduino.h>
#include <vector>

enum class RF24Channel {
  RF24_LOW = 0,
  RF24_MID = 1,
  RF24_HIGH = 2
};

class RF24ChannelHelpers {
public:
  static String nameFromValue(const RF24Channel& value);
  static RF24Channel valueFromName(const String& name);
  static RF24Channel defaultValue();
  static std::vector<RF24Channel> allValues();
};
