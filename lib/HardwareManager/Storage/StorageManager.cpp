#include "StorageManager.h"

// Storage callback instance
Storage::StorageCallback Storage::_callback = nullptr;

bool Storage::init() {
    // LittleFS is the persistent storage layer for the web UI, config files, and action definitions.
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[STORAGE][init] Initializing storage system...");
    #endif
    #endif

    // Mount LittleFS before any read/write access. This is the filesystem used for the web pages.
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 3
        Debug::println("[STORAGE][DEBUG] Attempting to mount LittleFS...");
    #endif
    #endif

    if (!LittleFS.begin()) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][ERROR] Failed to mount LittleFS");
        #endif
        #endif
        triggerCallback("mount_failed", "LittleFS mount failed");
        return false;
    }

    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[STORAGE][INFO] LittleFS mounted successfully");
    #endif
    #endif

    // Initialize standard files
    if (initializeStandardFiles()) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 2
            Debug::println("[STORAGE][INFO] Standard files initialized successfully");
        #endif
        #endif
    } else {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][WARNING] Some standard files may not have been initialized");
        #endif
        #endif
    }

    // Display file system info and root directory contents to validate the mounted filesystem.
    getFSInfo();
    listDir("/");

    triggerCallback("mounted", "LittleFS mounted successfully");
    Debug::println("[STORAGE][init] Storage initialization complete; root directory has been inspected");
    return true;
}

bool Storage::exists(const String& path) {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][exists] Checking if file exists: " + path);
    #endif
    #endif
    return LittleFS.exists(path);
}

bool Storage::remove(const String& path) {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][remove] Attempting to remove file: " + path);
    #endif
    #endif

    if (!LittleFS.exists(path)) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][ERROR] File not found for removal: " + path);
        #endif
        #endif
        triggerCallback("remove_failed", "File not found: " + path);
        return false;
    }

    bool success = LittleFS.remove(path);
    if (success) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 2
            Debug::println("[STORAGE][remove] File removed successfully: " + path);
        #endif
        #endif
        triggerCallback("removed", path);
    } else {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][ERROR] Failed to remove file: " + path);
        #endif
        #endif
        triggerCallback("remove_failed", "Failed to remove: " + path);
    }
    return success;
}

bool Storage::mkdir(const String& path) {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][mkdir] Attempting to create directory: " + path);
    #endif
    #endif

    bool success = LittleFS.mkdir(path);
    if (success) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 2
            Debug::println("[STORAGE][mkdir] Directory created successfully: " + path);
        #endif
        #endif
        triggerCallback("directory_created", path);
    } else {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][mkdir] Directory may already exist: " + path);
        #endif
        #endif
    }
    return success;
}

String Storage::read(const String& path) {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][read] Attempting to read file: " + path);
    #endif
    #endif

    File file = LittleFS.open(path, "r");
    if (!file) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][read] Failed to open file for reading: " + path);
        #endif
        #endif
        triggerCallback("read_failed", "Failed to open for reading: " + path);
        return "";
    }

    String data = file.readString();
    file.close();

    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 3
        Debug::println("[STORAGE][readString] Read " + String(data.length()) + " bytes from " + path);
    #endif
    #endif

    triggerCallback("read", path + " (" + String(data.length()) + " bytes)");
    return data;
}

bool Storage::write(const String& path, const String& data) {
    // Writes are used for JSON config files and persisted scenes/action data.
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][write] Attempting to write to file: " + path);
        Debug::println("[STORAGE][write] Data size: " + String(data.length()) + " bytes");
    #endif
    #endif

    File file = LittleFS.open(path, "w");
    if (!file) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][open] Failed to open file for writing: " + path);
        #endif
        #endif
        triggerCallback("write_failed", "Failed to open for writing: " + path);
        return false;
    }

    size_t bytesWritten = file.print(data);
    file.close();

    if (bytesWritten > 0) {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 2
            Debug::println("[STORAGE][open] Successfully wrote " + String(bytesWritten) + " bytes to " + path);
        #endif
        #endif
        triggerCallback("written", path + " (" + String(bytesWritten) + " bytes)");
        return true;
    } else {
        #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[STORAGE][open] Failed to write data to file: " + path);
        #endif
        #endif
        triggerCallback("write_failed", "Failed to write data to: " + path);
        return false;
    }
}

void Storage::listDir(const String& path) {
    // This is useful when debugging filesystem structure or checking that the web pages were written correctly.
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[STORAGE][listDir] Listing directory: " + path);
    #endif
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
