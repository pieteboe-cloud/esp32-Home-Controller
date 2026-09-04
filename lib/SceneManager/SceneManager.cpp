#include "SceneManager.h"

SceneManager::SceneManager(HardwareManager& hardware)
    : hardware(hardware), sceneCount(0) {
    // Constructor: Initialize SceneManager with hardware reference
    // sceneCount is initialized to 0, will be populated when begin() is called
}

void SceneManager::begin() {
    // Initialize the SceneManager
    // Attempts to load scenes from storage, creates new file if none exists
    Debug::println(2, "[SceneManager] Initializing...");
    if (!loadScenes()) {
        Debug::println(2, "[SceneManager] No scripts.json found, will create on first save.");
    }
    Debug::println(2, "[SceneManager] Ready. " + String(sceneCount) + " scene(s) loaded.");
}

// ============================================================================
// Persistence
// ============================================================================

bool SceneManager::loadScenes() {
    // Load scenes from storage file
    // Reads scenes.json from filesystem and populates the scenes array
    // Returns true if successful, false if file doesn't exist or has errors

    // First try to load from the new scenes.json file. If it is missing OR empty
    // (0 bytes after a partial flash), fall back to the legacy scripts.json file.
    if (!LittleFS.exists(SCENE_FILE) || LittleFS.open(SCENE_FILE, "r").size() == 0) {
        const char* OLD_SCENE_FILE = "/scripts.json";
        if (LittleFS.exists(OLD_SCENE_FILE)) {
            Debug::println(2, "[SceneManager] scenes.json empty/missing, loading scenes from scripts.json");
            File f = LittleFS.open(OLD_SCENE_FILE, "r");
            if (!f) {
                Debug::println(1, "[SceneManager] Failed to open scripts.json.");
                return false;
            }
            DynamicJsonDocument doc(JSON_DOC_SIZE);
            DeserializationError error = deserializeJson(doc, f);
            f.close();
            if (error) {
                Debug::println(1, "[SceneManager] JSON parse error: " + String(error.c_str()));
                return false;
            }
            JsonArray arr = doc.as<JsonArray>();
            sceneCount = 0;
            for (JsonObject obj : arr) {
                if (sceneCount >= MAX_SCENES) break;
                if (!loadSceneObject(obj)) continue;
                sceneCount++;
            }
            Debug::println(2, "[SceneManager] Loaded " + String(sceneCount) + " scenes from scripts.json.");
            return true;
        }
        Debug::println(2, "[SceneManager] scenes.json does not exist yet.");
        return false;
    }

    File f = LittleFS.open(SCENE_FILE, "r");
    if (!f) {
        Debug::println(1, "[SceneManager] Failed to open scenes.json.");
        return false;
    }

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, f);
    f.close();

    if (error) {
        Debug::println(1, "[SceneManager] JSON parse error: " + String(error.c_str()));
        return false;
    }

    // Handle both scripts and scenes formats
    JsonArray arr;
    if (doc.containsKey("scenes")) {
        // Traditional scenes format
        arr = doc["scenes"].as<JsonArray>();
        Debug::println(2, "[SceneManager] Loading traditional scenes format");
    } else if (doc.is<JsonArray>()) {
        // Scripts format (array of scripts)
        arr = doc.as<JsonArray>();
        Debug::println(2, "[SceneManager] Loading scripts format");
    } else {
        Debug::println(1, "[SceneManager] Invalid JSON format - expected array or scenes object");
        return false;
    }
    sceneCount = 0;

    // Process each scene object in the array
    for (JsonObject obj : arr) {
        if (sceneCount >= MAX_SCENES) {
            Debug::println(1, "[SceneManager] Warning: Reached maximum number of scenes");
            break;
        }

        if (!loadSceneObject(obj)) continue;
        sceneCount++;
    }

    Debug::println(2, "[SceneManager] Loaded " + String(sceneCount) + " scenes.");
    return true;
}

