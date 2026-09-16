#include "ScriptManager.h"

#include "../Debug/Debug.h"


// ============================================================================
// Constructor
// ============================================================================

ScriptManager::ScriptManager(
    CommandSink& commandSink
)
    : commandSink(commandSink)
{
    Debug::println(
        2,
        "[ScriptManager] Constructor"
    );
}


// ============================================================================
// Begin
// ============================================================================

void ScriptManager::begin()
{
    Debug::println(
        2,
        "[ScriptManager] Initializing..."
    );

    Debug::println(
        2,
        "[ScriptManager] Script file: " +
        String(SCRIPT_FILE)
    );

    // if (!LittleFS.begin())
    // {
    //     Debug::println(
    //         1,
    //         "[ScriptManager] LittleFS mount failed!"
    //     );

    //     return;
    // }

    // Debug::println(
    //     2,
    //     "[ScriptManager] LittleFS mounted."
    // );

    loadScripts();

    Debug::println(
        2,
        "[ScriptManager] Loaded " +
        String(scriptCount) +
        " scripts."
    );
}


// ============================================================================
// Load
// ============================================================================

bool ScriptManager::loadScripts()
{
    Debug::println(
        2,
        "[ScriptManager] Loading scripts..."
    );

    if (Storage::exists(SCRIPT_FILE))
    {
        Debug::println(
            2,
            "[ScriptManager] Found " +
            String(SCRIPT_FILE)
        );

        if (loadScriptsFromFile(SCRIPT_FILE))
        {
            return true;
        }

        Debug::println(
            1,
            "[ScriptManager] Primary script file failed."
        );
    }
    else
    {
        Debug::println(
            2,
            "[ScriptManager] Primary script file not found."
        );
    }

    if (Storage::exists(SCRIPT_BACKUP_FILE))
    {
        Debug::println(
            2,
            "[ScriptManager] Loading backup script file."
        );

        return loadScriptsFromFile(
            SCRIPT_BACKUP_FILE
        );
    }

    scriptCount = 0;

    Debug::println(
        2,
        "[ScriptManager] No scripts file found."
    );

    return false;
}


// ============================================================================
// Load from file
// ============================================================================

bool ScriptManager::loadScriptsFromFile(
    const char* path
)
{
    Debug::println(
        2,
        "[ScriptManager] Opening: " +
        String(path)
    );

    // Read file using Storage class
    String fileContent = Storage::read(path);

    if (fileContent.isEmpty())
    {
        Debug::println(
            1,
            "[ScriptManager] Cannot open " +
            String(path)
        );

        return false;
    }

    Debug::println(
        2,
        "[ScriptManager] File size: " +
        String(fileContent.length()) +
        " bytes"
    );

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
            "[ScriptManager] JSON error: " +
            String(error.c_str())
        );

        return false;
    }

    if (!doc.is<JsonArray>())
    {
        Debug::println(
            1,
            "[ScriptManager] scripts.json is not an array."
        );

        return false;
    }

    JsonArray array =
        doc.as<JsonArray>();

    Debug::println(
        2,
        "[ScriptManager] JSON contains " +
        String(array.size()) +
        " script entries."
    );

    scriptCount = 0;

    for (JsonObject obj : array)
    {
        if (scriptCount >= MAX_SCRIPTS)
        {
            Debug::println(
                1,
                "[ScriptManager] MAX_SCRIPTS reached."
            );

            break;
        }

        Script& script =
            scripts[scriptCount];

        // --------------------------------------------------------------------
        // ID
        // --------------------------------------------------------------------

        script.id =
            obj["id"] | nextScriptId();

        // --------------------------------------------------------------------
        // Name
        // --------------------------------------------------------------------

        script.name =
            obj["name"] | "";

        // --------------------------------------------------------------------
        // Aliases
        // --------------------------------------------------------------------

        script.aliases = "";

        if (obj["aliases"].is<JsonArray>())
        {
            serializeJson(
                obj["aliases"],
                script.aliases
            );
        }
        else if (obj["alias"].is<const char*>())
        {
            DynamicJsonDocument aliasDoc(
                512
            );

            JsonArray aliases =
                aliasDoc.to<JsonArray>();

            String alias =
                obj["alias"] | "";

            if (alias.length())
            {
                aliases.add(alias);
            }

            serializeJson(
                aliases,
                script.aliases
            );
        }

        // --------------------------------------------------------------------
        // Commands
        // --------------------------------------------------------------------

        script.commands = "";

        if (obj["commands"].is<JsonArray>())
        {
            serializeJson(
                obj["commands"],
                script.commands
            );
        }
        else if (obj["actions"].is<JsonArray>())
        {
            /*
             * Temporary backwards compatibility.
             *
             * Allows old scripts using "actions"
             * to load while the project is migrated.
             */
            serializeJson(
                obj["actions"],
                script.commands
            );
        }

        // --------------------------------------------------------------------
        // Debug information
        // --------------------------------------------------------------------

        Debug::println(
            2,
            "[ScriptManager] Loaded script #" +
            String(script.id) +
            ": " +
            script.name
        );

        Debug::println(
            2,
            "[ScriptManager] Aliases: " +
            script.aliases
        );

        Debug::println(
            2,
            "[ScriptManager] Commands JSON: " +
            script.commands
        );

        scriptCount++;
    }

    Debug::println(
        2,
        "[ScriptManager] Loaded " +
        String(scriptCount) +
        " scripts from " +
        String(path)
    );

    return true;
}


