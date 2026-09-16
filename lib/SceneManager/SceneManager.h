#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../HardwareManager/Storage/StorageManager.h"

class ScriptManager;


class SceneManager
{
public:

    SceneManager(ScriptManager& scripts);

    void begin();

    // ------------------------------------------------------------------------
    // Scene activation
    // ------------------------------------------------------------------------

    bool activateScene(int sceneId);

    bool activateScene(
        const String& nameOrAlias
    );

    // Kaku → Scene
    bool handleKaku(
        char house,
        uint8_t button
    );

    // ------------------------------------------------------------------------
    // Lookup
    // ------------------------------------------------------------------------

    int findSceneByKaku(
        char house,
        uint8_t button
    ) const;

    bool hasScene(
        const String& nameOrAlias
    ) const;

    // ------------------------------------------------------------------------
    // Persistence
    // ------------------------------------------------------------------------

    bool loadScenes();
    bool saveScenes();

    bool addScene(const String& json);
    bool updateScene(
        int sceneId,
        const String& json
    );

    bool deleteScene(int sceneId);

    String getScenesAsJson();
    String getSceneById(int sceneId);

private:

    static const int MAX_SCENES = 50;
    static const size_t JSON_DOC_SIZE = 16384;

    struct Scene
    {
        int id = 0;

        String name;

        // JSON array:
        // ["movie", "film"]
        String aliases;

        // Script name or alias.
        String script;

        // Kaku house.
        char kakuHouse = 0;

        // Kaku button.
        uint8_t kakuButton = 0;

        // static / animated
        String type = "static";
    };

    ScriptManager& scriptManager;

    Scene scenes[MAX_SCENES];
    int sceneCount = 0;

    const char* SCENE_FILE = "/scenes.json";

    int findSceneIndex(int sceneId) const;

    int findSceneIndex(
        const String& nameOrAlias
    ) const;

    int nextSceneId() const;

    bool loadScenesFromFile(
        const char* path
    );

    bool saveScenesToFile(
        const char* path,
        const String& json
    );

    bool aliasesContain(
        const String& aliasesJson,
        const String& value
    ) const;
};