#include <ESPId.h>

#ifdef ESP8266
uint32_t getESPId()
{
    return EspClass::getChipId();
}
#elif ESP32
uint32_t getESPId()
{
    uint32_t id = 0;
    for(int i=0; i<17; i=i+8) {
        id |= ((EspClass::getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return id;
}
#endif