// ============================================================================
// Find script by ID
// ============================================================================

int ScriptManager::findScriptIndex(
    int scriptId
) const
{
    for (int i = 0; i < scriptCount; i++)
    {
        if (scripts[i].id == scriptId)
        {
            return i;
        }
    }

    return -1;
}


// ============================================================================
// Find script by name / alias
// ============================================================================

int ScriptManager::findScriptIndex(
    const String& nameOrAlias
) const
{
    String search =
        nameOrAlias;

    search.trim();

    Debug::println(
        2,
        "[ScriptManager] Looking for script: " +
        search
    );

    for (int i = 0; i < scriptCount; i++)
    {
        if (scripts[i].name.equalsIgnoreCase(search))
        {
            Debug::println(
                2,
                "[ScriptManager] Name match: " +
                scripts[i].name
            );

            return i;
        }

        if (aliasesContain(
                scripts[i].aliases,
                search))
        {
            Debug::println(
                2,
                "[ScriptManager] Alias match: " +
                scripts[i].name
            );

            return i;
        }
    }

    Debug::println(
        2,
        "[ScriptManager] No script match."
    );

    return -1;
}


// ============================================================================
// Alias lookup
// ============================================================================

bool ScriptManager::aliasesContain(
    const String& aliasesJson,
    const String& value
)
{
    DynamicJsonDocument doc(
        1024
    );

    DeserializationError error =
        deserializeJson(
            doc,
            aliasesJson
        );

    if (error || !doc.is<JsonArray>())
    {
        return false;
    }

    JsonArray aliases =
        doc.as<JsonArray>();

    for (JsonVariant alias : aliases)
    {
        String aliasString =
            alias.as<String>();

        if (aliasString.equalsIgnoreCase(value))
        {
            return true;
        }
    }

    return false;
}


// ============================================================================
// Run by ID
// ============================================================================

bool ScriptManager::runScript(
    int scriptId
)
{
    Debug::println(
        2,
        "[ScriptManager] runScript(ID): " +
        String(scriptId)
    );

    int index =
        findScriptIndex(
            scriptId
        );

    if (index < 0)
    {
        Debug::println(
            1,
            "[ScriptManager] Script ID not found: " +
            String(scriptId)
        );

        return false;
    }

    return executeScriptAtIndex(
        index
    );
}


// ============================================================================
// Run by name / alias
// ============================================================================

bool ScriptManager::runScript(
    const String& nameOrAlias
)
{
    Debug::println(
        2,
        "[ScriptManager] runScript(name): " +
        nameOrAlias
    );

    int index =
        findScriptIndex(
            nameOrAlias
        );

    if (index < 0)
    {
        Debug::println(
            1,
            "[ScriptManager] Script not found: " +
            nameOrAlias
        );

        return false;
    }

    return executeScriptAtIndex(
        index
    );
}


// ============================================================================
// Has script
// ============================================================================

bool ScriptManager::hasScript(
    const String& nameOrAlias
) const
{
    return findScriptIndex(
        nameOrAlias
    ) >= 0;
}


// ============================================================================
// Execute script
// ============================================================================

