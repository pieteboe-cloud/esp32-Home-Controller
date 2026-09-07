#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "../Debug/Debug.h"

class Storage {
public:
    // Initialize the file system
    static bool begin();
    
    // Check if filesystem is mounted
    static bool isMounted();
    
    // Check if file exists
    static bool exists(const String& path);
    
    // Remove a file
    static bool remove(const String& path);
    
    // Create a directory
    static bool mkdir(const String& path);
    
    // Read file contents
    static String read(const String& path);
    
    // Write data to a file
    static bool write(const String& path, const String& data);
    
    // List directory contents
    static void listDir(const String& path);
    
    // Initialize standard configuration files
    static bool initializeStandardFiles();
    
    // Get total and used space
    static void getFSInfo();
    
    // Format the file system
    static bool format();
    
    // Callback for storage events
    using StorageCallback = std::function<void(const String& event, const String& details)>;
    static void setCallback(StorageCallback callback);
    
private:
    static StorageCallback _callback;
    static bool isInitialized;
    
    // Helper function to trigger callbacks
    static void triggerCallback(const String& event, const String& details = "");
};