bool SceneManager::loadSceneObject(JsonObject obj) {
    // Extract scene properties into scenes[sceneCount].
    // Returns false if the object has no usable name.

    // Extract scene properties
    scenes[sceneCount].id = obj["id"] | 0;
    scenes[sceneCount].name = obj["name"].as<String>();

    // Check if this is a script format (has triggers) or scene format
    if (obj.containsKey("triggers")) {
        // Script format - convert to scene format
        Debug::println(3, "[SceneManager] Converting script to scene format");

        // Create aliases from the script name
        DynamicJsonDocument aliasDoc(512);
        JsonArray aliases = aliasDoc.to<JsonArray>();
        aliases.add(obj["name"].as<String>());
        String aliasStr;
        serializeJson(aliases, aliasStr);
        scenes[sceneCount].aliasesJson = aliasStr;

        // Create empty colors object
        DynamicJsonDocument colorDoc(1024);
        JsonObject colors = colorDoc.to<JsonObject>();
        String colorStr;
        serializeJson(colors, colorStr);
        scenes[sceneCount].colorsJson = colorStr;

        // Convert actions from script format to scene format
        DynamicJsonDocument actionDoc(2048);
        JsonArray actions = actionDoc.to<JsonArray>();

        // Copy actions from script
        if (obj.containsKey("actions")) {
            for (JsonObject action : obj["actions"].as<JsonArray>()) {
                JsonObject newAction = actions.createNestedObject();
                newAction["type"] = action["type"];  // ir, living, etc.
                newAction["deviceId"] = action["deviceId"];

                // Copy specific properties based on type
                if (action.containsKey("remoteId")) newAction["remoteId"] = action["remoteId"];
                if (action.containsKey("buttonName")) newAction["buttonName"] = action["buttonName"];
                if (action.containsKey("code")) newAction["code"] = action["code"];
                if (action.containsKey("bits")) newAction["bits"] = action["bits"];
                if (action.containsKey("color")) newAction["color"] = action["color"];
                if (action.containsKey("intensity")) newAction["intensity"] = action["intensity"];
            }
        }

        String actionStr;
        serializeJson(actions, actionStr);
        scenes[sceneCount].actionsJson = actionStr;

        // Create empty kaku object
        DynamicJsonDocument kakuDoc(256);
        JsonObject kaku = kakuDoc.to<JsonObject>();
        String kakuStr;
        serializeJson(kaku, kakuStr);
        scenes[sceneCount].kakuJson = kakuStr;
    } else {
        // Traditional scene format
        Debug::println(3, "[SceneManager] Loading traditional scene format");

        // Serialize each JSON element to string
        String aliasStr;
        serializeJson(obj["aliases"], aliasStr);
        scenes[sceneCount].aliasesJson = aliasStr;

        String colorStr;
        serializeJson(obj["colors"], colorStr);
        scenes[sceneCount].colorsJson = colorStr;

        String actionStr;
        serializeJson(obj["actions"], actionStr);
        scenes[sceneCount].actionsJson = actionStr;

        String kakuStr;
        serializeJson(obj["kaku"], kakuStr);
        scenes[sceneCount].kakuJson = kakuStr;
    }

    return true;
}

