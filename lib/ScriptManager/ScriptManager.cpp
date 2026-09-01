#include "ScriptManager.h"
#include "../SceneManager/SceneManager.h"

/**
 * @brief Constructor for ScriptManager
 * @param hw Reference to the HardwareManager instance
 * 
 * Initializes the ScriptManager with a reference to the HardwareManager.
 * The HardwareManager is used to execute actions (e.g., sending IR/RF signals, controlling LivingColors).
 */
ScriptManager::ScriptManager(HardwareManager& hw)
    : hardware(hw) {}

/**
 * @brief Initialize the ScriptManager
 * 
 * This method performs the following operations:
 * 1. Initializes LittleFS for script storage
 * 2. Loads existing scripts from filesystem
 * 3. Sets up event subscription to receive system events
 * 4. Prepares the system to match incoming events against script triggers
 */
void ScriptManager::begin() {
    // Initialize Little filesystem for script storage
    if (!LittleFS.begin()) {
        Debug::println(1, "[SCRIPTMANAGER][ERROR] LittleFS mount failed!");
        return;
    }

    Debug::println(3, "[SCRIPTMANAGER] Initializing runtime action engine");
    Debug::println(3, "[SCRIPTMANAGER] Script file: " + String(SCRIPT_FILE) + ", max scripts: " + String(MAX_SCRIPTS));
    
    // Load existing scripts from filesystem
    loadScripts();
    
    // Subscribe to system events to trigger scripts
    EventBus::getInstance().subscribe([this](const SystemEvent& evt) {
        handleSystemEvent(evt);
    });
    
    Debug::println(3, "[SCRIPTMANAGER] Event trigger listener installed");
    Debug::println(2, "[SCRIPTMANAGER][SUMMARY] Ready to match RF/IR/storage events against saved triggers");
}

/**
 * @brief Load scripts from a file
 * @param path Filesystem path to the script file
 * @return true if loading succeeded, false otherwise
 * 
 * This method loads scripts from the specified JSON file in LittleFS.
 * It parses the JSON structure and populates the internal script array.
 * Handles different JSON formats for backwards compatibility.
 */
bool ScriptManager::loadScriptsFromFile(const char* path) {
    // Attempt to open the script file
    File file = LittleFS.open(path, "r");
    if (!file) {
        Debug::println(2, "[SCRIPTMANAGER][LOAD] no file at " + String(path));
        return false;
    }

    size_t fileSize = file.size();
    Debug::println(4, "[SCRIPTMANAGER][LOAD] opening " + String(path) + " size=" + String(fileSize));

    // Parse JSON from file
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        Debug::println(1, "[SCRIPTMANAGER][LOAD][ERROR] JSON parse failed for " + String(path) + ": " + String(err.c_str()));
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    scriptCount = 0;
    Debug::println(3, "[SCRIPTMANAGER][LOAD] found array entries=" + String(arr.size()) + " in " + String(path));
    
    // Process each script object in the array
    for (JsonObject obj : arr) {
        if (scriptCount >= MAX_SCRIPTS) {
            Debug::println(2, "[SCRIPTMANAGER][LOAD][WARNING] Reached maximum script limit, ignoring remaining scripts");
            break;
        }
        
        Script& s = scripts[scriptCount++];
        s.id = obj["id"] | 0;
        s.name = obj["name"] | "";
        s.alias = obj["alias"] | "";
        Debug::println(3, "[SCRIPTMANAGER][LOAD] script id=" + String(s.id) + " name=" + s.name + " alias=" + s.alias + " from " + String(path));

        // Process aliases - support both array and string format for backwards compatibility
        if (obj["aliases"].is<JsonArray>()) {
            s.aliases = jsonArrayToString(obj["aliases"].as<JsonArray>());
        } else {
            DynamicJsonDocument aDoc(512);
            JsonArray a = aDoc.to<JsonArray>();
            if (s.alias.length()) a.add(s.alias);
            serializeJson(a, s.aliases);
        }

        // Process triggers - support both array and string format
        if (obj["triggers"].is<JsonArray>()) {
            String triggersJson;
            serializeJson(obj["triggers"], triggersJson);
            s.triggers = triggersJson;
        } else if (obj["triggers"].is<const char*>()) {
            s.triggers = obj["triggers"].as<String>();
        } else {
            s.triggers = "[]";
        }

        // Process actions - support both array and string format
        if (obj["actions"].is<JsonArray>()) {
            String actionsJson;
            serializeJson(obj["actions"], actionsJson);
            s.actions = actionsJson;
        } else if (obj["actions"].is<const char*>()) {
            s.actions = obj["actions"].as<String>();
        } else {
            s.actions = "[]";
        }
    }

    Debug::println(2, "[SCRIPTMANAGER] Loaded " + String(scriptCount) + " action(s) from " + String(path));
    return true;
}

/**
 * @brief Load scripts from primary or backup file
 * @return true if loading succeeded (from either file), false otherwise
 * 
 * This method attempts to load scripts from the primary file first,
 * then falls back to the backup file if the primary is not available.
 * If neither file is available, it initializes with an empty script set.
 */
