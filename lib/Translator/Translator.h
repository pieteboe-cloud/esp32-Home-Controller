#pragma once
#include <Arduino.h>
#include "Debug.h"
#include <FS.h>
#include <ArduinoJson.h>
#include "EventBus.h"
#include "../HardwareManager/Storage/StorageManager.h"
#include <LittleFS.h>


/**
 * @brief Structure representing an IR button mapping
 * 
 * This structure defines a mapping between an IR remote button and a virtual color.
 * Used by the Translator to translate raw IR codes to virtual color events.
 */
struct IrButtonMap {
    String remoteId;     ///< ID of the remote control (e.g., "Samsung-TV")
    String buttonName;  ///< Name of the button (e.g., "Power", "VolumeUp")
    String code;        ///< Normalized 32-bit hex code of the IR signal
    String virtualColor;///< Virtual color name that this IR button triggers
};

/**
 * @brief IR code to virtual color translator
 * 
 * This class translates raw IR codes to virtual color events, enabling
 * physical remote controls to trigger virtual color actions.
 * It loads IR databases from JSON files and maintains a mapping of
 * IR codes to virtual colors.
 */
class Translator {
public:
    /**
     * @brief Initialize the translator
     * 
     * Loads IR databases from filesystem and subscribes to EventBus
     * to receive IR events for translation.
     */
    static void init();
    /**
     * @brief Update method called in the main loop
     * 
     * Currently not used as the translator is event-driven.
     * Retained for potential future functionality.
     */
    static void update(); // Called in loop to process events
    
    /**
     * @brief Resolve a raw IR code to a virtual color
     * @param rawCode The raw IR code to resolve
     * @param sourceRemote Optional remote ID to filter results
     * @return Virtual color name if found, empty string otherwise
     * 
     * This method searches the loaded IR database for a matching code
     * and returns the associated virtual color name.
     */
    static String resolveIrCode(const String& rawCode, const String& sourceRemote = "");

private:
    static std::vector<IrButtonMap> _irMap; ///< Vector of IR button mappings
    
    /**
     * @brief Load IR databases from filesystem
     * 
     * Loads all JSON files from the /ir_db/ directory and populates
     * the internal mapping database.
     */
    static void loadIrDatabase();
    
    /**
     * @brief Handle incoming system events
     * @param evt The system event to process
     * 
     * Currently only processes IR events, translating them to virtual color events.
     */
    static void handleEvent(const SystemEvent& evt);
};   