#include "Debug.h"

// The logger is intentionally self-contained: it can start before the rest of
// the application and remains useful even when the network or filesystem fails.
String Debug::webLogBuffer = "";
unsigned long Debug::deviceTimeOffset = 0;
unsigned long Debug::bootSeconds = 0;
int Debug::timezoneOffsetMinutes = 0;
int Debug::currentDebugLevel = Debug::INFO;
bool Debug::timestampEnabled = true;

int Debug::normalizeLevel(int level) {
    // Be forgiving. Bad input from a web client must never disable logging.
    if (level < ERROR) return ERROR;
    if (level > VERBOSE) return VERBOSE;
    return level;
}

void Debug::begin(long baud, int defaultLevel) {
    Serial.begin(baud);
    bootSeconds = millis() / 1000UL;
    deviceTimeOffset = 0;
    timezoneOffsetMinutes = 0;
    currentDebugLevel = normalizeLevel(defaultLevel);
    timestampEnabled = true;
}

String Debug::getTimestamp() {
    if (!timestampEnabled) return "";

    if (deviceTimeOffset == 0) {
        // Before time sync, uptime is honest and useful for boot diagnostics.
        return "+" + String((millis() / 1000UL) - bootSeconds) + "s ";
    }

    const long localTime = static_cast<long>(
        deviceTimeOffset + millis() / 1000UL + timezoneOffsetMinutes * 60L
    );
    const unsigned long safeTime = localTime < 0 ? 0UL : static_cast<unsigned long>(localTime);
    const unsigned long hours = (safeTime / 3600UL) % 24UL;
    const unsigned long minutes = (safeTime / 60UL) % 60UL;
    const unsigned long seconds = safeTime % 60UL;

    char timestamp[20];
    snprintf(timestamp, sizeof(timestamp), "[%02lu:%02lu:%02lu] ", hours, minutes, seconds);
    return String(timestamp);
}

void Debug::setDeviceTime(unsigned long unixTime) {
    deviceTimeOffset = unixTime - millis() / 1000UL;
    println(INFO, "[DEBUG][TIME] Wall clock synchronized; timestamps now use wall clock");
}

void Debug::setTimezoneOffsetMinutes(int offsetMinutes) {
    timezoneOffsetMinutes = constrain(offsetMinutes, -840, 840);
    println(INFO, "[DEBUG][TIME] UTC offset set to " + String(timezoneOffsetMinutes) + " minutes");
}

void Debug::setDebugLevel(int level) {
    const int previousLevel = currentDebugLevel;
    currentDebugLevel = normalizeLevel(level);

    // Log after changing the level so the confirmation is visible at INFO.
    println(INFO, "[DEBUG][LEVEL] Runtime level " + String(previousLevel) + " -> " + String(currentDebugLevel));
}

int Debug::getDebugLevel() { return currentDebugLevel; }
int Debug::getDebugLevelVar() { return currentDebugLevel; }
bool Debug::hasSynchronizedTime() { return deviceTimeOffset != 0; }
void Debug::setTimestampEnabled(bool enabled) { timestampEnabled = enabled; }
bool Debug::isTimestampEnabled() { return timestampEnabled; }

void Debug::print(const char* message) {
    const String value = message ? String(message) : String("");
    Serial.print(value);
    addToWebLog(value, INFO);
}

void Debug::print(int value) {
    Serial.print(value);
    addToWebLog(String(value), INFO);
}

void Debug::print(unsigned long value) {
    Serial.print(value);
    addToWebLog(String(value), INFO);
}

void Debug::print(const String& message) {
    Serial.print(message);
    addToWebLog(message, INFO);
}

void Debug::println(const char* message) { println(INFO, message ? String(message) : String("")); }
void Debug::println(int value) { println(INFO, String(value)); }
void Debug::println(unsigned long value) { println(INFO, String(value)); }
void Debug::println(const String& message) { println(INFO, message); }
void Debug::println(int level, int value) { println(level, String(value)); }
void Debug::println(int level, unsigned long value) { println(level, String(value)); }

void Debug::println(int level, const String& message) {
    level = normalizeLevel(level);

    // Levels are thresholds: level 3 includes levels 1, 2, and 3.
    if (level > currentDebugLevel) return;

    Serial.println(getTimestamp() + message);
    addToWebLog(message, level);
}

String Debug::escapeHtml(const String& value) {
    String escaped;
    escaped.reserve(value.length());

    for (size_t i = 0; i < value.length(); ++i) {
        switch (value.charAt(i)) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '\"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += value.charAt(i); break;
        }
    }

    return escaped;
}

void Debug::addToWebLog(const String& message, int level) {
    // The dashboard renders this as HTML. Escape content and close every span.
    webLogBuffer += getWebLogColor(normalizeLevel(level));
    webLogBuffer += getTimestamp();
    webLogBuffer += escapeHtml(message);
    webLogBuffer += "</span>\n";
    rotateWebLogBuffer();
}

void Debug::rotateWebLogBuffer() {
    if (webLogBuffer.length() <= MAX_LOG_SIZE) return;

    // Keep the newest 75%, starting at a complete line where possible.
    const size_t keepSize = MAX_LOG_SIZE * 3 / 4;
    const int start = static_cast<int>(webLogBuffer.length() - keepSize);
    const int newline = webLogBuffer.lastIndexOf('\n', start);

    if (newline >= 0) {
        webLogBuffer = webLogBuffer.substring(newline + 1);
    } else {
        webLogBuffer = webLogBuffer.substring(webLogBuffer.length() / 2);
    }
}

String Debug::getWebLogColor(int level) {
    switch (normalizeLevel(level)) {
        case ERROR: return "<span style='color:#ff8585'>";
        case WARN: return "<span style='color:#ffd166'>";
        case INFO: return "<span style='color:#8ec5ff'>";
        case DEBUG: return "<span style='color:#c4b5fd'>";
        default: return "<span style='color:#94a3b8'>";
    }
}

String Debug::getWebLogs() { return webLogBuffer; }
void Debug::clearLogs() { webLogBuffer = ""; }

String Debug::hex(const uint8_t* data, uint8_t len) {
    if (!data || len == 0) return "";

    String result;
    result.reserve(len * 3);
    for (uint8_t i = 0; i < len; ++i) {
        if (data[i] < 0x10) result += '0';
        result += String(data[i], HEX);
        if (i + 1 < len) result += ' ';
    }
    return result;
}

void Debug::acknowledgeLogs(int count) {
    if (count == 0) {
        clearLogs();
        return;
    }

    if (count < 0 || webLogBuffer.isEmpty()) return;

    int found = 0;
    int position = -1;
    for (int i = webLogBuffer.length() - 1; i >= 0 && found < count; --i) {
        if (webLogBuffer.charAt(i) == '\n') {
            ++found;
            position = i;
        }
    }

    if (position >= 0) {
        webLogBuffer = webLogBuffer.substring(position + 1);
    } else {
        clearLogs();
    }
}