bool ScriptManager::loadScripts() {
    // Try to load from primary script file
    if (LittleFS.exists(SCRIPT_FILE) && loadScriptsFromFile(SCRIPT_FILE)) {
        Debug::println(3, "[SCRIPTMANAGER][LOAD] Successfully loaded scripts from primary file");
        return true;
    }

    // If primary failed, try the backup file
    if (LittleFS.exists(SCRIPT_BACKUP_FILE) && loadScriptsFromFile(SCRIPT_BACKUP_FILE)) {
        Debug::println(2, "[SCRIPTMANAGER][LOAD] Falling back to backup scripts file: " + String(SCRIPT_BACKUP_FILE));
        return true;
    }

    // No script files found, initialize with empty set
    scriptCount = 0;
    Debug::println(2, "[SCRIPTMANAGER][LOAD] No script file found; starting with empty script set");
    return true;
}

/**
 * @brief Save scripts to a file
 * @param path Filesystem path to save the scripts
 * @param json JSON string containing the scripts
 * @return true if saving succeeded, false otherwise
 * 
 * This method saves scripts to the specified JSON file in LittleFS.
 * It creates necessary directories if they don't exist.
 * Writes the JSON content and reports the number of bytes written.
 */
bool ScriptManager::saveScriptsToFile(const char* path, const String& json) {
    String pathStr = path;
    int slash = pathStr.lastIndexOf('/');
    
    // Create directory structure if needed
    if (slash > 0) {
        String dir = pathStr.substring(0, slash);
        if (!LittleFS.exists(dir)) {
            Debug::println(3, "[SCRIPTMANAGER][SAVE] Creating directory: " + dir);
            LittleFS.mkdir(dir);
        }
    }

    // Open file for writing
    File file = LittleFS.open(path, "w");
    if (!file) {
        Debug::println(1, "[SCRIPTMANAGER][SAVE][ERROR] Cannot open " + String(path) + " for writing");
        return false;
    }
    
    // Write JSON content to file
    size_t bytesWritten = file.print(json);
    file.close();
    
    Debug::println(2, "[SCRIPTMANAGER][SAVE] Completed file=" + String(path) + " bytes=" + String(bytesWritten));
    return true;
}

/**
 * @brief Save all scripts to filesystem
 * @return true if saving succeeded, false otherwise
 * 
 * This method serializes all scripts to JSON format and saves them to both
 * the primary script file and the backup file. It handles different JSON
 * structures for backwards compatibility.
 */
bool ScriptManager::saveScripts() {
    Debug::println(3, "[SCRIPTMANAGER][SAVE] Starting save operation with " + String(scriptCount) + " scripts");
    
    // Create JSON document to hold all scripts
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    JsonArray arr = doc.to<JsonArray>();

    // Serialize each script to JSON
    for (int i = 0; i < scriptCount; ++i) {
        const Script& s = scripts[i];
        JsonObject obj = arr.createNestedObject();
        Debug::println(4, "[SCRIPTMANAGER][SAVE] Serializing script id=" + String(s.id) + " name=" + s.name);
        
        // Basic script properties
        obj["id"] = s.id;
        obj["name"] = s.name;
        obj["alias"] = s.alias;

        // Process aliases - support both array and string format
        DynamicJsonDocument aDoc(1024);
        JsonArray aliases = aDoc.to<JsonArray>();
        if (s.aliases.length()) {
            if (deserializeJson(aDoc, s.aliases)) {
                aliases = aDoc.to<JsonArray>();
            }
        }
        if (aliases.isNull() || aliases.size() == 0) {
            if (s.alias.length()) aliases.add(s.alias);
        }
        obj["aliases"] = aliases;

        // Process triggers - support both array and string format
        DynamicJsonDocument tDoc(4096);
        JsonArray triggers = tDoc.to<JsonArray>();
        if (s.triggers.length()) {
            if (deserializeJson(tDoc, s.triggers)) {
                triggers = tDoc.to<JsonArray>();
            }
        }
        obj["triggers"] = triggers;

        // Process actions - support both array and string format
        DynamicJsonDocument actionsDoc(8192);
        if (s.actions.length() > 0 && !deserializeJson(actionsDoc, s.actions)) {
            obj["actions"] = actionsDoc.as<JsonArray>();
        } else {
            JsonArray emptyActions = actionsDoc.to<JsonArray>();
            obj["actions"] = emptyActions;
        }
    }

    // Serialize the complete JSON array
    String json;
    serializeJson(arr, json);
    Debug::println(4, "[SCRIPTMANAGER][SAVE] JSON serialized, size=" + String(json.length()) + " bytes");

    // Save the primary first, then mirror the same JSON into the recovery file.
    // Keep both results separate so a partial save cannot be reported as healthy.
    bool ok = saveScriptsToFile(SCRIPT_FILE, json);
    if (ok) {
        Debug::println(3, "[SCRIPTMANAGER][SAVE] Successfully saved to primary file, now saving to backup");
        bool backupOk = saveScriptsToFile(SCRIPT_BACKUP_FILE, json);
        if (backupOk) {
            Debug::println(2, "[SCRIPTMANAGER][SAVE] Scripts successfully saved to primary and backup files");
        } else {
            Debug::println(1, "[SCRIPTMANAGER][SAVE][ERROR] Primary saved, but backup save failed");
        }
    } else {
        Debug::println(1, "[SCRIPTMANAGER][SAVE][ERROR] Failed to save scripts to primary file");
    }
    
    return ok;
}

/**
 * @brief Parse a JSON string into a JsonObject
 * @param json JSON string to parse
 * @param out Output JsonObject reference
 * @param doc Reference to a DynamicJsonDocument for parsing
 * @return true if parsing succeeded, false otherwise
 * 
 * This method attempts to parse a JSON string and validate that it's a JSON object.
 * Provides detailed error messages for debugging purposes.
 */