bool ScriptManager::executeScriptAtIndex(
    int index
)
{
    if (index < 0 || index >= scriptCount)
    {
        Debug::println(
            1,
            "[ScriptManager] Invalid script index: " +
            String(index)
        );

        return false;
    }

    Script& script =
        scripts[index];

    Debug::println(
        2,
        "================================================"
    );

    Debug::println(
        2,
        "[ScriptManager] Running script: " +
        script.name
    );

    Debug::println(
        2,
        "[ScriptManager] Script ID: " +
        String(script.id)
    );

    Debug::println(
        2,
        "[ScriptManager] Commands JSON: " +
        script.commands
    );

    // ------------------------------------------------------------------------
    // Parse command array
    // ------------------------------------------------------------------------

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    DeserializationError error =
        deserializeJson(
            doc,
            script.commands
        );

    if (error)
    {
        Debug::println(
            1,
            "[ScriptManager] Command JSON error: " +
            String(error.c_str())
        );

        return false;
    }

    if (!doc.is<JsonArray>())
    {
        Debug::println(
            1,
            "[ScriptManager] Script commands are not an array."
        );

        return false;
    }

    JsonArray commands =
        doc.as<JsonArray>();

    Debug::println(
        2,
        "[ScriptManager] Command count: " +
        String(commands.size())
    );

    // ------------------------------------------------------------------------
    // Execute commands
    // ------------------------------------------------------------------------

    bool success = true;

    int commandIndex = 0;

    for (JsonVariant commandValue : commands)
    {
        Debug::println(
            2,
            "[ScriptManager] Processing command #" +
            String(commandIndex)
        );

        if (!commandValue.is<const char*>())
        {
            Debug::println(
                1,
                "[ScriptManager] Invalid command entry #" +
                String(commandIndex)
            );

            success = false;
            commandIndex++;
            continue;
        }

        String command =
            commandValue.as<String>();

        command.trim();

        if (!command.length())
        {
            Debug::println(
                2,
                "[ScriptManager] Empty command #" +
                String(commandIndex)
            );

            commandIndex++;
            continue;
        }

        Debug::println(
            2,
            "[ScriptManager] Command #" +
            String(commandIndex) +
            ": " +
            command
        );

        // --------------------------------------------------------------------
        // Send canonical command to Core
        // --------------------------------------------------------------------

        Debug::println(
            2,
            "[ScriptManager] → CommandSink"
        );

        bool commandResult =
            commandSink.executeCommand(
                command
            );

        Debug::println(
            2,
            "[ScriptManager] ← CommandSink result: " +
            String(commandResult ? "SUCCESS" : "FAILED")
        );

        if (!commandResult)
        {
            Debug::println(
                1,
                "[ScriptManager] Command failed: " +
                command
            );

            /*
             * IMPORTANT:
             *
             * Continue executing the remaining commands.
             *
             * This lets us see every command during development,
             * even if one command is not implemented yet.
             */
            success = false;
        }

        commandIndex++;
    }

    // ------------------------------------------------------------------------
    // Finished
    // ------------------------------------------------------------------------

    Debug::println(
        2,
        "[ScriptManager] Script completed: " +
        script.name
    );

    Debug::println(
        2,
        "[ScriptManager] Overall result: " +
        String(success ? "SUCCESS" : "FAILED")
    );

    Debug::println(
        2,
        "================================================"
    );

    return success;
}


// ============================================================================
// Save
// ============================================================================

bool ScriptManager::saveScripts()
{
    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    JsonArray array =
        doc.to<JsonArray>();

    for (int i = 0; i < scriptCount; i++)
    {
        Script& script =
            scripts[i];

        JsonObject obj =
            array.createNestedObject();

        obj["id"] =
            script.id;

        obj["name"] =
            script.name;

        // --------------------------------------------------------------------
        // aliases
        // --------------------------------------------------------------------

        DynamicJsonDocument aliasDoc(
            1024
        );

        DeserializationError aliasError =
            deserializeJson(
                aliasDoc,
                script.aliases
            );

        if (!aliasError &&
            aliasDoc.is<JsonArray>())
        {
            obj["aliases"] =
                aliasDoc.as<JsonArray>();
        }

        // --------------------------------------------------------------------
        // commands
        // --------------------------------------------------------------------

        DynamicJsonDocument commandDoc(
            4096
        );

        DeserializationError commandError =
            deserializeJson(
                commandDoc,
                script.commands
            );

        if (!commandError &&
            commandDoc.is<JsonArray>())
        {
            obj["commands"] =
                commandDoc.as<JsonArray>();
        }
    }

    String json;

    serializeJsonPretty(
        doc,
        json
    );

    return saveScriptsToFile(
        SCRIPT_FILE,
        json
    );
}


// ============================================================================
// Save to file
// ============================================================================

bool ScriptManager::saveScriptsToFile(
    const char* path,
    const String& json
)
{
    // Write file using Storage class
    return Storage::write(path, json);
}


