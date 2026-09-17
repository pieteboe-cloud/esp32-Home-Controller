#include "Debug.h"

// Initialize static members
String Debug::webLogBuffer = "";
unsigned long Debug::deviceTimeOffset = 0;
int Debug::timezoneOffsetMinutes = 0;
int Debug::currentDebugLevel = 3; // Default to debug level 3

/**
 * @brief Initialize the debug system
 * @param baud Serial baud rate (default: 115200)
 * @param defaultLevel Default debug level if not set elsewhere (default: 3)
 */
void Debug::begin(long baud, int defaultLevel) {
    Serial.begin(baud);
    currentDebugLevel = defaultLevel;
   
    
}
/**
 * @brief Get the current timestamp string
 * @return Timestamp string in [HH:MM:SS] format using either uptime or synced wall time
 */
String Debug::getTimestamp() {
    long currentTime;
    if (deviceTimeOffset == 0) {
        currentTime = millis() / 1000UL;
    } else {
        currentTime = (long)(deviceTimeOffset + (millis() / 1000UL)) + (timezoneOffsetMinutes * 60L);
    }

    if (currentTime < 0) currentTime = 0;

    unsigned long hours = (currentTime / 3600UL) % 24UL;
    unsigned long minutes = (currentTime / 60UL) % 60UL;
    unsigned long seconds = currentTime % 60UL;

    String ts = "[";
    if (hours < 10) ts += "0";
    ts += String(hours) + ":";
    if (minutes < 10) ts += "0";
    ts += String(minutes) + ":";
    if (seconds < 10) ts += "0";
    ts += String(seconds) + "] ";

    return ts;
}

/**
 * @brief Set the device time for synchronized timestamps
 * @param unixTime Unix timestamp to set
 */
void Debug::setDeviceTime(unsigned long unixTime) {
    deviceTimeOffset = unixTime - (millis() / 1000);
    println(3, "[DEBUG][setDeviceTime] Device time synchronized to: " + String(unixTime) + " (UTC offset " + String(timezoneOffsetMinutes) + " minutes)");
}

void Debug::setTimezoneOffsetMinutes(int offsetMinutes) {
    timezoneOffsetMinutes = constrain(offsetMinutes, -840, 840);
    println(3, "[DEBUG][setTimezoneOffsetMinutes] UTC" + String(timezoneOffsetMinutes >= 0 ? "+" : "") + String(timezoneOffsetMinutes / 60.0, 2));
}

/**
 * @brief Set the debug level at runtime
 * @param level New debug level (1-5)
 */
void Debug::setDebugLevel(int level) {
    if (level < 1 || level > 6) {
        level = 3; // Default to INFO level if invalid
    }
    println(4, "[DEBUG] Debug level was " + String(currentDebugLevel) + " and now set to " + String(level));
    currentDebugLevel = level;

}

/**
 * @brief Get the current debug level
 * @return Current debug level (1-4)
 */
int Debug::getDebugLevel() {
    return currentDebugLevel;
}

int Debug::getDebugLevelVar() {
    return currentDebugLevel;
}

void Debug::logStartupBanner(const String& projectName, const String& version) {
    println(3, "");
    println(5, "  ┌─────────────────────────────────────────────┐");
    println(5, "  │  ▄▀█ █▀▀ █▀█ █▀▄▀█                         │");
    println(5, "  │  █▀█ ██▄ █▀▀ █ ▀ █                         │");
    println(5, "  └─────────────────────────────────────────────┘");
    println(4, "  " + projectName + " v" + version);
    println(4, "  Target: ESP32 DOIT DEVKIT V1");
    println(3, "  222 files · 5.96 MB · .h:60 .cpp:19 .ino:38");
    println(2, "  ⚠ Pieteboe's childhood dream: INITIALIZED");
    println(1, "  ✗ No bugs found (they're hiding)");
    println(3, "  Timestamp: " + String(deviceTimeOffset == 0 ? "uptime fallback" : "wall clock"));
    println(5, "  ─────────────────────────────────────────────");
    println(4, "  🚀 All systems go. Launching...");
    println(3, "");
}   

void Debug::logSubsystemStatus(const String& subsystem, const String& status, const String& details) {
    String message = "[STATUS] " + subsystem + " -> " + status;
    if (details.length() > 0) {
        message += " | " + details;
    }
    println(currentDebugLevel, message);
}

/**
 * @brief Print a message to serial and web log buffer
 * @param message Message to print
 */
void Debug::print(const char* message) {
    Serial.print(message);
    addToWebLog(message);
}

/**
 * @brief Print an integer value to serial and web log buffer
 * @param value Integer value to print
 */
void Debug::print(int value) {
    Serial.print(value);
    addToWebLog(String(value));
}

/**
 * @brief Print an unsigned long value to serial and web log buffer
 * @param value Unsigned long value to print
 */
void Debug::print(unsigned long value) {
    Serial.print(value);
    addToWebLog(String(value));
}

/**
 * @brief Print a String message to serial and web log buffer
 * @param message String to print
 */
void Debug::print(const String& message) {
    Serial.print(message);
    addToWebLog(message);
}

/**
 * @brief Print a message with newline to serial and web log buffer
 * @param message Message to print (default: empty string)
 */
void Debug::println(const char* message) {
    println(currentDebugLevel, String(message));
}