bool SceneManager::saveScenes() {
    // Save scenes to storage file
    // Creates a JSON document with all scenes and writes to scenes.json
    // Returns true if successful, false if file operations fail

    Debug::println(3, "[SceneManager] Saving " + String(sceneCount) + " scenes to disk...");
    DynamicJsonDocument doc(JSON_DOC_SIZE);

    // Check if we should save in scripts format or scenes format
    bool useScriptsFormat = false;
    for (int i = 0; i < sceneCount; i++) {
        // If any scene has triggers, use scripts format
        DynamicJsonDocument testDoc(256);
        if (deserializeJson(testDoc, scenes[i].actionsJson) == DeserializationError::Ok) {
            JsonArray actions = testDoc.as<JsonArray>();
            for (JsonObject action : actions) {
                if (action.containsKey("type") && action["type"] == "script") {
                    useScriptsFormat = true;
                    break;
                }
            }
        }
        if (useScriptsFormat) break;
    }

    JsonArray arr;
    if (useScriptsFormat) {
        // Save in scripts format
        arr = doc.to<JsonArray>();
        Debug::println(3, "[SceneManager] Using scripts format for save");
    } else {
        // Save in traditional scenes format
        arr = doc.createNestedArray("scenes");
        Debug::println(3, "[SceneManager] Using traditional scenes format for save");
    }

    for (int i = 0; i < sceneCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["id"] = scenes[i].id;
        obj["name"] = scenes[i].name;

        if (useScriptsFormat) {
            // Save in scripts format
            Debug::println(4, "[SceneManager] Saving scene " + String(i) + " in scripts format");

            // Add aliases
            DynamicJsonDocument aliasDoc(512);
            if (deserializeJson(aliasDoc, scenes[i].aliasesJson) == DeserializationError::Ok) {
                obj["aliases"] = aliasDoc.as<JsonArray>();
            } else {
                obj["aliases"] = JsonArray();
            }

            // Add triggers (empty for now)
            JsonArray triggers = JsonArray();
            obj["triggers"] = triggers;

            // Add actions
            DynamicJsonDocument actionDoc(2048);
            if (deserializeJson(actionDoc, scenes[i].actionsJson) == DeserializationError::Ok) {
                obj["actions"] = actionDoc.as<JsonArray>();
            } else {
                obj["actions"] = JsonArray();
            }
        } else {
            // Save in traditional scenes format
            Debug::println(4, "[SceneManager] Saving scene " + String(i) + " in traditional format");

            // Parse and re-serialize to ensure valid JSON
            DynamicJsonDocument aliasDoc(512);
            if (deserializeJson(aliasDoc, scenes[i].aliasesJson) == DeserializationError::Ok) {
                obj["aliases"] = aliasDoc.as<JsonArray>();
            } else {
                obj["aliases"] = JsonArray();
            }

            DynamicJsonDocument colorDoc(1024);
            if (deserializeJson(colorDoc, scenes[i].colorsJson) == DeserializationError::Ok) {
                obj["colors"] = colorDoc.as<JsonObject>();
            } else {
                obj["colors"] = JsonObject();
            }

            DynamicJsonDocument actionDoc(2048);
            if (deserializeJson(actionDoc, scenes[i].actionsJson) == DeserializationError::Ok) {
                obj["actions"] = actionDoc.as<JsonArray>();
            } else {
                obj["actions"] = JsonArray();
            }

            DynamicJsonDocument kakuDoc(256);
            if (deserializeJson(kakuDoc, scenes[i].kakuJson) == DeserializationError::Ok) {
                obj["kaku"] = kakuDoc.as<JsonObject>();
            }
        }
    }

    // Open file for writing
    File f = LittleFS.open(SCENE_FILE, "w");
    if (!f) {
        Debug::println(1, "[SceneManager] Failed to open scripts.json for writing.");
        return false;
    }

    // Write JSON to file and close
    serializeJson(doc, f);
    f.close();

    Debug::println(2, "[SceneManager] Successfully saved " + String(sceneCount) + " scenes to disk.");
    return true;
}

