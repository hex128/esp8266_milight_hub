#pragma once

#include <MiLightRadio.h>
#include <MiLightRemoteConfig.h>
#include <MiLightRadioConfig.h>
#include <MiLightRadioFactory.h>

class RadioSwitchboard {
public:
  RadioSwitchboard(
    const std::shared_ptr<MiLightRadioFactory> &radioFactory,
    GroupStateStore* stateStore,
    const Settings& settings
  );

  std::shared_ptr<MiLightRadio> switchRadio(const MiLightRemoteConfig* remote);
  std::shared_ptr<MiLightRadio> switchRadio(size_t index);
  size_t getNumRadios() const;

  bool available() const;
  void write(uint8_t* packet, size_t len) const;
  size_t read(uint8_t* packet) const;

private:
  std::vector<std::shared_ptr<MiLightRadio>> radios;
  std::shared_ptr<MiLightRadio> currentRadio;
};
