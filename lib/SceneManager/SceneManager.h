#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../Debug/Debug.h"
#include "../HardwareManager/HardwareManager.h"

/**
 * Scene Manager - manages named scenes with hardware state + action references.
 * 
 * A scene is:
 *   - name + aliases
 *   - hardware state (color map for devices)
 *   - actions (direct HW commands or references to scripts)
 *   - optional Kaku binding (house code + buttons)
 * 
 * Scenes are stored in /scripts.json (same file as ScriptManager) and can be applied to set multiple devices at once.
 */
class SceneManager {
public:
    explicit SceneManager(HardwareManager& hardware);

    void begin();
    
    // Persistence
    bool loadScenes();
    bool saveScenes();
    String getScenesAsJson();
    
    // CRUD
    bool addScene(const String& sceneJson);          // Create new scene
    bool updateScene(int sceneId, const String& sceneJson);  // Update existing
    bool deleteScene(int sceneId);                   // Delete by ID
    String getSceneById(int sceneId);                // Fetch single scene as JSON
    
    // Execution
    bool applyScene(int sceneId);                    // Apply scene state to hardware
    
    // Kaku lookup
    int findSceneByKaku(char house, uint8_t button); // Returns scene ID or -1
    
private:
    static const int MAX_SCENES = 50;
    static const size_t JSON_DOC_SIZE = 32768;  // Larger for scenes
    
    struct Scene {
        int id = 0;
        String name;
        String aliasesJson;      // JSON array string: ["alias1", "alias2", ...]
        String colorsJson;       // JSON object: { "device_key": "#rrggbb", ... }
        String actionsJson;      // JSON array: [{ type, id, ... }, ...]
        String kakuJson;         // JSON object: { "house": "D", "buttons": [9, 10] }
    };
    
    HardwareManager& hardware;
    Scene scenes[MAX_SCENES];
    int sceneCount = 0;
    const char* SCENE_FILE = "/scripts.json";  // Use same file as ScriptManager
    
    int findSceneIndex(int sceneId);
    int nextSceneId() const;
    
    bool parseSceneObject(const String& json, JsonObject& out, DynamicJsonDocument& doc);
    bool executeSceneActions(JsonArray actions);
    
    static void parseHexColor(const String& color, uint8_t& r, uint8_t& g, uint8_t& b);
};