bool ScriptManager::parseScriptObject(const String& json, JsonObject& out, DynamicJsonDocument& doc) {
    // Attempt to parse JSON
    DeserializationError err = deserializeJson(doc, json);
    if (err || !doc.is<JsonObject>()) {
        Debug::println(1, "[SCRIPTMANAGER][ERROR] Invalid script JSON: " + String(err ? err.c_str() : "not an object") + 
                      " | Input: " + json.substring(0, _min(json.length(), 100)) + 
                      (json.length() > 100 ? "..." : ""));
        return false;
    }
    
    // Output the parsed object
    out = doc.as<JsonObject>();
    Debug::println(4, "[SCRIPTMANAGER][PARSE] Successfully parsed JSON object with " + String(out.size()) + " properties");
    return true;
}

/**
 * @brief Add a new script to the system
 * @param json JSON string containing the script definition
 * @return true if adding succeeded, false otherwise
 * 
 * This method adds a new script to the internal script array.
 * It validates the JSON, assigns a new ID, and saves the scripts to filesystem.
 */
bool ScriptManager::addScript(const String& json) {
    Debug::println(3, "[SCRIPTMANAGER][ADD] Incoming script JSON length=" + String(json.length()));
    
    // Check if we've reached the maximum number of scripts
    if (scriptCount >= MAX_SCRIPTS) {
        Debug::println(1, "[SCRIPTMANAGER][ADD][ERROR] Script limit reached (" + String(MAX_SCRIPTS) + ")");
        return false;
    }
    
    // Parse the JSON and validate it
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    JsonObject obj;
    if (!parseScriptObject(json, obj, doc)) {
        Debug::println(1, "[SCRIPTMANAGER][ADD][ERROR] Failed to parse script JSON");
        return false;
    }

    // Add the new script to our array
    Script& s = scripts[scriptCount++];
    s.id = nextScriptId();
    s.name = obj["name"] | "Unnamed action";
    Debug::println(2, "[SCRIPTMANAGER][ADD] Assigned id=" + String(s.id) + " name=" + s.name);
    s.alias = obj["alias"] | "";

    if (obj["aliases"].is<JsonArray>()) serializeJson(obj["aliases"], s.aliases);
    else {
        DynamicJsonDocument aDoc(512); JsonArray a = aDoc.to<JsonArray>();
        if (s.alias.length()) a.add(s.alias);
        serializeJson(a, s.aliases);
    }

    if (obj["triggers"].is<JsonArray>()) {
        String triggersJson;
        serializeJson(obj["triggers"], triggersJson);
        s.triggers = triggersJson;
    } else {
        s.triggers = "[]";
    }
    if (obj["actions"].is<JsonArray>()) {
        String actionsJson;
        serializeJson(obj["actions"], actionsJson);
        s.actions = actionsJson;
    } else {
        s.actions = "[]";
    }

    return saveScripts();
}

/**
 * @brief Update an existing script
 * @param scriptId ID of the script to update
 * @param json JSON string containing the updated script definition
 * @return true if update succeeded, false otherwise
 * 
 * This method finds an existing script by ID and updates its properties.
 * It validates the JSON, updates the script properties, and saves to filesystem.
 */
bool ScriptManager::updateScript(int scriptId, const String& json) {
    // Find the script by ID
    int idx = findScriptIndex(scriptId);
    if (idx < 0) {
        Debug::println(1, "[SCRIPTMANAGER][UPDATE][ERROR] Missing script id=" + String(scriptId));
        return false;
    }
    
    Debug::println(3, "[SCRIPTMANAGER][UPDATE] ScriptId=" + String(scriptId) + " payload length=" + String(json.length()));
    
    // Parse the JSON and validate it
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    JsonObject obj;
    if (!parseScriptObject(json, obj, doc)) {
        Debug::println(1, "[SCRIPTMANAGER][UPDATE][ERROR] Failed to parse script JSON for id=" + String(scriptId));
        return false;
    }

    // Get reference to the script to update
    Script& s = scripts[idx];
    String oldName = s.name;
    
    // Update script properties
    s.name = obj["name"] | s.name;
    s.alias = obj["alias"] | "";
    Debug::println(3, "[SCRIPTMANAGER][UPDATE] Updating script " + String(scriptId) + " (" + oldName + " -> " + s.name + ")");
    
    // Update aliases - support both array and string format
    if (obj["aliases"].is<JsonArray>()) {
        serializeJson(obj["aliases"], s.aliases);
    } else {
        DynamicJsonDocument aDoc(512); 
        JsonArray a = aDoc.to<JsonArray>();
        if (s.alias.length()) a.add(s.alias);
        serializeJson(a, s.aliases);
    }
    
    // Update triggers
    if (obj["triggers"].is<JsonArray>()) {
        String triggersJson;
        serializeJson(obj["triggers"], triggersJson);
        s.triggers = triggersJson;
    }
    
    // Update actions
    if (obj["actions"].is<JsonArray>()) {
        String actionsJson;
        serializeJson(obj["actions"], actionsJson);
        s.actions = actionsJson;
    }
    
    // Save the updated scripts to filesystem
    Debug::println(2, "[SCRIPTMANAGER][UPDATE] Script " + String(scriptId) + " updated, saving to filesystem");
    return saveScripts();
}