String SceneManager::getScenesAsJson() {
    // Export all scenes as JSON string
    // Creates a JSON document with all scenes and returns as string
    // Useful for API responses or debugging

    Debug::println(3, "[SceneManager] Exporting " + String(sceneCount) + " scenes as JSON");
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    JsonArray arr = doc.createNestedArray("scenes");

    for (int i = 0; i < sceneCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["id"] = scenes[i].id;
        obj["name"] = scenes[i].name;

        // Re-parse stored JSON strings back into objects
        DynamicJsonDocument tmp1(512);
        if (deserializeJson(tmp1, scenes[i].aliasesJson) == DeserializationError::Ok) {
            obj["aliases"] = tmp1.as<JsonArray>();
        }

        DynamicJsonDocument tmp2(1024);
        if (deserializeJson(tmp2, scenes[i].colorsJson) == DeserializationError::Ok) {
            obj["colors"] = tmp2.as<JsonObject>();
        }

        DynamicJsonDocument tmp3(2048);
        if (deserializeJson(tmp3, scenes[i].actionsJson) == DeserializationError::Ok) {
            obj["actions"] = tmp3.as<JsonArray>();
        }

        DynamicJsonDocument tmp4(256);
        if (deserializeJson(tmp4, scenes[i].kakuJson) == DeserializationError::Ok) {
            obj["kaku"] = tmp4.as<JsonObject>();
        }
    }

    String result;
    serializeJson(doc, result);
    return result;
}

// ============================================================================
// CRUD Operations
// ============================================================================

int SceneManager::findSceneIndex(int sceneId) {
    // Find scene index by ID
    // Searches through all loaded scenes for matching ID
    // Returns index if found, -1 if not found

    Debug::println(4, "[SceneManager] Searching for scene with ID: " + String(sceneId));
    for (int i = 0; i < sceneCount; i++) {
        if (scenes[i].id == sceneId) {
            Debug::println(4, "[SceneManager] Found scene at index " + String(i));
            return i;
        }
    }
    Debug::println(4, "[SceneManager] Scene not found");
    return -1;
}

int SceneManager::nextSceneId() const {
    // Calculate next available scene ID
    // Finds the highest existing ID and returns the next integer
    // Used when creating new scenes to ensure unique IDs

    int maxId = 0;
    for (int i = 0; i < sceneCount; i++) {
        if (scenes[i].id > maxId) maxId = scenes[i].id;
    }
    Debug::println(4, "[SceneManager] Next available scene ID: " + String(maxId + 1));
    return maxId + 1;
}

bool SceneManager::parseSceneObject(const String& json, JsonObject& out, DynamicJsonDocument& doc) {
    // Parse JSON string into JsonObject
    // Validates JSON and ensures it's an object type
    // Returns true if successful, false on parse errors

    Debug::println(3, "[SceneManager] Parsing JSON object...");
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Debug::println(1, "[SceneManager] JSON parse error: " + String(error.c_str()));
        return false;
    }

    if (!doc.is<JsonObject>()) {
        Debug::println(1, "[SceneManager] Input is not a JSON object.");
        return false;
    }

    out = doc.as<JsonObject>();
    Debug::println(3, "[SceneManager] Successfully parsed JSON object");
    return true;
}

