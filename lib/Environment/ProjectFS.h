#pragma once

#ifdef MILIGHT_USE_LITTLE_FS
#include <LittleFS.h>
#define ProjectFS LittleFS
#else
#include <FS.h>
#define ProjectFS SPIFFS
#endif