/**
 * @brief Delete a script by ID
 * @param scriptId ID of the script to delete
 * @return true if deletion succeeded, false otherwise
 * 
 * This method finds a script by ID, removes it from the array,
 * shifts all subsequent scripts to fill the gap, and saves to filesystem.
 */
bool ScriptManager::deleteScript(int scriptId) {
    // Find the script by ID
    int idx = findScriptIndex(scriptId);
    if (idx < 0) {
        Debug::println(1, "[SCRIPTMANAGER][DELETE][ERROR] Missing script id=" + String(scriptId));
        return false;
    }
    
    // Get the script name for logging before deletion
    String scriptName = scripts[idx].name;
    Debug::println(2, "[SCRIPTMANAGER][DELETE] Deleting script id=" + String(scriptId) + " (name: " + scriptName + ")");
    
    // Remove the script by shifting all subsequent scripts
    for (int i = idx; i < scriptCount - 1; ++i) {
        scripts[i] = scripts[i + 1];
    }
    
    // Decrement the script count
    --scriptCount;
    Debug::println(3, "[SCRIPTMANAGER][DELETE] Script deleted. Remaining scripts: " + String(scriptCount));
    
    // Save the updated scripts to filesystem
    return saveScripts();
}

/**
 * @brief Execute a script by ID
 * @param scriptId ID of the script to execute
 * @return true if execution succeeded (all actions successful), false otherwise
 * 
 * This method finds a script by ID, parses its actions, and executes each action.
 * Reports success/failure for each action and overall execution result.
 */
bool ScriptManager::executeScript(int scriptId) {
    // Find the script by ID
    int idx = findScriptIndex(scriptId);
    if (idx < 0) {
        Debug::println(1, "[SCRIPTMANAGER][EXECUTE][ERROR] Missing script id=" + String(scriptId));
        return false;
    }

    // Parse the actions JSON array
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(doc, scripts[idx].actions);
    if (err || !doc.is<JsonArray>()) {
        Debug::println(1, "[SCRIPTMANAGER][EXECUTE][ERROR] Invalid actions JSON for " + scripts[idx].name +
                       ": " + String(err ? err.c_str() : "not an array"));
        return false;
    }

    // Log script execution start
    JsonArray actions = doc.as<JsonArray>();
    Debug::println(2, "[SCRIPTMANAGER][EXECUTE] Starting script id=" + String(scriptId) + 
                   " name=" + scripts[idx].name + " with " + String(actions.size()) + " actions");
    
    // Execute each action in sequence
    bool allActionsOk = true;
    int actionIndex = 0;
    for (JsonObject action : actions) {
        String actionType = action["type"] | "unknown";
        bool actionOk = executeActionObject(action);
        
        // Log result of each action
        Debug::println(3, "[SCRIPTMANAGER][EXECUTE] Action #" + String(actionIndex++) + 
                       " type=" + actionType + " result=" + String(actionOk ? "SUCCESS" : "FAILED"));
        
        if (!actionOk) {
            allActionsOk = false;
            Debug::println(2, "[SCRIPTMANAGER][EXECUTE][WARNING] Action failed: " + actionType);
        }
    }
    
    // Log final result
    Debug::println(2, "[SCRIPTMANAGER][EXECUTE] Script execution completed with result: " + 
                   String(allActionsOk ? "SUCCESS" : "PARTIAL/FAILURE"));
    
    return allActionsOk;
}

/**
 * @brief Execute a single action from JSON string
 * @param actionJson JSON string containing the action definition
 * @return true if execution succeeded, false otherwise
 * 
 * This method parses a JSON string containing a single action
 * and delegates execution to executeActionObject.
 */
bool ScriptManager::executeAction(const String& actionJson) {
    Debug::println(4, "[SCRIPTMANAGER][ACTION] Parsing action JSON: " + actionJson.substring(0, _min(actionJson.length(), 100)) + 
                  (actionJson.length() > 100 ? "..." : ""));
    
    // Parse the JSON and validate it
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, actionJson);
    if (err || !doc.is<JsonObject>()) {
        Debug::println(1, "[SCRIPTMANAGER][ACTION][ERROR] Invalid action JSON: " + String(err ? err.c_str() : "not an object"));
        return false;
    }
    
    // Execute the action
    JsonObject actionObj = doc.as<JsonObject>();
    String actionType = actionObj["type"] | "unknown";
    Debug::println(3, "[SCRIPTMANAGER][ACTION] Executing action type: " + actionType);
    
    return executeActionObject(actionObj);
}

/**
 * @brief Execute a single action object
 * @param action JsonObject containing the action definition
 * @return true if execution succeeded, false otherwise
 * 
 * This method handles different types of actions (living, IR, RF) and
 * executes them using the appropriate hardware manager methods.
 * Supports LivingColors control, IR signal transmission, and RF signal transmission.
 */