bool SceneManager::addScene(const String& sceneJson) {
    // Add a new scene from JSON
    // Validates JSON, extracts properties, and adds to scenes array
    // Returns true if successful, false on errors or if max scenes reached

    Debug::println(3, "[SceneManager] Adding new scene...");
    if (sceneCount >= MAX_SCENES) {
        Debug::println(1, "[SceneManager] Max scenes (" + String(MAX_SCENES) + ") reached.");
        return false;
    }

    DynamicJsonDocument doc(4096);
    JsonObject obj;
    if (!parseSceneObject(sceneJson, obj, doc)) {
        return false;
    }

    // Assign unique ID and extract properties
    int newId = nextSceneId();
    scenes[sceneCount].id = newId;
    scenes[sceneCount].name = obj["name"].as<String>();
    Debug::println(4, "[SceneManager] Adding scene with ID: " + String(newId));

    // Check if this is a script format (has triggers) or scene format
    if (obj.containsKey("triggers")) {
        // Script format - convert to scene format
        Debug::println(3, "[SceneManager] Converting script to scene format");

        // Create aliases from the script name
        DynamicJsonDocument aliasDoc(512);
        JsonArray aliases = aliasDoc.to<JsonArray>();
        aliases.add(obj["name"].as<String>());
        String aliasStr;
        serializeJson(aliases, aliasStr);
        scenes[sceneCount].aliasesJson = aliasStr;

        // Create empty colors object
        DynamicJsonDocument colorDoc(1024);
        JsonObject colors = colorDoc.to<JsonObject>();
        String colorStr;
        serializeJson(colors, colorStr);
        scenes[sceneCount].colorsJson = colorStr;

        // Convert actions from script format to scene format
        DynamicJsonDocument actionDoc(2048);
        JsonArray actions = actionDoc.to<JsonArray>();

        // Copy actions from script
        if (obj.containsKey("actions")) {
            for (JsonObject action : obj["actions"].as<JsonArray>()) {
                JsonObject newAction = actions.createNestedObject();
                newAction["type"] = action["type"];  // ir, living, etc.
                newAction["deviceId"] = action["deviceId"];

                // Copy specific properties based on type
                if (action.containsKey("remoteId")) newAction["remoteId"] = action["remoteId"];
                if (action.containsKey("buttonName")) newAction["buttonName"] = action["buttonName"];
                if (action.containsKey("code")) newAction["code"] = action["code"];
                if (action.containsKey("bits")) newAction["bits"] = action["bits"];
                if (action.containsKey("color")) newAction["color"] = action["color"];
                if (action.containsKey("intensity")) newAction["intensity"] = action["intensity"];
            }
        }

        String actionStr;
        serializeJson(actions, actionStr);
        scenes[sceneCount].actionsJson = actionStr;

        // Create empty kaku object
        DynamicJsonDocument kakuDoc(256);
        JsonObject kaku = kakuDoc.to<JsonObject>();
        String kakuStr;
        serializeJson(kaku, kakuStr);
        scenes[sceneCount].kakuJson = kakuStr;
    } else {
        // Traditional scene format
        Debug::println(3, "[SceneManager] Adding traditional scene format");

        // Serialize each JSON element to string
        String aliasStr;
        serializeJson(obj["aliases"], aliasStr);
        scenes[sceneCount].aliasesJson = aliasStr;

        String colorStr;
        serializeJson(obj["colors"], colorStr);
        scenes[sceneCount].colorsJson = colorStr;

        String actionStr;
        serializeJson(obj["actions"], actionStr);
        scenes[sceneCount].actionsJson = actionStr;

        String kakuStr;
        serializeJson(obj["kaku"], kakuStr);
        scenes[sceneCount].kakuJson = kakuStr;
    }

    // Increment scene count and save to storage
    sceneCount++;
    saveScenes();

    Debug::println(2, "[SceneManager] Added scene #" + String(scenes[sceneCount - 1].id) + ": " + scenes[sceneCount - 1].name);
    Debug::println(3, "[SceneManager] Total scenes: " + String(sceneCount));
    return true;
}

