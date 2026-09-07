#include "StorageManager.h"

// Storage callback instance
Storage::StorageCallback Storage::_callback = nullptr;
bool Storage::isInitialized = false;

bool Storage::begin() {
    if (isInitialized) {
        return true;
    }
    // LittleFS is the persistent storage layer for the web UI, config files, and action definitions.
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][INIT] Initializing storage system");
#endif

    // Mount LittleFS before any read/write access. This is the filesystem used for the web pages.
#if DEBUG_LEVEL >= 3
    Debug::println(3, "[STORAGE][INIT] Attempting to mount LittleFS");
#endif

    if (!LittleFS.begin()) {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][ERROR] Failed to mount LittleFS");
#endif
        triggerCallback("mount_failed", "LittleFS mount failed");
        return false;
    }

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][INFO] LittleFS mounted successfully");
#endif

    // Initialize standard files
    if (initializeStandardFiles()) {
#if DEBUG_LEVEL >= 2
        Debug::println(2, "[STORAGE][INFO] Standard files initialized successfully");
#endif
    } else {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][WARN] Some standard files may not have been initialized");
#endif
    }

    // Display file system info and root directory contents to validate the mounted filesystem.
    getFSInfo();
    listDir("/");

    triggerCallback("mounted", "LittleFS mounted successfully");
    Debug::println(2, "[STORAGE][INIT] Storage initialization complete; root directory has been inspected");
    return true;
} 

bool Storage::exists(const String& path) {
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][EXISTS] Checking if file exists: " + path);
#endif
    return LittleFS.exists(path);
}

bool Storage::remove(const String& path) {
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][REMOVE] Attempting to remove file: " + path);
#endif

    if (!LittleFS.exists(path)) {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][ERROR] File not found for removal: " + path);
#endif
        triggerCallback("remove_failed", "File not found: " + path);
        return false;
    }

    bool success = LittleFS.remove(path);
    if (success) {
#if DEBUG_LEVEL >= 2
        Debug::println(2, "[STORAGE][REMOVE] File removed successfully: " + path);
#endif
        triggerCallback("removed", path);
    } else {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][ERROR] Failed to remove file: " + path);
#endif
        triggerCallback("remove_failed", "Failed to remove: " + path);
    }
    return success;
}

bool Storage::mkdir(const String& path) {
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][MKDIR] Attempting to create directory: " + path);
#endif

    bool success = LittleFS.mkdir(path);
    if (success) {
#if DEBUG_LEVEL >= 2
        Debug::println(2, "[STORAGE][MKDIR] Directory created successfully: " + path);
#endif
        triggerCallback("directory_created", path);
    } else {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][WARN] Directory may already exist: " + path);
#endif
    }
    return success;
}

String Storage::read(const String& path) {
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][READ] Attempting to read file: " + path);
#endif

    File file = LittleFS.open(path, "r");
    if (!file) {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][ERROR] Failed to open file for reading: " + path);
#endif
        triggerCallback("read_failed", "Failed to open for reading: " + path);
        return "";
    }

    String data = file.readString();
    file.close();

#if DEBUG_LEVEL >= 3
    Debug::println(3, "[STORAGE][READ] Read " + String(data.length()) + " bytes from " + path);
#endif

    triggerCallback("read", path + " (" + String(data.length()) + " bytes)");
    return data;
}

bool Storage::write(const String& path, const String& data) {
    // Writes are used for JSON config files and persisted scenes/action data.
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][WRITE] Attempting to write to file: " + path);
    Debug::println(2, "[STORAGE][WRITE] Data size: " + String(data.length()) + " bytes");
#endif

    File file = LittleFS.open(path, "w");
    if (!file) {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][ERROR] Failed to open file for writing: " + path);
#endif
        triggerCallback("write_failed", "Failed to open for writing: " + path);
        return false;
    }

    size_t bytesWritten = file.print(data);
    file.close();

    if (bytesWritten > 0) {
#if DEBUG_LEVEL >= 2
        Debug::println(2, "[STORAGE][WRITE] Successfully wrote " + String(bytesWritten) + " bytes to " + path);
#endif
        triggerCallback("written", path + " (" + String(bytesWritten) + " bytes)");
        return true;
    } else {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[STORAGE][ERROR] Failed to write data to file: " + path);
#endif
        triggerCallback("write_failed", "Failed to write data to: " + path);
        return false;
    }
}

