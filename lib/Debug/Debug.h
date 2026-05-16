#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

class Debug {
public:
    static void init(long baud = 115200);
    static void print(const char* message);
    static void print(int value);
    static void print(unsigned long value);
    static void print(const String& message); // Added for String support
    static void println(const char* message = "");
    static void println(int value);
    static void println(unsigned long value);
    static void println(const String& message); // Added for String support
    static String getWebLogs();
    static void setVerbose(bool verbose);
    static bool isVerbose();
    static void info(const String& message);
    static void packet(const String& message);
    static String hex(const uint8_t* data, uint8_t len);
    static void clearLogs();
    static void setDeviceTime(unsigned long unixTime);
    static String getTimestamp();
private:
    static String webLogBuffer;
    static bool _verbose;
};

#endif