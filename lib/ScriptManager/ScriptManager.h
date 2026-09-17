#pragma once

#include <Arduino.h>
#include "Debug.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../HardwareManager/Storage/StorageManager.h"

/*
 * ScriptManager
 *
 * A script is simply an ordered list of canonical commands.
 *
 * ScriptManager does NOT:
 *   - know about Kaku
 *   - listen for hardware events
 *   - control IR directly
 *   - control RF directly
 *   - control LivingColors directly
 *   - know how HardwareManager works
 *
 * SceneManager decides WHEN a script should run.
 * Core decides HOW a command reaches hardware.
 */


// ============================================================================
// CommandSink
// ============================================================================

class CommandSink
{
public:

    virtual ~CommandSink() = default;

    /*
     * Core implements this.
     *
     * ScriptManager passes canonical commands here.
     *
     * Example:
     *   "ir TV POWER"
     *   "delay 1000"
     *   "living 1 rgb 255 100 20"
     */
    virtual bool executeCommand(
        const String& command
    ) = 0;
};


// ============================================================================
// ScriptManager
// ============================================================================

class ScriptManager
{
public:

    explicit ScriptManager(
        CommandSink& commandSink
    );

    void begin();

    // ------------------------------------------------------------------------
    // Run
    // ------------------------------------------------------------------------

    bool runScript(
        int scriptId
    );

    bool runScript(
        const String& nameOrAlias
    );

    // ------------------------------------------------------------------------
    // Lookup
    // ------------------------------------------------------------------------

    bool hasScript(
        const String& nameOrAlias
    ) const;

    // ------------------------------------------------------------------------
    // Persistence
    // ------------------------------------------------------------------------

    bool loadScripts();

    bool saveScripts();

    bool addScript(
        const String& json
    );

    bool updateScript(
        int scriptId,
        const String& json
    );

    bool deleteScript(
        int scriptId
    );

    // ------------------------------------------------------------------------
    // JSON output
    // ------------------------------------------------------------------------

    String getScriptsAsJson();

    String getScriptById(
        int scriptId
    );


private:

    // ------------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------------

    static const int MAX_SCRIPTS = 50;

    static const size_t JSON_DOC_SIZE = 16384;

    const char* SCRIPT_FILE =
        "/scripts.json";

   


    // ------------------------------------------------------------------------
    // Script structure
    // ------------------------------------------------------------------------

    struct Script
    {
        int id = 0;

        String name;

        /*
         * JSON array:
         *
         * ["movie", "film"]
         */
        String aliases;

        /*
         * JSON array:
         *
         * [
         *   "ir TV POWER",
         *   
         *   "living 1 rgb 255 100 20"
         * ]
         */
        String commands;
    };


    // ------------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------------

    CommandSink& commandSink;

    Script scripts[MAX_SCRIPTS];

    int scriptCount = 0;


    // ------------------------------------------------------------------------
    // Internal lookup
    // ------------------------------------------------------------------------

    int findScriptIndex(
        int scriptId
    ) const;

    int findScriptIndex(
        const String& nameOrAlias
    ) const;

    int nextScriptId() const;


    // ------------------------------------------------------------------------
    // Internal persistence
    // ------------------------------------------------------------------------

    bool loadScriptsFromFile(
        const char* path
    );

    bool saveScriptsToFile(
        const char* path,
        const String& json
    );


    // ------------------------------------------------------------------------
    // Execution
    // ------------------------------------------------------------------------

    bool executeScriptAtIndex(
        int index
    );


    // ------------------------------------------------------------------------
    // Alias helper
    // ------------------------------------------------------------------------

    static bool aliasesContain(
        const String& aliasesJson,
        const String& value
    );
};