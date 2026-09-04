#include "Translator.h"

namespace {
String normalizeHex32(const String& value) {
    String s = value;
    s.trim();
    s.toLowerCase();
    if (s.startsWith("0x") || s.startsWith("0X")) {
        s = s.substring(2);
    }
    if (s.length() == 0) {
        return "0x00000000";
    }
    while (s.length() < 8) {
        s = "0" + s;
    }
    if (s.length() > 8) {
        s = s.substring(s.length() - 8);
    }
    return "0x" + s;
}

String normalizeHex32(uint32_t value) {
    String s = String(value, HEX);
    s.toLowerCase();
    while (s.length() < 8) {
        s = "0" + s;
    }
    return "0x" + s;
}
}

std::vector<IrButtonMap> Translator::_irMap;

void Translator::init() {
    // Initialize the IR code translator
    // This loads IR databases from filesystem and subscribes to EventBus
    // to receive IR events for translation
    Debug::println(2, "[TRANSLATOR] Initializing...");
    loadIrDatabase();

    // Subscribe to EventBus to listen for raw IR/RF codes
    EventBus::getInstance().subscribe(handleEvent);
}

void Translator::loadIrDatabase() {
    // Load IR databases from filesystem
    // This method loads all JSON files from the /ir_db/ directory and populates
    // the internal mapping database with IR code to virtual color mappings
    Debug::println(2, "[TRANSLATOR] Loading IR databases from /ir_db/...");

    // Open the directory
    File root = LittleFS.open("/ir_db");
    if (!root || !root.isDirectory()) {
        Debug::println(1, "[TRANSLATOR][ERROR] /ir_db/ directory not found!");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".json")) {
            Debug::println(3, "[TRANSLATOR] Loading: " + String(file.name()));

            // Parse JSON
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, file);

            if (error) {
                Debug::println(1, "[TRANSLATOR][ERROR] Failed to parse " + String(file.name()));
            } else {
                String remoteId = doc["id"] | "UNKNOWN";
                JsonArray buttons = doc["buttons"];

                for (JsonObject btn : buttons) {
                    IrButtonMap map;
                    map.remoteId = remoteId;
                    map.buttonName = btn["name"] | "";

                    // Normalize codes to a canonical 32-bit hex representation to keep
                    // all leading zero bytes intact and avoid mismatches like 0x00ff1ae5 vs 0xff1ae5.
                    String rawCode = btn["code"] | "0x00000000";
                    map.code = normalizeHex32(rawCode);

                    map.virtualColor = btn["virtual_color"] | "";
                    map.virtualColor.trim();

                    if (map.virtualColor != "") {
                        _irMap.push_back(map);
                        #if DEBUG_LEVEL >= 3
                        Debug::println(2, "  Mapped: " + map.code + " -> " + map.virtualColor + " (" + map.remoteId + ")");
                        #endif
                    }
                }
            }
        }
        file = root.openNextFile();
    }

    Debug::println(2, "[TRANSLATOR] Loaded " + String(_irMap.size()) + " IR button mappings.");
    root.close();
}

String Translator::resolveIrCode(const String& rawCode, const String& sourceRemote) {
    // Resolve a raw IR code to a virtual color
    // This method searches the loaded IR database for a matching code
    // and returns the associated virtual color name
    String searchCode = normalizeHex32(rawCode);
    Debug::println(4, "[TRANSLATOR][RESOLVE] Searching for code: " + searchCode);

    for (const auto& map : _irMap) {
        if (map.code == searchCode) {
            Debug::println(3, "[TRANSLATOR][RESOLVE] Found match: " + map.code + " -> " + map.virtualColor);
            if (sourceRemote == "" || map.remoteId == sourceRemote) {
                Debug::println(3, "[TRANSLATOR][RESOLVE] Returning virtual color: " + map.virtualColor);
                return map.virtualColor;
            } else {
                Debug::println(3, "[TRANSLATOR][RESOLVE] Remote ID mismatch, skipping match");
            }
        }
    }

    Debug::println(3, "[TRANSLATOR][RESOLVE] No match found for code: " + searchCode);
    return ""; // No match found
}

void Translator::handleEvent(const SystemEvent& evt) {
    // Handle incoming system events
    // Currently only processes IR events, translating them to virtual color events
    Debug::println(3, "[TRANSLATOR][EVENT] Processing event: " + evt.source + " -> " + evt.identifier);

    // Only process IR events for now
    if (evt.source == "IR") {
        Debug::println(4, "[TRANSLATOR][IR] Processing IR event: " + evt.identifier);

        // evt.identifier is like "IR_RAW_fb04ef00"
        String rawCode = evt.identifier.substring(7); // Remove "IR_RAW_"
        Debug::println(4, "[TRANSLATOR][IR] Extracted raw code: " + rawCode);

        String virtualColor = resolveIrCode(rawCode);

        if (virtualColor != "") {
            Debug::println(2, "[TRANSLATOR] Translated " + evt.identifier + " -> " + virtualColor);

            // Publish the new Virtual Color event
            SystemEvent newEvt;
            newEvt.source = "VIRTUAL_COLOR";
            newEvt.identifier = virtualColor;
            newEvt.rawData = evt.rawData;
            Debug::println(3, "[TRANSLATOR][EVENT] Publishing new event: " + newEvt.source + " -> " + newEvt.identifier);
            EventBus::getInstance().publish(newEvt);
        } else {
            Debug::println(3, "[TRANSLATOR] Unknown IR Code: " + rawCode);
        }
    }
}

void Translator::update() {
    // Update method called in the main loop
    // Currently not used as the translator is event-driven
    // Retained for potential future functionality
}