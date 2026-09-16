#include "SceneManager.h"

#include "../Debug/Debug.h"
#include "../ScriptManager/ScriptManager.h"


// ============================================================================
// Constructor
// ============================================================================

SceneManager::SceneManager(
    ScriptManager& scripts
)
    : scriptManager(scripts)
{
}


// ============================================================================
// Begin
// ============================================================================

void SceneManager::begin()
{
    Debug::println(
        2,
        "[SceneManager] Initializing..."
    );

   
    loadScenes();

    Debug::println(
        2,
        "[SceneManager] Loaded " +
        String(sceneCount) +
        " scenes."
    );
}


// ============================================================================
// Load
// ============================================================================

bool SceneManager::loadScenes()
{
    if (!Storage::exists(SCENE_FILE))
    {
        sceneCount = 0;

        Debug::println(
            2,
            "[SceneManager] No scenes.json found."
        );

        return false;
    }

    return loadScenesFromFile(
        SCENE_FILE
    );
}


bool SceneManager::loadScenesFromFile(
    const char* path
)
{
    // Read file using Storage class
    String fileContent = Storage::read(path);

    if (fileContent.isEmpty())
        return false;

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    DeserializationError error =
        deserializeJson(
            doc,
            fileContent
        );

    if (error)
    {
        Debug::println(
            1,
            "[SceneManager] JSON error: " +
            String(error.c_str())
        );

        return false;
    }

    if (!doc.is<JsonArray>())
        return false;

    JsonArray array =
        doc.as<JsonArray>();

    sceneCount = 0;

    for (JsonObject obj : array)
    {
        if (sceneCount >= MAX_SCENES)
            break;

        Scene& scene =
            scenes[sceneCount];

        scene.id =
            obj["id"] | nextSceneId();

        scene.name =
            obj["name"] | "";

        scene.type =
            obj["type"] | "static";

        scene.script =
            obj["script"] | "";

        // aliases
        scene.aliases = "[]";

        if (obj["aliases"].is<JsonArray>())
        {
            serializeJson(
                obj["aliases"],
                scene.aliases
            );
        }

        // Kaku
        scene.kakuHouse = 0;
        scene.kakuButton = 0;

        if (obj["kaku"].is<JsonObject>())
        {
            JsonObject kaku =
                obj["kaku"];

            String house =
                kaku["house"] | "";

            if (house.length())
            {
                scene.kakuHouse =
                    house.charAt(0);

                scene.kakuHouse =
                    toupper(scene.kakuHouse);
            }

            scene.kakuButton =
                kaku["button"] | 0;
        }

        sceneCount++;
    }

    return true;
}


// ============================================================================
// Activate scene by ID
// ============================================================================

bool SceneManager::activateScene(
    int sceneId
)
{
    int index =
        findSceneIndex(sceneId);

    if (index < 0)
    {
        Debug::println(
            1,
            "[SceneManager] Scene not found: " +
            String(sceneId)
        );

        return false;
    }

    Scene& scene =
        scenes[index];

    Debug::println(
        2,
        "[SceneManager] Activating scene: " +
        scene.name
    );

    if (!scene.script.length())
    {
        Debug::println(
            2,
            "[SceneManager] Scene has no script: " +
            scene.name
        );

        return true;
    }

    /*
     * This is the important relationship:
     *
     * SceneManager does NOT execute the commands.
     *
     * SceneManager tells ScriptManager:
     *
     *     "Run this script."
     */

    return scriptManager.runScript(
        scene.script
    );
}


// ============================================================================
// Activate scene by name / alias
// ============================================================================

bool SceneManager::activateScene(
    const String& nameOrAlias
)
{
    int index =
        findSceneIndex(nameOrAlias);

    if (index < 0)
    {
        Debug::println(
            1,
            "[SceneManager] Scene not found: " +
            nameOrAlias
        );

        return false;
    }

    return activateScene(
        scenes[index].id
    );
}


// ============================================================================
// Kaku handling
// ============================================================================

