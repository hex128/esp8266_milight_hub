#pragma once

#ifndef ARDUINO
#include <stdint.h>
#include <stdlib.h>
#endif

#include <MiLightRadioConfig.h>

class MiLightRadio {
  public:
    virtual ~MiLightRadio() = default;

    virtual int begin() = 0;
    virtual bool available() = 0;
    virtual int read(uint8_t frame[], size_t &frame_length) = 0;
    virtual size_t write(uint8_t frame[], size_t frame_length) = 0;
    virtual int resend() = 0;
    virtual int configure() = 0;
    virtual const MiLightRadioConfig& config() = 0;

};