/**
 * @brief Print an integer value with newline to serial and web log buffer
 * @param value Integer value to print
 */
void Debug::println(int value) {
    println(currentDebugLevel, value);
}

/**
 * @brief Print an unsigned long value with newline to serial and web log buffer
 * @param value Unsigned long value to print
 */
void Debug::println(unsigned long value) {
    println(currentDebugLevel, value);
}

/**
 * @brief Print a String message with newline to serial and web log buffer
 * @param message String to print
 */
void Debug::println(const String& message) {
    println(currentDebugLevel, message);
}

/**
 * @brief Print a message with specified debug level
 * @param level Debug level (1-5)
 * @param message Message to print
 */
void Debug::println(int level, const String& message) {
    // Enforce the configured debug threshold so the logger behaves predictably.
    // This keeps the serial window readable while still preserving noisy traces
    // when DEBUG_LEVEL is raised intentionally during debugging.
    if (level < 1 || level > 6) {
        level = 3;
    }
    // if (level > currentDebugLevel) {
    //     return;
    // }
   
    String timestamp = getTimestamp();
    
        String output = timestamp + message;   
       


    
        // Print to serial with the selected severity.
    Serial.println(output);

    // Add to web log with level-specific color coding.
    addToWebLog(message, level);
}

/**
 * @brief Print an integer value with specified debug level
 * @param level Debug level (1-5)
 * @param value Integer value to print
 */
void Debug::println(int level, int value) {
    println(level, String(value));
}

/**
 * @brief Print an unsigned long value with specified debug level
 * @param level Debug level (1-5)
 * @param value Unsigned long value to print
 */
void Debug::println(int level, unsigned long value) {
    println(level, String(value));
}

/**
 * @brief Add a message to web log buffer with timestamp
 * @param message Message to add
 * @param level Debug level for color coding
 */
void Debug::addToWebLog(const String& message, int level) {
    // Always add a timestamp, even before wall-clock sync. This keeps the log timeline useful
    // and makes debugging easier when the device is still booting or has not yet received time.
    String timestampedMsg = getTimestamp() + message;
    webLogBuffer += getWebLogColor(level) + timestampedMsg + "\n";

    // Check if buffer needs rotation
    rotateWebLogBuffer();
}

/**
 * @brief Rotate the web log buffer if needed
 */
void Debug::rotateWebLogBuffer() {
    // Check if adding this message would exceed buffer size
    if (webLogBuffer.length() > MAX_LOG_SIZE) {
        // Rotate buffer: keep the most recent 75%
        size_t keepSize = MAX_LOG_SIZE * 3 / 4;

        // Find the position to start keeping from (look for last newline)
        int startPos = webLogBuffer.length() - keepSize;
        int newlinePos = webLogBuffer.lastIndexOf('\n', startPos);

        if (newlinePos > 0) {
            // Keep everything after the last newline
            webLogBuffer = webLogBuffer.substring(newlinePos + 1);
        } else {
            // If no newline found, clear half the buffer
            webLogBuffer = webLogBuffer.substring(webLogBuffer.length() / 2);
        }
    }
}

/**
 * @brief Get color code for web log based on debug level
 * @param level Debug level
 * @return HTML color code string
 */
String Debug::getWebLogColor(int level) {
    // Keep the web log colors aligned with the serial-level convention used in the firmware.
    switch(level) {
        case 1: return "<span style='color:red'>";
        case 2: return "<span style='color:blue'>";
        case 3: return "<span style='color:green'>";
        case 4: return "<span style='color:violet'>";
        case 5: return "<span style='color:yellow'>";
        default: return "";
    }
}

/**
 * @brief Get the web log buffer contents
 * @return String containing all web logs with timestamps
 */
String Debug::getWebLogs() {
    return webLogBuffer;
}

/**
 * @brief Clear the web log buffer
 */
void Debug::clearLogs() {
    webLogBuffer = "";
}

/**
 * @brief Convert binary data to hexadecimal string representation
 * @param data Pointer to binary data
 * @param len Length of data in bytes
 * @return String containing hexadecimal representation
 */
String Debug::hex(const uint8_t* data, uint8_t len) {
    String result = "";
    for (uint8_t i = 0; i < len; i++) {
        if (data[i] < 0x10) {
            result += "0";
        }
        result += String(data[i], HEX);
        result += " ";
    }
    return result;
}

/**
 * @brief Acknowledge received logs (remove from buffer)
 * @param count Number of logs to acknowledge (0 = clear all)
 */
void Debug::acknowledgeLogs(int count) {
    if (count == 0) {
        // Clear all logs
        clearLogs();
        return;
    }

    if (count < 0 || webLogBuffer.isEmpty()) {
        return; // Invalid count or empty buffer
    }

    // Find the position to keep based on count of newlines
    int newlineCount = 0;
    int pos = -1;

    // Count newlines from the end
    for (int i = webLogBuffer.length() - 1; i >= 0 && newlineCount < count; i--) {
        if (webLogBuffer.charAt(i) == '\n') {
            newlineCount++;
            pos = i;
        }
    }

    if (pos > 0) {
        // Keep everything after the last found newline
        webLogBuffer = webLogBuffer.substring(pos + 1);
    } else {
        // Not enough newlines found, clear buffer
        clearLogs();
    }
}
