#pragma once

#ifndef ARDUINO
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <RF24.h>
#include <PL1167_nRF24.h>
#include <MiLightRadioConfig.h>
#include <MiLightRadio.h>
#include <RF24Channel.h>
#include <vector>

class NRF24MiLightRadio final : public MiLightRadio {
  public:
    NRF24MiLightRadio(
      RF24& rf24,
      const MiLightRadioConfig& config, 
      const std::vector<RF24Channel>& channels, 
      RF24Channel listenChannel
    );

    int begin() override;
    bool available() override;
    int read(uint8_t frame[], size_t &frame_length) override;
    int dupesReceived();
    size_t write(uint8_t frame[], size_t frame_length) override;
    int resend() override;
    int configure() override;
    const MiLightRadioConfig& config() override;

  private:
    const std::vector<RF24Channel>& channels;
    const size_t listenChannelIx;

    PL1167_nRF24 _pl1167;
    const MiLightRadioConfig& _config;
    uint32_t _prev_packet_id;

    uint8_t _packet[10];
    uint8_t _out_packet[10];
    bool _waiting;
    int _dupes_received;
};
