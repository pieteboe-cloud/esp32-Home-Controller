#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../Debug/Debug.h"
#include "../Core/EventBus.h"
#include "../HardwareManager/HardwareManager.h"

// Forward declaration to avoid circular includes
class SceneManager;

/**
 * Action/scene manager.
 *
 * IMPORTANT: this class deliberately uses the existing HardwareManager API;
 * HardwareManager and Debug are not modified by this package.
 */
class ScriptManager {
public:
    explicit ScriptManager(HardwareManager& hardware);

    void begin();
    void setSceneManager(SceneManager* sm) { sceneManager = sm; }
    bool loadScripts();
    bool saveScripts();
    bool addScript(const String& script);
    bool updateScript(int scriptId, const String& script);
    bool deleteScript(int scriptId);
    bool executeScript(int scriptId);
    bool executeAction(const String& actionJson);

    String getScriptsAsJson();
    String getScriptById(int scriptId);
    String getRawScriptsFile() const;

    // Learn the next incoming event. filter: ANY, KAKU, IR or RF.
    void startCapture(const String& filter = "ANY");
    String getCaptureStatus() const;

private:
    static const int MAX_SCRIPTS = 50;
    static const size_t JSON_DOC_SIZE = 16384;

    struct Script {
        int id = 0;
        String name;
        String alias;       // backwards-compatible first alias
        String aliases;     // JSON array string
        String triggers;    // JSON array string
        String actions;     // JSON array string
    };

    HardwareManager& hardware;
    SceneManager* sceneManager = nullptr;  // Optional reference for Kaku→Scene mapping
    Script scripts[MAX_SCRIPTS];
    int scriptCount = 0;
    const char* SCRIPT_FILE = "/scripts.json";
    const char* SCRIPT_BACKUP_FILE = "/backup/scripts.json";

    bool loadScriptsFromFile(const char* path);
    bool saveScriptsToFile(const char* path, const String& json);

    volatile bool captureWaiting = false;
    String captureFilter = "ANY";
    String capturedSource;
    String capturedIdentifier;
    String capturedRawData;
    unsigned long captureTime = 0;

    int findScriptIndex(int scriptId);
    int nextScriptId() const;

    bool parseScriptObject(const String& json, JsonObject& out, DynamicJsonDocument& doc);
    bool executeActionObject(JsonObject action);
    bool matchesTrigger(JsonArray triggers, const SystemEvent& evt) const;
    bool eventMatchesCapture(const SystemEvent& evt) const;
    void handleSystemEvent(const SystemEvent& evt);

    static uint32_t parseUnsigned(const String& text);
    static void parseHexColor(const String& color, uint8_t& r, uint8_t& g, uint8_t& b);
    static String jsonArrayToString(JsonArray array);
};