bool ScriptManager::executeActionObject(JsonObject action) {
    // Get the action type
    String type = action["type"] | "";
    Debug::println(3, "[SCRIPTMANAGER][ACTION] Executing action type: " + type);

    // Handle LivingColors actions
    if (type == "living") {
        Debug::println(3, "[SCRIPTMANAGER][LIVING] Processing LivingColors action");
        
        // Extract action parameters
        String deviceId = action["deviceId"] | "";
        String color = action["color"] | "#ffffff";
        int intensity = constrain((int)(action["intensity"] | 100), 0, 100);
        
        Debug::println(4, "[SCRIPTMANAGER][LIVING] Device ID: " + deviceId + 
                       ", Color: " + color + ", Intensity: " + String(intensity));
        
        // Parse color and apply intensity
        uint8_t r, g, b;
        parseHexColor(color, r, g, b);
        r = (uint8_t)((uint16_t)r * intensity / 100);
        g = (uint8_t)((uint16_t)g * intensity / 100);
        b = (uint8_t)((uint16_t)b * intensity / 100);

        // Handle different device ID formats
        if (deviceId == "all" || deviceId == "0-11") {
            Debug::println(3, "[SCRIPTMANAGER][LIVING] Setting all LivingColors to RGB(" + String(r) + "," + String(g) + "," + String(b) + ")");
            hardware.living().setColorRGBAll(r, g, b);
            return true;
        }

        // Handle device ID range (e.g., "3-7")
        int dash = deviceId.indexOf('-');
        if (dash >= 0) {
            int start = _max(0, deviceId.substring(0, dash).toInt());
            int end = _min(LAMP_COUNT - 1, deviceId.substring(dash + 1).toInt());
            if (start > end) {
                Debug::println(1, "[SCRIPTMANAGER][LIVING][ERROR] Invalid device range: " + deviceId);
                return false;
            }
            Debug::println(3, "[SCRIPTMANAGER][LIVING] Setting LivingColors range " + deviceId + " to RGB(" + String(r) + "," + String(g) + "," + String(b) + ")");
            for (int i = start; i <= end; ++i) hardware.living().setColorRGB(i, r, g, b);
            return true;
        }

        // Handle single device ID
        int lamp = deviceId.toInt();
        if (lamp < 0 || lamp >= LAMP_COUNT) {
            Debug::println(1, "[SCRIPTMANAGER][LIVING][ERROR] Invalid lamp ID: " + String(lamp));
            return false;
        }
        Debug::println(4, "[SCRIPTMANAGER][LIVING] Setting LivingColors lamp " + String(lamp) + " to RGB(" + String(r) + "," + String(g) + "," + String(b) + ")");
        hardware.living().setColorRGB(lamp, r, g, b);
        return true;
    }

    // Handle IR actions
    if (type == "ir") {
        Debug::println(3, "[SCRIPTMANAGER][IR] Processing IR action");
        
        // Extract action parameters
        IRCommand cmd{};
        String codeText = action["code"] | "0";
        cmd.code = parseUnsigned(codeText);
        cmd.bits = action["bits"] | 32;
        cmd.protocol = action["protocol"] | 0;
        cmd.timestamp = millis();
        cmd.rawCodeLength = 0;
        cmd.hasRaw = false;
        
        Debug::println(4, "[SCRIPTMANAGER][IR] Code: 0x" + String(cmd.code, HEX) + 
                       ", Bits: " + String(cmd.bits) + 
                       ", Protocol: " + String(cmd.protocol));
        
        if (cmd.code == 0) {
            Debug::println(1, "[SCRIPTMANAGER][IR][ERROR] Invalid IR code: 0");
            return false;
        }
        
        // Send the IR command
        hardware.ir().send(cmd);
        Debug::println(3, "[SCRIPTMANAGER][IR] IR command sent successfully");
        return true;
    }

    // Handle RF actions
    if (type == "rf") {
        Debug::println(3, "[SCRIPTMANAGER][RF] Processing RF action");
        
        // Extract action parameters
        String codeText = action["code"] | "0";
        unsigned long value = parseUnsigned(codeText);
        int bits = action["bits"] | 24;
        int protocol = action["protocol"] | 1;
        int pulse = action["pulse"] | 350;
        
        Debug::println(4, "[SCRIPTMANAGER][RF] Code: 0x" + String(value, HEX) + 
                       ", Bits: " + String(bits) + 
                       ", Protocol: " + String(protocol) + 
                       ", Pulse: " + String(pulse));
        
        if (value == 0) {
            Debug::println(1, "[SCRIPTMANAGER][RF][ERROR] Invalid RF code: 0");
            return false;
        }

        // Configure and send RF command
        Debug::println(3, "[SCRIPTMANAGER][RF] Sending 0x" + String(value, HEX) +
                       " bits=" + String(bits) + " protocol=" + String(protocol) +
                       " pulse=" + String(pulse));
                       
        hardware.rf().disableReceive();
        hardware.rf().setProtocol(protocol);
        hardware.rf().setPulseLength(pulse);
        hardware.rf().send(value, bits);
        hardware.rf().enableReceive();
        Debug::println(3, "[SCRIPTMANAGER][RF] RF command sent successfully");
        return true;
    }

    // Unknown action type
    Debug::println(1, "[SCRIPTMANAGER][ERROR] Unknown action type: " + type);
    return false;
}

/**
 * @brief Get all scripts as a JSON string
 * @return String containing all scripts in JSON format
 * 
 * This method serializes all scripts to a JSON string for web UI display
 * or external transfer. It handles different JSON formats for compatibility.
 */