bool SceneManager::handleKaku(
    char house,
    uint8_t button
)
{
    house =
        toupper(house);

    int sceneId =
        findSceneByKaku(
            house,
            button
        );

    if (sceneId < 0)
    {
        Debug::println(
            3,
            "[SceneManager] No scene for Kaku " +
            String(house) +
            String(button)
        );

        return false;
    }

    Debug::println(
        2,
        "[SceneManager] Kaku " +
        String(house) +
        String(button) +
        " → Scene " +
        String(sceneId)
    );

    return activateScene(sceneId);
}


// ============================================================================
// Find scene by Kaku
// ============================================================================

int SceneManager::findSceneByKaku(
    char house,
    uint8_t button
) const
{
    house =
        toupper(house);

    for (int i = 0; i < sceneCount; i++)
    {
        if (scenes[i].kakuHouse == house &&
            scenes[i].kakuButton == button)
        {
            return scenes[i].id;
        }
    }

    return -1;
}


// ============================================================================
// Find scene
// ============================================================================

int SceneManager::findSceneIndex(
    int sceneId
) const
{
    for (int i = 0; i < sceneCount; i++)
    {
        if (scenes[i].id == sceneId)
            return i;
    }

    return -1;
}


int SceneManager::findSceneIndex(
    const String& nameOrAlias
) const
{
    String search =
        nameOrAlias;

    search.trim();

    for (int i = 0; i < sceneCount; i++)
    {
        if (scenes[i].name.equalsIgnoreCase(search))
            return i;

        if (aliasesContain(
                scenes[i].aliases,
                search))
        {
            return i;
        }
    }

    return -1;
}


bool SceneManager::hasScene(
    const String& nameOrAlias
) const
{
    return findSceneIndex(
        nameOrAlias
    ) >= 0;
}


// ============================================================================
// Alias lookup
// ============================================================================

bool SceneManager::aliasesContain(
    const String& aliasesJson,
    const String& value
) const
{
    DynamicJsonDocument doc(1024);

    if (deserializeJson(
            doc,
            aliasesJson))
    {
        return false;
    }

    if (!doc.is<JsonArray>())
        return false;

    JsonArray aliases =
        doc.as<JsonArray>();

    for (JsonVariant alias : aliases)
    {
        String a =
            alias.as<String>();

        if (a.equalsIgnoreCase(value))
            return true;
    }

    return false;
}


// ============================================================================
// Add scene
// ============================================================================

bool SceneManager::addScene(
    const String& json
)
{
    if (sceneCount >= MAX_SCENES)
        return false;

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    if (deserializeJson(doc, json))
        return false;

    if (!doc.is<JsonObject>())
        return false;

    JsonObject obj =
        doc.as<JsonObject>();

    Scene& scene =
        scenes[sceneCount];

    scene.id =
        obj["id"] | nextSceneId();

    scene.name =
        obj["name"] | "";

    if (!scene.name.length())
        return false;

    scene.type =
        obj["type"] | "static";

    scene.script =
        obj["script"] | "";

    scene.aliases = "[]";

    if (obj["aliases"].is<JsonArray>())
    {
        serializeJson(
            obj["aliases"],
            scene.aliases
        );
    }

    scene.kakuHouse = 0;
    scene.kakuButton = 0;

    if (obj["kaku"].is<JsonObject>())
    {
        JsonObject kaku =
            obj["kaku"];

        String house =
            kaku["house"] | "";

        if (house.length())
            scene.kakuHouse =
                toupper(house.charAt(0));

        scene.kakuButton =
            kaku["button"] | 0;
    }

    sceneCount++;

    return saveScenes();
}


// ============================================================================
// Update
// ============================================================================

bool SceneManager::updateScene(
    int sceneId,
    const String& json
)
{
    int index =
        findSceneIndex(sceneId);

    if (index < 0)
        return false;

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    if (deserializeJson(doc, json))
        return false;

    if (!doc.is<JsonObject>())
        return false;

    JsonObject obj =
        doc.as<JsonObject>();

    Scene& scene =
        scenes[index];

    scene.name =
        obj["name"] | scene.name;

    scene.type =
        obj["type"] | scene.type;

    scene.script =
        obj["script"] | scene.script;

    if (obj["aliases"].is<JsonArray>())
    {
        serializeJson(
            obj["aliases"],
            scene.aliases
        );
    }

    if (obj["kaku"].is<JsonObject>())
    {
        JsonObject kaku =
            obj["kaku"];

        String house =
            kaku["house"] | "";

        if (house.length())
            scene.kakuHouse =
                toupper(house.charAt(0));

        scene.kakuButton =
            kaku["button"] | 0;
    }

    return saveScenes();
}