bool SceneManager::updateScene(int sceneId, const String& sceneJson) {
    // Update an existing scene
    // Finds scene by ID and updates properties from JSON
    // Returns true if successful, false if scene not found

    Debug::println(3, "[SceneManager] Updating scene #" + String(sceneId));
    int idx = findSceneIndex(sceneId);
    if (idx < 0) {
        Debug::println(1, "[SceneManager] Scene #" + String(sceneId) + " not found.");
        return false;
    }

    DynamicJsonDocument doc(4096);
    JsonObject obj;
    if (!parseSceneObject(sceneJson, obj, doc)) {
        return false;
    }

    // Update scene properties
    String oldName = scenes[idx].name;
    scenes[idx].name = obj["name"].as<String>();
    Debug::println(4, "[SceneManager] Updated scene name from '" + oldName + "' to '" + scenes[idx].name + "'");

    // Check if this is a script format (has triggers) or scene format
    if (obj.containsKey("triggers")) {
        // Script format - convert to scene format
        Debug::println(3, "[SceneManager] Converting script to scene format");

        // Create aliases from the script name
        DynamicJsonDocument aliasDoc(512);
        JsonArray aliases = aliasDoc.to<JsonArray>();
        aliases.add(obj["name"].as<String>());
        String aliasStr;
        serializeJson(aliases, aliasStr);
        scenes[idx].aliasesJson = aliasStr;

        // Create empty colors object
        DynamicJsonDocument colorDoc(1024);
        JsonObject colors = colorDoc.to<JsonObject>();
        String colorStr;
        serializeJson(colors, colorStr);
        scenes[idx].colorsJson = colorStr;

        // Convert actions from script format to scene format
        DynamicJsonDocument actionDoc(2048);
        JsonArray actions = actionDoc.to<JsonArray>();

        // Copy actions from script
        if (obj.containsKey("actions")) {
            for (JsonObject action : obj["actions"].as<JsonArray>()) {
                JsonObject newAction = actions.createNestedObject();
                newAction["type"] = action["type"];  // ir, living, etc.
                newAction["deviceId"] = action["deviceId"];

                // Copy specific properties based on type
                if (action.containsKey("remoteId")) newAction["remoteId"] = action["remoteId"];
                if (action.containsKey("buttonName")) newAction["buttonName"] = action["buttonName"];
                if (action.containsKey("code")) newAction["code"] = action["code"];
                if (action.containsKey("bits")) newAction["bits"] = action["bits"];
                if (action.containsKey("color")) newAction["color"] = action["color"];
                if (action.containsKey("intensity")) newAction["intensity"] = action["intensity"];
            }
        }

        String actionStr;
        serializeJson(actions, actionStr);
        scenes[idx].actionsJson = actionStr;

        // Create empty kaku object
        DynamicJsonDocument kakuDoc(256);
        JsonObject kaku = kakuDoc.to<JsonObject>();
        String kakuStr;
        serializeJson(kaku, kakuStr);
        scenes[idx].kakuJson = kakuStr;
    } else {
        // Traditional scene format
        Debug::println(3, "[SceneManager] Updating traditional scene format");

        // Serialize each JSON element to string
        String aliasStr;
        serializeJson(obj["aliases"], aliasStr);
        scenes[idx].aliasesJson = aliasStr;

        String colorStr;
        serializeJson(obj["colors"], colorStr);
        scenes[idx].colorsJson = colorStr;

        String actionStr;
        serializeJson(obj["actions"], actionStr);
        scenes[idx].actionsJson = actionStr;

        String kakuStr;
        serializeJson(obj["kaku"], kakuStr);
        scenes[idx].kakuJson = kakuStr;
    }

    // Save updated scenes to storage
    saveScenes();

    Debug::println(2, "[SceneManager] Updated scene #" + String(sceneId));
    return true;
}

bool SceneManager::deleteScene(int sceneId) {
    // Delete a scene by ID
    // Finds scene by ID, removes from array, and shifts remaining scenes
    // Returns true if successful, false if scene not found

    Debug::println(3, "[SceneManager] Deleting scene #" + String(sceneId));
    int idx = findSceneIndex(sceneId);
    if (idx < 0) {
        Debug::println(1, "[SceneManager] Scene #" + String(sceneId) + " not found.");
        return false;
    }

    // Shift all scenes after the deleted one
    String deletedName = scenes[idx].name;
    for (int i = idx; i < sceneCount - 1; i++) {
        scenes[i] = scenes[i + 1];
    }

    // Decrement scene count and save to storage
    sceneCount--;
    saveScenes();

    Debug::println(2, "[SceneManager] Deleted scene #" + String(sceneId) + ": " + deletedName);
    Debug::println(3, "[SceneManager] Total scenes: " + String(sceneCount));
    return true;
}