String ScriptManager::getScriptsAsJson() {
    if (DEBUG_LEVEL >= 3) {
        Debug::println(4, "[SCRIPTMANAGER][EXPORT] Starting export of " + String(scriptCount) + " scripts");
    }
    
    // Create JSON document to hold all scripts
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    JsonArray arr = doc.to<JsonArray>();
    
    // Process each script
    for (int i = 0; i < scriptCount; ++i) {
        const Script& s = scripts[i];
        JsonObject obj = arr.createNestedObject();
        
        // Basic script properties
        obj["id"] = s.id;
        obj["name"] = s.name;
        obj["alias"] = s.alias;
        if (DEBUG_LEVEL >= 3) {
            Debug::println(4, "[SCRIPTMANAGER][EXPORT] Processing script id=" + String(s.id) + " name=" + s.name);
        }

        // Process aliases - support both array and string format
        DynamicJsonDocument aDoc(1024);
        if (!s.aliases.length() || deserializeJson(aDoc, s.aliases)) {
            JsonArray a = aDoc.to<JsonArray>();
            if (s.alias.length()) a.add(s.alias);
        }
        obj["aliases"] = aDoc.as<JsonArray>();

        // Process triggers - support both array and string format
        DynamicJsonDocument tDoc(4096);
        if (!s.triggers.length() || deserializeJson(tDoc, s.triggers)) tDoc.to<JsonArray>();
        obj["triggers"] = tDoc.as<JsonArray>();

        // Process actions - support both array and string format
        DynamicJsonDocument actDoc(8192);
        if (s.actions.length() > 0 && !deserializeJson(actDoc, s.actions)) {
            obj["actions"] = actDoc.as<JsonArray>();
        } else {
            JsonArray emptyActions = actDoc.to<JsonArray>();
            obj["actions"] = emptyActions;
        }
    }
    
    // Serialize to string and return
    String out;
    serializeJson(arr, out);
    if (DEBUG_LEVEL >= 3) {
        Debug::println(4, "[SCRIPTMANAGER][EXPORT] Export completed, JSON size: " + String(out.length()) + " bytes");
    }
    return out;
}

/**
 * @brief Get the raw content of the primary script file
 * @return String containing the raw file content, or empty string if file not found
 * 
 * This method reads the raw content of the primary script file directly
 * from LittleFS without any parsing or processing.
 */
String ScriptManager::getRawScriptsFile() const {
    // Attempt to open the primary script file
    File file = LittleFS.open(SCRIPT_FILE, "r");
    if (!file) {
        Debug::println(2, "[SCRIPTMANAGER][RAW] Could not open script file: " + String(SCRIPT_FILE));
        return "";
    }

    // Read the entire file content
    String contents = file.readString();
    file.close();
    
    Debug::println(4, "[SCRIPTMANAGER][RAW] Read " + String(contents.length()) + " bytes from script file");
    return contents;
}

/**
 * @brief Get a specific script by ID as JSON string
 * @param scriptId ID of the script to retrieve
 * @return String containing the script in JSON format, or empty object if not found
 * 
 * This method finds a specific script by ID and returns it as a JSON string.
 * It first finds the script index, then creates a JSON document with all scripts,
 * and finally extracts the requested script from the JSON.
 */
String ScriptManager::getScriptById(int scriptId) {
    // Find the script index
    int idx = findScriptIndex(scriptId);
    if (idx < 0) {
        Debug::println(2, "[SCRIPTMANAGER][GET] Script not found with id=" + String(scriptId));
        return "{}";
    }
    
    Debug::println(4, "[SCRIPTMANAGER][GET] Retrieving script id=" + String(scriptId) + " name=" + scripts[idx].name);
    
    // Get all scripts as JSON and find the requested one
    DynamicJsonDocument all(JSON_DOC_SIZE);
    deserializeJson(all, getScriptsAsJson());
    
    for (JsonObject obj : all.as<JsonArray>()) {
        if ((int)(obj["id"] | 0) == scriptId) {
            String out;
            serializeJson(obj, out);
            Debug::println(4, "[SCRIPTMANAGER][GET] Script JSON size: " + String(out.length()) + " bytes");
            return out;
        }
    }
    
    Debug::println(2, "[SCRIPTMANAGER][GET][WARNING] Script with id=" + String(scriptId) + " not found in JSON export");
    return "{}";
}

/**
 * @brief Start capturing the next system event
 * @param filter Event source filter (e.g., "RF", "IR", "KAKU", "ANY")
 * 
 * This method enables event capture mode, which records the next system event
 * that matches the specified filter. This is useful for learning new triggers
 * for scripts without manually identifying event details.
 */
void ScriptManager::startCapture(const String& filter) {
    // Set and normalize the filter
    captureFilter = filter;
    captureFilter.toUpperCase();
    
    // Reset capture state
    captureWaiting = true;
    capturedSource = capturedIdentifier = capturedRawData = "";
    captureTime = millis();
    
    // Log the capture start
    Debug::println(2, "[SCRIPTMANAGER][LEARN] Starting event capture with filter: " + captureFilter);
    Debug::println(3, "[SCRIPTMANAGER][LEARN] Waiting for " + captureFilter + " event...");
    Debug::println(2, "[SCRIPTMANAGER][LEARN] Trigger capture is active; next RF/IR/storage event will be recorded");
}

/**
 * @brief Get the current event capture status
 * @return JSON string containing capture state information
 * 
 * This method returns a JSON object with the current capture status,
 * including whether capture is active, the filter being used,
 * and any captured event details.
 */