// ============================================================================
// Delete
// ============================================================================

bool SceneManager::deleteScene(
    int sceneId
)
{
    int index =
        findSceneIndex(sceneId);

    if (index < 0)
        return false;

    for (int i = index;
         i < sceneCount - 1;
         i++)
    {
        scenes[i] =
            scenes[i + 1];
    }

    sceneCount--;

    return saveScenes();
}


// ============================================================================
// IDs
// ============================================================================

int SceneManager::nextSceneId() const
{
    int maxId = 0;

    for (int i = 0; i < sceneCount; i++)
    {
        if (scenes[i].id > maxId)
            maxId = scenes[i].id;
    }

    return maxId + 1;
}


// ============================================================================
// Save
// ============================================================================

bool SceneManager::saveScenes()
{
    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    JsonArray array =
        doc.to<JsonArray>();

    for (int i = 0; i < sceneCount; i++)
    {
        Scene& scene =
            scenes[i];

        JsonObject obj =
            array.createNestedObject();

        obj["id"] =
            scene.id;

        obj["name"] =
            scene.name;

        obj["type"] =
            scene.type;

        obj["script"] =
            scene.script;

        // aliases
        DynamicJsonDocument aliasDoc(1024);

        if (!deserializeJson(
                aliasDoc,
                scene.aliases))
        {
            obj["aliases"] =
                aliasDoc.as<JsonArray>();
        }

        // Kaku
        if (scene.kakuHouse != 0)
        {
            JsonObject kaku =
                obj.createNestedObject("kaku");

            String house;
            house += scene.kakuHouse;

            kaku["house"] =
                house;

            kaku["button"] =
                scene.kakuButton;
        }
    }

    String json;

    serializeJsonPretty(
        doc,
        json
    );

    return saveScenesToFile(
        SCENE_FILE,
        json
    );
}


bool SceneManager::saveScenesToFile(
    const char* path,
    const String& json
)
{
    // Write file using Storage class
    return Storage::write(path, json);
}


// ============================================================================
// JSON output
// ============================================================================

String SceneManager::getScenesAsJson()
{
    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    JsonArray array =
        doc.to<JsonArray>();

    for (int i = 0; i < sceneCount; i++)
    {
        Scene& scene =
            scenes[i];

        JsonObject obj =
            array.createNestedObject();

        obj["id"] =
            scene.id;

        obj["name"] =
            scene.name;

        obj["type"] =
            scene.type;

        obj["script"] =
            scene.script;

        DynamicJsonDocument aliasDoc(1024);

        if (!deserializeJson(
                aliasDoc,
                scene.aliases))
        {
            obj["aliases"] =
                aliasDoc.as<JsonArray>();
        }

        if (scene.kakuHouse != 0)
        {
            JsonObject kaku =
                obj.createNestedObject("kaku");

            String house;
            house += scene.kakuHouse;

            kaku["house"] =
                house;

            kaku["button"] =
                scene.kakuButton;
        }
    }

    String output;

    serializeJson(
        doc,
        output
    );

    return output;
}


String SceneManager::getSceneById(
    int sceneId
)
{
    int index =
        findSceneIndex(sceneId);

    if (index < 0)
        return "{}";

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    Scene& scene =
        scenes[index];

    JsonObject obj =
        doc.to<JsonObject>();

    obj["id"] =
        scene.id;

    obj["name"] =
        scene.name;

    obj["type"] =
        scene.type;

    obj["script"] =
        scene.script;

    DynamicJsonDocument aliasDoc(1024);

    if (!deserializeJson(
            aliasDoc,
            scene.aliases))
    {
        obj["aliases"] =
            aliasDoc.as<JsonArray>();
    }

    if (scene.kakuHouse != 0)
    {
        JsonObject kaku =
            obj.createNestedObject("kaku");

        String house;
        house += scene.kakuHouse;

        kaku["house"] =
            house;

        kaku["button"] =
            scene.kakuButton;
    }

    String output;

    serializeJson(
        doc,
        output
    );

    return output;
}