#pragma once

#include <Settings.h>

class BackupManager {
public:
    // last byte stores version
    static const uint8_t SETTINGS_BACKUP_VERSION;
    static const uint32_t SETTINGS_MAGIC_HEADER;

    enum class RestoreStatus {
        OK,
        INVALID_HEADER,
        INVALID_VERSION,
        INVALID_FILE,
        INVALID_SIZE,
        INVALID_NUM_ALIASES,
        INVALID_ALIAS,
        INVALID_SETTINGS,
        UNKNOWN_ERROR
    };

    static void createBackup(const Settings& settings, Stream& stream);
    static RestoreStatus restoreBackup(Settings& settings, Stream& stream);
};
