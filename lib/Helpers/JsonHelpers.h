#pragma once

#include <ArduinoJson.h>
#include <vector>
#include <functional>
#include <algorithm>

class JsonHelpers {
public:
  template<typename T>
  static void copyFrom(JsonArray arr, std::vector<T> vec) {
    for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
      arr.add(*it);
    }
  }

  template<typename T>
  static void copyTo(const JsonArray arr, std::vector<T> vec) {
    for (auto && i : arr) {
      JsonVariant val = i;
      vec.push_back(val.as<T>());
    }
  }

  template<typename T, typename StrType>
  static std::vector<T> jsonArrToVector(JsonArray& arr, std::function<T (const StrType)> converter, const bool unique = true) {
    std::vector<T> vec;

    for (auto strVal : arr) {
      // inefficient, but everything using this is tiny, so doesn't matter
      if (T convertedVal = converter(strVal); !unique || std::find(vec.begin(), vec.end(), convertedVal) == vec.end()) {
        vec.push_back(convertedVal);
      }
    }

    return vec;
  }

  template<typename T, typename StrType>
  static void vectorToJsonArr(JsonArray& arr, const std::vector<T>& vec, std::function<StrType (const T&)> converter) {
    for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
      arr.add(converter(*it));
    }
  }
};
