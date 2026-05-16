#include "SettingsManager.h"
#include "../Debug/Debug.h"

const char* SettingsManager::SETTINGS_FILE = "/settings.json";

uint8_t SettingsManager::lampAddr[11][9] = {
    { 0x5B,0xC8,0x15,0x63, 0x6D,0x92,0xE5,0x0C, 0x11 },
    { 0xB0,0x56,0x86,0x01, 0x6D,0x92,0xE5,0x0C, 0x11 },
    { 0xD3,0x06,0x8E,0x43, 0x6D,0x92,0xE5,0x0C, 0x11 },
    { 0x04,0x11,0x68,0x2E, 0x6D,0x92,0xE5,0x0C, 0x11 },
    { 0x94,0x4B,0x2D,0x30, 0x6D,0x92,0xE5,0x0C, 0x11 },
    { 0x60,0xA7,0x3B,0x0A, 0x6D,0x92,0xE5,0x0C, 0x11 },

    { 0x1C,0x27,0x2F,0x1B, 0x78,0x53,0x51,0x2E, 0x11 },
    { 0xF8,0x6F,0x21,0x31, 0x78,0x53,0x51,0x2E, 0x11 },
    { 0x73,0x34,0x63,0x73, 0x78,0x53,0x51,0x2E, 0x11 },

    { 0x2F,0x06,0xE6,0x30, 0xAD,0xFA,0x3F,0x50, 0x11 },
    { 0x8B,0x73,0xA3,0x18, 0xAD,0xFA,0x3F,0x50, 0x11 }
};

SettingsManager::SettingsManager() {
}

bool SettingsManager::init() {
    if (!SPIFFS.begin(true)) {
        Debug::println("[ERROR][SETTINGS] Failed to mount file system");
        return false;
    }

    Debug::println("[INFO][SETTINGS] File system mounted successfully");

    // Check if settings file exists, if not create a default one
    if (!settingsExist()) {
        Debug::println("[ERROR][SETTINGS] Settings file not found, creating default settings");
        String defaultSettings = "{\"system\": {\"name\": \"Home Controller\", \"version\": \"1.0\"}, \"wifi\": {\"ssid\": \"Home-Controller-AP\"}}";
        saveSettings(defaultSettings);
    }

    return true;
}

bool SettingsManager::saveSettings(const String& settings) {
    File file = SPIFFS.open(SETTINGS_FILE, FILE_WRITE);
    if (!file) {
        Debug::println("[ERROR][SETTINGS] Failed to open settings file for writing");
        return false;
    }

    if (file.print(settings)) {
        Debug::println("[INFO][SETTINGS] Settings saved successfully");
        file.close();
        return true;
    } else {
        Debug::println("[ERROR][SETTINGS] Failed to write settings to file");
        file.close();
        return false;
    }
}

String SettingsManager::loadSettings() {
    if (!settingsExist()) {
        Debug::println("[WARN][SETTINGS] Settings file does not exist");
        return "{}";
    }

    File file = SPIFFS.open(SETTINGS_FILE, FILE_READ);
    if (!file) {
        Debug::println("[ERROR][SETTINGS] Failed to open settings file for reading");
        return "{}";
    }

    String settings = file.readString();
    file.close();

    Debug::println("[INFO][SETTINGS] Settings loaded successfully");
    return settings;
}

String SettingsManager::loadSettingsWithPresetSummary(const LampState presets[5][11]) {
    String json = loadSettings();
    String lampJson = ",\"lamp_data\":{";

    for (int slot = 0; slot < 5; slot++) {
        int configured = 0;
        for (int i = 0; i < 11; i++) {
            if (presets[slot][i].v > 0) {
                configured++;
            }
        }
        lampJson += "\"Preset_" + String(slot) + "\":\"" + String(configured) + " lamps configured\"";
        if (slot < 4) {
            lampJson += ",";
        }
    }
    lampJson += "}";

    if (json.endsWith("}")) {
        return json.substring(0, json.length() - 1) + lampJson + "}";
    }
    return json;
}

bool SettingsManager::savePresets(const uint8_t* data, size_t size) {
    File file = SPIFFS.open("/presets.bin", FILE_WRITE);
    if (!file) {
        Debug::println("[ERROR][SETTINGS] Failed to open presets file for writing");
        return false;
    }
    file.write(data, size);
    file.close();
    Debug::println("[INFO][SETTINGS] Presets saved to flash memory.");
    return true;
}

void SettingsManager::loadPresets(uint8_t* data, size_t size) {
    if (SPIFFS.exists("/presets.bin")) {
        File file = SPIFFS.open("/presets.bin", FILE_READ);
        if (file && file.size() == size) {
            size_t read = file.read(data, size);
            if (read != size) {
                Debug::println("[ERROR][SETTINGS] Preset file incomplete, initializing to defaults");
                memset(data, 0, size);  // Reset to defaults
            } else {
                Debug::println("[INFO][SETTINGS] Presets loaded from flash memory.");
            }
            file.close();
        } else {
            Debug::println("[WARN][SETTINGS] Preset file missing or wrong size, initializing to defaults");
            memset(data, 0, size);  // Default empty presets
        }
    } else {
        Debug::println("[INFO][SETTINGS] No preset file found, using defaults");
        memset(data, 0, size);  // Default empty presets
    }
}

bool SettingsManager::settingsExist() {
    return SPIFFS.exists(SETTINGS_FILE);
}

bool SettingsManager::formatFileSystem() {
    Debug::println("[WARN][SETTINGS] Formatting file system...");
    if (SPIFFS.format()) {
        Debug::println("[INFO][SETTINGS] File system formatted successfully");
        return true;
    } else {
        Debug::println("[ERROR][SETTINGS] Failed to format file system");
        return false;
    }
}

void SettingsManager::reboot() {
    Debug::println("[INFO][SETTINGS] Rebooting system...");
    delay(1000);
    ESP.restart();
}
