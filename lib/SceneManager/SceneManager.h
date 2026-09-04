#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../Debug/Debug.h"
#include "../HardwareManager/HardwareManager.h"

/**
 * SceneManager owns scene persistence and scene execution.
 *
 * A scene is a reusable collection of device state and actions that can be applied at once.
 * It is the single owner of loading and saving scene definitions, plus scene lookup and
 * application logic. ScriptManager may consume scene information for Kaku-trigger mapping,
 * but it does not own the scene file itself.
 */
class SceneManager {
public:
    explicit SceneManager(HardwareManager& hardware);

    /**
     * Loads scene data from storage and prepares the runtime state.
     */
    void begin();

    /**
     * Persistence helpers for the scene file.
     */
    bool loadScenes();
    bool saveScenes();
    String getScenesAsJson();

    /**
     * CRUD methods for scene entries.
     */
    bool addScene(const String& sceneJson);
    bool updateScene(int sceneId, const String& sceneJson);
    bool deleteScene(int sceneId);
    String getSceneById(int sceneId);

    /**
     * Applies a full scene to hardware state.
     */
    bool applyScene(int sceneId);

    /**
     * Finds the scene bound to a Kaku house/button pair.
     * @return scene ID, or -1 when no scene matches.
     */
    int findSceneByKaku(char house, uint8_t button);

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
    const char* SCENE_FILE = "/scenes.json";  // Separate file for scenes

    int findSceneIndex(int sceneId);
    int nextSceneId() const;

    bool parseSceneObject(const String& json, JsonObject& out, DynamicJsonDocument& doc);
    bool loadSceneObject(JsonObject obj);
    bool executeSceneActions(JsonArray actions);

    static void parseHexColor(const String& color, uint8_t& r, uint8_t& g, uint8_t& b);
};
