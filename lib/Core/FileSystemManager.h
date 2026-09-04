#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "../Debug/Debug.h"

class FileSystemManager {
public:
    static bool begin() {
        if (!isInitialized) {
            if (LittleFS.begin()) {
                isInitialized = true;
                #if DEBUG_LEVEL >= 1
                Debug::println("[FS] Filesystem mounted successfully");
                #endif
                return true;
            } else {
                #if DEBUG_LEVEL >= 1
                Debug::println("[FS][ERROR] Filesystem mount failed");
                #endif
                return false;
            }
        }
        return true; // Already initialized
    }

    static bool isMounted() {
        return isInitialized;
    }

private:
    static bool isInitialized;
};