String SceneManager::getSceneById(int sceneId) {
    // Get a single scene by ID as JSON string
    // Finds scene by ID and returns as JSON string
    // Returns empty string if scene not found

    Debug::println(3, "[SceneManager] Getting scene #" + String(sceneId));
    int idx = findSceneIndex(sceneId);
    if (idx < 0) {
        Debug::println(1, "[SceneManager] Scene #" + String(sceneId) + " not found.");
        return String();
    }

    DynamicJsonDocument doc(4096);
    JsonObject obj = doc.to<JsonObject>();
    obj["id"] = scenes[idx].id;
    obj["name"] = scenes[idx].name;

    // Re-parse stored JSON strings back into objects
    DynamicJsonDocument tmp1(512);
    if (deserializeJson(tmp1, scenes[idx].aliasesJson) == DeserializationError::Ok) {
        obj["aliases"] = tmp1.as<JsonArray>();
    }

    DynamicJsonDocument tmp2(1024);
    if (deserializeJson(tmp2, scenes[idx].colorsJson) == DeserializationError::Ok) {
        obj["colors"] = tmp2.as<JsonObject>();
    }

    DynamicJsonDocument tmp3(2048);
    if (deserializeJson(tmp3, scenes[idx].actionsJson) == DeserializationError::Ok) {
        obj["actions"] = tmp3.as<JsonArray>();
    }

    DynamicJsonDocument tmp4(256);
    if (deserializeJson(tmp4, scenes[idx].kakuJson) == DeserializationError::Ok) {
        obj["kaku"] = tmp4.as<JsonObject>();
    }

    String result;
    serializeJson(doc, result);
    return result;
}

// ============================================================================
// Execution
// ============================================================================

bool SceneManager::applyScene(int sceneId) {
    // Apply a scene by ID
    // Finds scene by ID and executes its actions
    // Returns true if successful, false if scene not found or actions fail

    Debug::println(2, "[SceneManager] Applying scene #" + String(sceneId));
    int idx = findSceneIndex(sceneId);
    if (idx < 0) {
        Debug::println(1, "[SceneManager] Scene #" + String(sceneId) + " not found.");
        return false;
    }

    Debug::println(3, "[SceneManager] Executing actions for scene: " + scenes[idx].name);

    // Parse and execute actions
    DynamicJsonDocument actionDoc(2048);
    if (deserializeJson(actionDoc, scenes[idx].actionsJson) != DeserializationError::Ok) {
        Debug::println(1, "[SceneManager] Failed to parse actions JSON.");
        return false;
    }

    JsonArray actions = actionDoc.as<JsonArray>();
    return executeSceneActions(actions);
}

