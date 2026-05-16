#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <LampTypes.h>

class SettingsManager {
public:
    SettingsManager();
    bool init();
    bool saveSettings(const String& settings);
    String loadSettings();
    String loadSettingsWithPresetSummary(const LampState presets[5][11]);
    bool settingsExist();
    void reboot();
    bool savePresets(const uint8_t* data, size_t size);
    void loadPresets(uint8_t* data, size_t size);

    static uint8_t lampAddr[11][9];

private:
    static const char* SETTINGS_FILE;
    bool formatFileSystem();
};

#endif
