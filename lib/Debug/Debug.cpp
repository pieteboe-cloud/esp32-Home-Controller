#include "Debug.h"



String Debug::webLogBuffer = "";
const int MAX_LOG_SIZE = 8192; // Maximum size of the web log buffer in characters
static unsigned long deviceTimeOffset = 0; // Stores the device time in seconds

void Debug::init(long baud) {

    Serial.begin(baud);
    println("[DEBUG] Debugging initialized at " + String(baud) + " baud");

#ifdef DEBUG_LEVEL
    println("[DEBUG] DEBUG_LEVEL = " + String(DEBUG_LEVEL));
#else
    // DEBUG_LEVEL build flag not defined → only basic logging is active.
    println("[DEBUG] DEBUG_LEVEL not defined (basic logging only)");
#endif
}

String Debug::getTimestamp() {
    if (deviceTimeOffset == 0) {
        return ""; // No time set yet
    }
    
    unsigned long currentTime = deviceTimeOffset + (millis() / 1000);
    unsigned long hours = (currentTime / 3600) % 24;
    unsigned long minutes = (currentTime / 60) % 60;
    unsigned long seconds = currentTime % 60;
    
    String ts = "[";
    if (hours < 10) ts += "0";
    ts += String(hours) + ":";
    if (minutes < 10) ts += "0";
    ts += String(minutes) + ":";
    if (seconds < 10) ts += "0";
    ts += String(seconds) + "] ";
    
    return ts;
}

void Debug::setDeviceTime(unsigned long unixTime) {
    deviceTimeOffset = unixTime - (millis() / 1000);
    println("[INFO][DEBUG] Device time synchronized to: " + String(unixTime));
}

void Debug::print(const char* message) {
    Serial.print(message);
    webLogBuffer += String(message);
}

void Debug::print(int value) {
    Serial.print(value);
    webLogBuffer += String(value);
}

void Debug::print(unsigned long value) {
    Serial.print(value);
    webLogBuffer += String(value);

}

void Debug::print(const String& message) {
    Serial.print(message);
    webLogBuffer += message;
}


void Debug::println(const char* message) {
    Serial.println(message);
    webLogBuffer += String(message) + "\n";
}

void Debug::println(int value) {
    Serial.println(value);
    webLogBuffer += String(value) + "\n";
}

void Debug::println(unsigned long value) {
    Serial.println(value);
    webLogBuffer += String(value) + "\n";
}

void Debug::println(const String& message) {
    Serial.println(message);
    
    // Add timestamp to web logs if time is set
    String timestampedMsg = getTimestamp() + message;
    
    // Check if adding this message would exceed buffer size
    size_t msgLen = timestampedMsg.length() + 1; // +1 for \n
    if (webLogBuffer.length() + msgLen > MAX_LOG_SIZE) {
        // Rotate buffer: keep the most recent 75%
        size_t keepSize = MAX_LOG_SIZE * 3 / 4;
        if (webLogBuffer.length() > keepSize) {
            // Find the position to start keeping from
            int startPos = webLogBuffer.length() - keepSize;
            int newlinePos = webLogBuffer.indexOf('\n', startPos);
            if (newlinePos > 0 && newlinePos < webLogBuffer.length()) {
                webLogBuffer = webLogBuffer.substring(newlinePos + 1);
            } else {
                // If no newline found, clear half the buffer
                webLogBuffer = webLogBuffer.substring(webLogBuffer.length() / 2);
            }
        }
    }
    
    // Add the message if buffer is still not full
if (webLogBuffer.length() + msgLen <= MAX_LOG_SIZE + 64) {  // Allow some overflow for rotation
        webLogBuffer += timestampedMsg + "\n";
    }
}


String Debug::getWebLogs() {
    return webLogBuffer;
}

bool Debug::_verbose = true; // Default to verbose logging enabled

void Debug::setVerbose(bool verbose) {
    _verbose = verbose;
}

bool Debug::isVerbose() {
    return _verbose;
}

void Debug::info(const String& message) {
    println("[INFO] " + message);
}

void Debug::packet(const String& message) {
    println("[PACKET] " + message);
}

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

void Debug::clearLogs() {
    webLogBuffer = "";
}