// ============================================================================
// Add
// ============================================================================

bool ScriptManager::addScript(
    const String& json
)
{
    if (scriptCount >= MAX_SCRIPTS)
        return false;

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    DeserializationError error =
        deserializeJson(
            doc,
            json
        );

    if (error || !doc.is<JsonObject>())
        return false;

    JsonObject obj =
        doc.as<JsonObject>();

    Script& script =
        scripts[scriptCount];

    script.id =
        obj["id"] | nextScriptId();

    script.name =
        obj["name"] | "";

    if (!script.name.length())
        return false;

    script.aliases = "[]";

    if (obj["aliases"].is<JsonArray>())
    {
        serializeJson(
            obj["aliases"],
            script.aliases
        );
    }

    script.commands = "[]";

    if (obj["commands"].is<JsonArray>())
    {
        serializeJson(
            obj["commands"],
            script.commands
        );
    }

    scriptCount++;

    return saveScripts();
}


// ============================================================================
// Update
// ============================================================================

bool ScriptManager::updateScript(
    int scriptId,
    const String& json
)
{
    int index =
        findScriptIndex(
            scriptId
        );

    if (index < 0)
        return false;

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    DeserializationError error =
        deserializeJson(
            doc,
            json
        );

    if (error || !doc.is<JsonObject>())
        return false;

    JsonObject obj =
        doc.as<JsonObject>();

    Script& script =
        scripts[index];

    script.name =
        obj["name"] | script.name;

    if (obj["aliases"].is<JsonArray>())
    {
        serializeJson(
            obj["aliases"],
            script.aliases
        );
    }

    if (obj["commands"].is<JsonArray>())
    {
        serializeJson(
            obj["commands"],
            script.commands
        );
    }

    return saveScripts();
}


// ============================================================================
// Delete
// ============================================================================

bool ScriptManager::deleteScript(
    int scriptId
)
{
    int index =
        findScriptIndex(
            scriptId
        );

    if (index < 0)
        return false;

    for (int i = index;
         i < scriptCount - 1;
         i++)
    {
        scripts[i] =
            scripts[i + 1];
    }

    scriptCount--;

    return saveScripts();
}


// ============================================================================
// IDs
// ============================================================================

int ScriptManager::nextScriptId() const
{
    int maxId = 0;

    for (int i = 0;
         i < scriptCount;
         i++)
    {
        if (scripts[i].id > maxId)
        {
            maxId =
                scripts[i].id;
        }
    }

    return maxId + 1;
}


// ============================================================================
// JSON output
// ============================================================================

String ScriptManager::getScriptsAsJson()
{
    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    JsonArray array =
        doc.to<JsonArray>();

    for (int i = 0;
         i < scriptCount;
         i++)
    {
        Script& script =
            scripts[i];

        JsonObject obj =
            array.createNestedObject();

        obj["id"] =
            script.id;

        obj["name"] =
            script.name;

        DynamicJsonDocument aliasDoc(
            1024
        );

        if (!deserializeJson(
                aliasDoc,
                script.aliases))
        {
            obj["aliases"] =
                aliasDoc.as<JsonArray>();
        }

        DynamicJsonDocument commandDoc(
            4096
        );

        if (!deserializeJson(
                commandDoc,
                script.commands))
        {
            obj["commands"] =
                commandDoc.as<JsonArray>();
        }
    }

    String output;

    serializeJsonPretty(
        doc,
        output
    );

    return output;
}


// ============================================================================
// Get script by ID
// ============================================================================

String ScriptManager::getScriptById(
    int scriptId
)
{
    int index =
        findScriptIndex(
            scriptId
        );

    if (index < 0)
        return "{}";

    DynamicJsonDocument doc(
        JSON_DOC_SIZE
    );

    Script& script =
        scripts[index];

    JsonObject obj =
        doc.to<JsonObject>();

    obj["id"] =
        script.id;

    obj["name"] =
        script.name;

    DynamicJsonDocument aliasDoc(
        1024
    );

    if (!deserializeJson(
            aliasDoc,
            script.aliases))
    {
        obj["aliases"] =
            aliasDoc.as<JsonArray>();
    }

    DynamicJsonDocument commandDoc(
        4096
    );

    if (!deserializeJson(
            commandDoc,
            script.commands))
    {
        obj["commands"] =
            commandDoc.as<JsonArray>();
    }

    String output;

    serializeJsonPretty(
        doc,
        output
    );

    return output;
}