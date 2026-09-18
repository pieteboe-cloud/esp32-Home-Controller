#pragma once

#include <Arduino.h>

/**
 * Central logger for Serial and the dashboard log.
 *
 * Runtime level is a visibility threshold: level 1 shows errors only;
 * level 5 shows every compiled diagnostic message.
 */
class Debug {
public:
    static constexpr int ERROR = 1;
    static constexpr int WARN = 2;
    static constexpr int INFO = 3;
    static constexpr int DEBUG = 4;
    static constexpr int VERBOSE = 5;

    static void begin(long baud = 115200, int defaultLevel = INFO);
    static void setDebugLevel(int level);
    static int getDebugLevel();
    static int getDebugLevelVar();

    static void print(const char* message);
    static void print(int value);
    static void print(unsigned long value);
    static void print(const String& message);
    static void println(const char* message = "");
    static void println(int value);
    static void println(unsigned long value);
    static void println(const String& message);
    static void println(int level, const String& message);
    static void println(int level, int value);
    static void println(int level, unsigned long value);

    static String getWebLogs();
    static void clearLogs();
    static void setDeviceTime(unsigned long unixTime);
    static void setTimezoneOffsetMinutes(int offsetMinutes);
    static String getTimestamp();
    static bool hasSynchronizedTime();
    static void setTimestampEnabled(bool enabled);
    static bool isTimestampEnabled();
    static String hex(const uint8_t* data, uint8_t len);
    static void acknowledgeLogs(int count = 0);

private:
    static String webLogBuffer;
    static constexpr size_t MAX_LOG_SIZE = 18192;
    static unsigned long deviceTimeOffset;
    static unsigned long bootSeconds;
    static int timezoneOffsetMinutes;
    static int currentDebugLevel;
    static bool timestampEnabled;

    static int normalizeLevel(int level);
    static String escapeHtml(const String& value);
    static void addToWebLog(const String& message, int level);
    static void rotateWebLogBuffer();
    static String getWebLogColor(int level);
};
