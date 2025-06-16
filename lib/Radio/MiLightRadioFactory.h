#pragma once

#include <RF24.h>
#include <PL1167_nRF24.h>
#include <MiLightRadioConfig.h>
#include <MiLightRadio.h>
#include <NRF24MiLightRadio.h>
#include <LT8900MiLightRadio.h>
#include <RF24PowerLevel.h>
#include <RF24Channel.h>
#include <Settings.h>
#include <vector>
#include <memory>

class MiLightRadioFactory {
public:

  virtual ~MiLightRadioFactory() { };
  virtual std::shared_ptr<MiLightRadio> create(const MiLightRadioConfig& config) = 0;

  static std::shared_ptr<MiLightRadioFactory> fromSettings(const Settings& settings);

};

class NRF24Factory final : public MiLightRadioFactory {
public:

  NRF24Factory(
    uint8_t cePin,
    uint8_t csnPin,
    RF24PowerLevel rF24PowerLevel,
    const std::vector<RF24Channel>& channels,
    RF24Channel listenChannel
  );

  std::shared_ptr<MiLightRadio> create(const MiLightRadioConfig& config) override;

protected:

  RF24 rf24;
  const std::vector<RF24Channel>& channels;
  const RF24Channel listenChannel;

};

class LT8900Factory final : public MiLightRadioFactory {
public:

  LT8900Factory(uint8_t csPin, uint8_t resetPin, uint8_t pktFlag);

  std::shared_ptr<MiLightRadio> create(const MiLightRadioConfig& config) override;

protected:

  uint8_t _csPin;
  uint8_t _resetPin;
  uint8_t _pktFlag;

};
