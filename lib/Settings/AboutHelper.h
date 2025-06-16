#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class AboutHelper {
public:
  static String generateAboutString(bool abbreviated = false);
  static void generateAboutObject(JsonDocument& obj, bool abbreviated = false);
};
