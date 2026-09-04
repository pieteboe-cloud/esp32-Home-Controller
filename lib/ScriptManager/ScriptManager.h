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
 * ScriptManager owns the runtime action engine and script persistence.
 *
 * Responsibilities:
 * - load and save script definitions from LittleFS
 * - match incoming events to script triggers
 * - execute actions against RF/IR/LivingColors hardware
 * - optionally resolve Kaku events to scenes when a SceneManager is attached
 *
 * This class intentionally does not own scene loading itself; scene persistence and
 * loading are owned by SceneManager and initialized by Core.
 */
class ScriptManager {
public:
    explicit ScriptManager(HardwareManager& hardware);

    /**
     * Starts the script runtime and loads scripts from disk.
     */
    void begin();

    /**
     * Wiring for Kaku-to-scene lookup used by learned triggers.
     */
    void setSceneManager(SceneManager* sm) { sceneManager = sm; }

    /**
     * Script persistence and runtime lifecycle methods.
     */
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

    /**
     * Begins event capture for learning new triggers.
     * @param filter One of ANY, KAKU, IR, RF.
     */
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