String ScriptManager::getCaptureStatus() const {
    Debug::println(4, "[SCRIPTMANAGER][CAPTURE] Getting capture status");
    
    // Create JSON document with capture status
    DynamicJsonDocument doc(1024);
    doc["waiting"] = captureWaiting;
    doc["filter"] = captureFilter;
    doc["source"] = capturedSource;
    doc["identifier"] = capturedIdentifier;
    doc["rawData"] = capturedRawData;
    doc["ageMs"] = captureTime ? (millis() - captureTime) : 0;
    
    // Add debug information about capture state
    if (captureWaiting) {
        Debug::println(3, "[SCRIPTMANAGER][CAPTURE] Capture is active, waiting for " + captureFilter + " event");
    } else if (capturedSource.length() > 0) {
        Debug::println(3, "[SCRIPTMANAGER][CAPTURE] Last captured: " + capturedSource + " -> " + capturedIdentifier);
    } else {
        Debug::println(3, "[SCRIPTMANAGER][CAPTURE] No active capture or previous capture");
    }
    
    // Serialize and return the status
    String out;
    serializeJson(doc, out);
    return out;
}

/**
 * @brief Check if an event matches the current capture filter
 * @param evt The system event to check
 * @return true if the event matches the capture filter, false otherwise
 * 
 * This method determines if a system event should be captured based on the
 * current capture filter. Special case: "ANY" filter matches all events.
 */
bool ScriptManager::eventMatchesCapture(const SystemEvent& evt) const {
    // Special case: "ANY" filter matches all events
    if (captureFilter == "ANY") {
        Debug::println(4, "[SCRIPTMANAGER][CAPTURE] Event matches \"ANY\" filter: " + evt.source + " -> " + evt.identifier);
        return true;
    }
    
    // Check if the event source matches the filter
    bool matches = evt.source.equalsIgnoreCase(captureFilter);
    Debug::println(4, "[SCRIPTMANAGER][CAPTURE] Checking if " + evt.source + " matches filter " + captureFilter + ": " + String(matches ? "YES" : "NO"));
    
    return matches;
}

/**
 * @brief Check if a system event matches any trigger in a trigger array
 * @param triggers JSON array of trigger objects to check against
 * @param evt The system event to check
 * @return true if the event matches any trigger, false otherwise
 * 
 * This method compares a system event against an array of triggers,
 * checking if both the source and identifier match for any trigger.
 * Case-insensitive comparison is used for both source and identifier.
 */
bool ScriptManager::matchesTrigger(JsonArray triggers, const SystemEvent& evt) const {
    Debug::println(4, "[SCRIPTMANAGER][TRIGGER] Checking event " + evt.source + " -> " + evt.identifier + 
                   " against " + String(triggers.size()) + " triggers");
    
    // Iterate through each trigger and check for a match
    for (JsonObject trigger : triggers) {
        String source = trigger["source"] | "";
        String identifier = trigger["identifier"] | "";
        
        Debug::println(4, "[SCRIPTMANAGER][TRIGGER] Checking trigger: " + source + " -> " + identifier);
        
        // Check if both source and identifier match (case-insensitive)
        if (source.equalsIgnoreCase(evt.source) && identifier.equalsIgnoreCase(evt.identifier)) {
            Debug::println(3, "[SCRIPTMANAGER][TRIGGER] Match found: " + source + " -> " + identifier);
            return true;
        }
    }
    
    Debug::println(4, "[SCRIPTMANAGER][TRIGGER] No matching trigger found");
    return false;
}

/**
 * @brief Handle incoming system events
 * @param evt The system event to handle
 * 
 * This method processes incoming system events by:
 * 1. Checking if event capture mode is active and the event matches the filter
 * 2. Handling KAKU events with scene mapping if sceneManager is available
 * 3. Checking all scripts for matching triggers and executing them
 */
void ScriptManager::handleSystemEvent(const SystemEvent& evt) {
    Debug::println(4, "[SCRIPTMANAGER][EVENT] Processing event: " + evt.source + " -> " + evt.identifier);

    // Check if we're in capture mode and this event matches our filter
    if (captureWaiting && eventMatchesCapture(evt)) {
        Debug::println(2, "[SCRIPTMANAGER][LEARN] Capturing event: " + evt.source + " -> " + evt.identifier);

        // Store the captured event details
        capturedSource = evt.source;
        capturedIdentifier = evt.identifier;
        capturedRawData = evt.rawData;
        captureWaiting = false;

        // Log the captured event
        Debug::println(2, "[SCRIPTMANAGER][LEARN] Captured " + evt.source + " -> " + evt.identifier);
        Debug::println(3, "[SCRIPTMANAGER][LEARN] Raw data: " + evt.rawData + " (filter=" + captureFilter + ")");
        return;
    }

    // Check if this is a KAKU event and if sceneManager is available, lookup bound scene
    if (evt.source == "KAKU" && sceneManager != nullptr) {
        // evt.identifier format from KakuDecoder: "house_button" e.g., "D_9"
        int underscorePos = evt.identifier.indexOf('_');
        if (underscorePos > 0) {
            char house = evt.identifier[0];
            uint8_t button = evt.identifier.substring(underscorePos + 1).toInt();
            
            int sceneId = sceneManager->findSceneByKaku(house, button);
            if (sceneId > 0) {
                Debug::println(2, "[SCRIPTMANAGER][KAKU->SCENE] " + evt.identifier + " -> Scene #" + String(sceneId));
                sceneManager->applyScene(sceneId);
                return;  // Scene matched, don't check scripts
            }
        }
    }

    for (int i = 0; i < scriptCount; ++i) {
        DynamicJsonDocument tDoc(4096);
        if (deserializeJson(tDoc, scripts[i].triggers)) continue;
        JsonArray triggers = tDoc.as<JsonArray>();
        if (matchesTrigger(triggers, evt)) {
            Debug::println(2, "[SCRIPTMANAGER][TRIGGER] " + evt.source + "/" + evt.identifier +
                           " -> " + scripts[i].name);
            executeScript(scripts[i].id);
        }
    }
}