int SceneManager::findSceneByKaku(char house, uint8_t button) {
    // Find scene by Kaku (Klik-Aan-Klik-Uit) remote binding
    // Searches through all scenes for matching house code and button
    // Returns scene ID if found, -1 if not found

    Debug::println(4, "[SceneManager] Searching for scene with Kaku " + String(house) + ":" + String(button));

    for (int i = 0; i < sceneCount; i++) {
        // Parse Kaku JSON for this scene
        DynamicJsonDocument kakuDoc(256);
        if (deserializeJson(kakuDoc, scenes[i].kakuJson) == DeserializationError::Ok) {
            JsonObject kaku = kakuDoc.as<JsonObject>();
            if (kaku.containsKey("house") && kaku.containsKey("buttons")) {
                char sceneHouse = kaku["house"].as<unsigned char>();
                JsonArray buttons = kaku["buttons"].as<JsonArray>();

                if (sceneHouse == house) {
                    for (uint8_t btn : buttons) {
                        if (btn == button) {
                            Debug::println(4, "[SceneManager] Found scene #" + String(scenes[i].id) + " for Kaku " + String(house) + ":" + String(button));
                            return scenes[i].id;
                        }
                    }
                }
            }
        }
    }

    Debug::println(4, "[SceneManager] No scene found for Kaku " + String(house) + ":" + String(button));
    return -1;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool SceneManager::executeSceneActions(JsonArray actions) {
    // Execute a list of actions
    // Iterates through actions and calls appropriate hardware methods
    // Returns true if all actions succeed, false if any fail

    Debug::println(4, "[SceneManager] Executing " + String(actions.size()) + " actions");

    for (JsonObject action : actions) {
        String type = action["type"].as<String>();
        String deviceId = action["deviceId"].as<String>();

        Debug::println(4, "[SceneManager] Executing action: " + type + " on " + deviceId);

        if (type == "ir") {
            // IR command
            String remoteId = action["remoteId"].as<String>();
            String buttonName = action["buttonName"].as<String>();
            String code = action["code"].as<String>();
            int bits = action["bits"] | 32;

            Debug::println(4, "[SceneManager] IR command: " + remoteId + " -> " + buttonName + " (" + code + ")");
            IRCommand cmd;
            cmd.code = strtoul(code.c_str(), NULL, 16);
            cmd.bits = bits;
            cmd.protocol = FM_UNKNOWN;
            cmd.timestamp = millis();
            hardware.ir().send(cmd);
            Debug::println(4, "[SceneManager] IR command sent: " + remoteId + " -> " + buttonName + " (" + code + ")");
        } else if (type == "living") {
            // Living color/white light command
            String color = action["color"].as<String>();
            int intensity = action["intensity"] | 100;

            Debug::println(4, "[SceneManager] Living color command: " + color + " @ " + String(intensity) + "%");
            // Convert hex color to RGB
            uint8_t r, g, b;
            parseHexColor(color, r, g, b);

            // Set living color
            int lampId = deviceId.toInt();
            hardware.living().setColorRGB(lampId, r, g, b);
            Debug::println(4, "[SceneManager] Living color set: " + color + " for lamp " + String(lampId));
        } else if (type == "white") {
            // White light command
            int intensity = action["intensity"] | 100;

            Debug::println(4, "[SceneManager] White intensity command: " + String(intensity) + "%");
            // Set white intensity (not directly supported in LivingColors, so set RGB to white with intensity)
            int lampId = deviceId.toInt();
            uint8_t value = (uint8_t)(intensity * 2.55); // Convert percentage to 0-255
            hardware.living().setColorRGB(lampId, value, value, value);
            Debug::println(4, "[SceneManager] White intensity set: " + String(intensity) + "% for lamp " + String(lampId));
        } else if (type == "script") {
            // Script execution
            String scriptId = action["scriptId"].as<String>();

            Debug::println(4, "[SceneManager] Script execution: " + scriptId);
            // Note: Script execution would be handled by a ScriptManager class
            // This is a placeholder for future implementation
        } else {
            Debug::println(1, "[SceneManager] Unknown action type: " + type);
            return false;
        }
    }

    Debug::println(3, "[SceneManager] All actions executed successfully");
    return true;
}

void SceneManager::parseHexColor(const String& color, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Parse a hex color string (e.g. "#RRGGBB") into RGB components
    // Handles both 6-digit and 3-digit hex formats

    if (color.length() == 7) {
        // 6-digit format: #RRGGBB
        r = (uint8_t)strtol(color.substring(1, 3).c_str(), NULL, 16);
        g = (uint8_t)strtol(color.substring(3, 5).c_str(), NULL, 16);
        b = (uint8_t)strtol(color.substring(5, 7).c_str(), NULL, 16);
    } else if (color.length() == 4) {
        // 3-digit format: #RGB
        r = (uint8_t)strtol(String(color[1]).c_str(), NULL, 16);
        g = (uint8_t)strtol(String(color[2]).c_str(), NULL, 16);
        b = (uint8_t)strtol(String(color[3]).c_str(), NULL, 16);
        // For 3-digit format, we need to repeat each character
        r = r * 16 + r;
        g = g * 16 + g;
        b = b * 16 + b;
    } else {
        // Invalid format, return black
        r = 0;
        g = 0;
        b = 0;
    }
}