void Storage::listDir(const String& path) {
    // This is useful when debugging filesystem structure or checking that the web pages were written correctly.
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][LIST] Listing directory: " + path);
#endif

    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) {
        #if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][listDir] Not a directory or missing: " + path);
        #endif
        triggerCallback("list_failed", "Not a directory: " + path);
        return;
    }

    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][listDir] Directory contents:");
    #endif
    #endif

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String fileInfo = String(file.name()) + " (" + String(file.size()) + " bytes)";
            #ifdef DEBUG_LEVEL
            #if DEBUG_LEVEL >= 2
                Debug::println("  " + fileInfo);
            #endif
            #endif
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    Debug::println("[STORAGE][listDir] Directory scan complete: " + path);
    triggerCallback("listed", path);
}

bool Storage::initializeStandardFiles() {
    // These files are the default config scaffolding for settings, devices, mappings, and schedules.
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][initializeStandardFiles] Initializing standard configuration files...");
    #endif
    #endif

    bool success = true;

    // Create config directory if it doesn't exist
    if (!exists("/config")) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 2
            Debug::println("[STORAGE][initializeStandardFiles] Creating config directory...");
        #endif
        #endif
        if (!mkdir("/config")) {
            #ifdef DEBUG_LEVEL
            #if DEBUG_LEVEL >= 1
                Debug::println("[STORAGE][initializeStandardFiles] Failed to create config directory");
            #endif
            #endif
            success = false;
        }
    }

    // Initialize standard files
    String files[] = {"/config/settings.json", "/config/devices.json", "/config/mappings.json", "/config/schedules.json"};
    String defaultContent[] = {
        "{}",  // Empty JSON for settings
        "[]",  // Empty array for devices
        "{}",  // Empty JSON for mappings
        "[]"   // Empty array for schedules
    };

    for (int i = 0; i < 4; i++) {
        if (!exists(files[i])) {
            #ifdef DEBUG_LEVEL
            #if DEBUG_LEVEL >= 2
                Debug::println("[STORAGE][initializeStandardFiles] Creating standard file: " + files[i]);
            #endif
            #endif
            if (!write(files[i], defaultContent[i])) {
                #ifdef DEBUG_LEVEL
                #if DEBUG_LEVEL >= 1
                    Debug::println("[STORAGE][initializeStandardFiles] Failed to create: " + files[i]);
                #endif
                #endif
                success = false;
            }
        }
    }

    return success;
}

void Storage::getFSInfo() {
    // No struct needed
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    size_t free = LittleFS.totalBytes() - LittleFS.usedBytes();

    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[STORAGE][getFSInfo] Total: " + String(total));
        Debug::println("[STORAGE][getFSInfo] Used:  " + String(used));
        Debug::println("[STORAGE][getFSInfo] Free:  " + String(free));
    #endif
    #endif

    // Format callback string
    String info = "Total: " + String(total) + 
                 ", Used: " + String(used) + 
                 ", Free: " + String(free);

    triggerCallback("fs_info", info);
}

bool Storage::format() {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[STORAGE][format] Formatting file system - all data will be lost!");
    #endif
    #endif

    if (LittleFS.format()) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][format] File system formatted successfully");
        #endif
        #endif
        triggerCallback("formatted", "File system formatted");
        return initializeStandardFiles();
    } else {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][format] Failed to format file system");
        #endif
        #endif
        triggerCallback("format_failed", "Failed to format file system");
        return false;
    }
}

void Storage::setCallback(StorageCallback callback) {
    _callback = callback;
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 3
        Debug::println("[STORAGE][StorageCallback] Storage callback set");
    #endif
    #endif
}

void Storage::triggerCallback(const String& event, const String& details) {
    if (_callback) {
        _callback(event, details);
    }

    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 3
        Debug::println("[STORAGE][triggerCallback] Event triggered: " + event + (details.length() > 0 ? " - " + details : ""));
    #endif
    #endif
}

bool Storage::isMounted() {
    return isInitialized;
}