/**
 * @brief Parse an unsigned integer from a string
 * @param text String to parse
 * @return Parsed unsigned integer value
 * 
 * This method parses an unsigned integer from a string, supporting both
 * decimal and hexadecimal formats. Hexadecimal values should be prefixed
 * with "0x" or "0X". Falls back to hex parsing for malformed input.
 */
uint32_t ScriptManager::parseUnsigned(const String& text) {
    String s = text; 
    s.trim();
    
    if (s.length() == 0) {
        Debug::println(4, "[SCRIPTMANAGER][PARSE] Empty string, returning 0");
        return 0;
    }

    // Accept both decimal and hexadecimal values, but default to decimal unless
    // the caller explicitly passes a 0x/0X prefixed number.
    if (s.startsWith("0x") || s.startsWith("0X")) {
        s = s.substring(2);
        uint32_t result = (uint32_t)strtoul(s.c_str(), nullptr, 16);
        Debug::println(4, "[SCRIPTMANAGER][PARSE] Parsed hex: 0x" + String(result, HEX) + " from " + text);
        return result;
    }

    // Try decimal parsing first
    char* end = nullptr;
    unsigned long value = strtoul(s.c_str(), &end, 10);
    if (end != nullptr && *end == '\0') {
        Debug::println(4, "[SCRIPTMANAGER][PARSE] Parsed decimal: " + String(value) + " from " + text);
        return (uint32_t)value;
    }

    // Fallback for legacy or malformed input: try hex as a last resort.
    uint32_t fallback = (uint32_t)strtoul(s.c_str(), nullptr, 16);
    Debug::println(3, "[SCRIPTMANAGER][PARSE][WARNING] Fallback hex parsing: 0x" + String(fallback, HEX) + " from " + text);
    return fallback;
}

/**
 * @brief Parse a hex color string to RGB values
 * @param color Hex color string in format "#RRGGBB"
 * @param r Output red component (0-255)
 * @param g Output green component (0-255)
 * @param b Output blue component (0-255)
 * 
 * This method parses a hex color string in the format "#RRGGBB" and
 * extracts the red, green, and blue components. If the format is invalid,
 * defaults to white (255, 255, 255).
 */
void ScriptManager::parseHexColor(const String& color, uint8_t& r, uint8_t& g, uint8_t& b) {
    String s = color; 
    s.trim();
    
    // Check if the color string has the correct format (#RRGGBB)
    if (s.length() == 7 && s[0] == '#') {
        // Parse each color component
        r = (uint8_t)strtoul(s.substring(1, 3).c_str(), nullptr, 16);
        g = (uint8_t)strtoul(s.substring(3, 5).c_str(), nullptr, 16);
        b = (uint8_t)strtoul(s.substring(5, 7).c_str(), nullptr, 16);
        
        Debug::println(4, "[SCRIPTMANAGER][COLOR] Parsed color " + color + " to RGB(" + 
                       String(r) + "," + String(g) + "," + String(b) + ")");
    } else {
        // Invalid format, default to white
        r = g = b = 255;
        Debug::println(3, "[SCRIPTMANAGER][COLOR][WARNING] Invalid color format: " + color + ", defaulting to white");
    }
}

/**
 * @brief Convert a JsonArray to a String
 * @param array The JsonArray to convert
 * @return String containing the JSON array representation
 * 
 * This method serializes a JsonArray to a string for storage or transfer.
 */
String ScriptManager::jsonArrayToString(JsonArray array) {
    String out;
    serializeJson(array, out);
    Debug::println(4, "[SCRIPTMANAGER][JSON] Converted JsonArray to string, length=" + String(out.length()));
    return out;
}

/**
 * @brief Find a script index by ID
 * @param scriptId The ID of the script to find
 * @return Index of the script in the scripts array, or -1 if not found
 * 
 * This method searches the internal scripts array for a script with
 * the specified ID and returns its index.
 */
int ScriptManager::findScriptIndex(int scriptId) {
    Debug::println(4, "[SCRIPTMANAGER][FIND] Searching for script ID: " + String(scriptId));
    
    for (int i = 0; i < scriptCount; ++i) {
        if (scripts[i].id == scriptId) {
            Debug::println(4, "[SCRIPTMANAGER][FIND] Found script ID " + String(scriptId) + " at index " + String(i));
            return i;
        }
    }
    
    Debug::println(3, "[SCRIPTMANAGER][FIND] Script ID " + String(scriptId) + " not found");
    return -1;
}

/**
 * @brief Calculate the next available script ID
 * @return The next available script ID
 * 
 * This method finds the highest script ID currently in use and
 * returns the next sequential ID for a new script.
 */
int ScriptManager::nextScriptId() const {
    int maxId = 0;
    
    // Find the highest script ID currently in use
    for (int i = 0; i < scriptCount; ++i) {
        if (scripts[i].id > maxId) {
            maxId = scripts[i].id;
        }
    }
    
    int nextId = maxId + 1;
    Debug::println(4, "[SCRIPTMANAGER][ID] Next available script ID: " + String(nextId) + " (max current ID: " + String(maxId) + ")");
    
    return nextId;
}
