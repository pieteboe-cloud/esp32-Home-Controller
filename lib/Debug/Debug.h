#pragma once

#include <Arduino.h>

/**
 * @brief Debug logging library with levels, timestamps, and web log support
 *
 * Logging convention used across the firmware:
 * - 1 = errors (red)
 * - 2 = warnings / important startup info (yellow)
 * - 3 = debug / normal runtime trace (blue)
 * - 4 = verbose / very noisy internals (grey)
 *
 * Preferred usage:
 * @code
 * #define DEBUG_LEVEL 3
 *
 * if (DEBUG_LEVEL >= 1) {
 *     Debug::println(1, "[CORE][ERROR] Something failed");
 * }
 *
 * if (DEBUG_LEVEL >= 2) {
 *     Debug::println(2, "[CORE][WARN] Something may need attention");
 * }
 *
 * if (DEBUG_LEVEL >= 3) {
 *     Debug::println(3, "[CORE][INFO] Normal startup details");
 * }
 *
 * if (DEBUG_LEVEL >= 4) {
 *     Debug::println(4, "[CORE][TRACE] Verbose low-level trace");
 * }
 * @endcode
 *
 * Notes:
 * - The logger still enforces the level gate internally so a direct call like
 *   Debug::println(3, "...") does not leak past the configured threshold.
 * - This keeps the serial output readable while preserving high-detail logs when
 *   you need them during troubleshooting.
 */
class Debug {
public:
    /**
     * @brief Initialize the debug system
     * @param baud Serial baud rate (default: 115200)
     */
    static void begin(long baud = 115200);
    
    /**
     * @brief Print a message to serial and web log buffer
     * @param message Message to print
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void print(const char* message);
    
    /**
     * @brief Print an integer value to serial and web log buffer
     * @param value Integer value to print
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void print(int value);
    
    /**
     * @brief Print an unsigned long value to serial and web log buffer
     * @param value Unsigned long value to print
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void print(unsigned long value);
    
    /**
     * @brief Print a String message to serial and web log buffer
     * @param message String to print
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void print(const String& message);
    
    /**
     * @brief Print a message with newline to serial and web log buffer
     * @param message Message to print (default: empty string)
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void println(const char* message = "");
    
    /**
     * @brief Print an integer value with newline to serial and web log buffer
     * @param value Integer value to print
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void println(int value);
    
    /**
     * @brief Print an unsigned long value with newline to serial and web log buffer
     * @param value Unsigned long value to print
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void println(unsigned long value);
    
    /**
     * @brief Print a String message with newline to serial and web log buffer
     * @param message String to print
     * @note Uses current DEBUG_LEVEL if defined, otherwise defaults to 3
     */
    static void println(const String& message);
    
    /**
     * @brief Print a message with specified debug level.
     * @param level Debug level (1-4)
     * @param message Message to print
     * @note Preferred convention: 1 = red/error, 2 = yellow/warn,
     *       3 = blue/debug, 4 = grey/verbose.
     */
    static void println(int level, const String& message);
    
    /**
     * @brief Print an integer value with specified debug level
     * @param level Debug level (1-4)
     * @param value Integer value to print
     * @note Level 1: Errors, Level 2: Warnings, Level 3: Debug, Level 4: Verbose
     */
    static void println(int level, int value);
    
    /**
     * @brief Print an unsigned long value with specified debug level
     * @param level Debug level (1-4)
     * @param value Unsigned long value to print
     * @note Level 1: Errors, Level 2: Warnings, Level 3: Debug, Level 4: Verbose
     */
    static void println(int level, unsigned long value);
    
    /**
     * @brief Get the web log buffer contents
     * @return String containing all web logs with timestamps
     */
    static String getWebLogs();
    
    /**
     * @brief Clear the web log buffer
     */
    static void clearLogs();
    
    /**
     * @brief Set the device time for synchronized timestamps
     * @param unixTime Unix timestamp to set
     */
    static void setDeviceTime(unsigned long unixTime);

    /**
     * @brief Set the local timezone offset used by debug timestamps
     * @param offsetMinutes Offset from UTC in minutes, for example 120 for UTC+2
     */
    static void setTimezoneOffsetMinutes(int offsetMinutes);
    
    /**
     * @brief Get the current timestamp string
     * @return Timestamp string in [HH:MM:SS] format. Falls back to uptime before wall-clock sync.
     */
    static String getTimestamp();

    /**
     * @brief Emit a friendly startup banner for the whole firmware.
     * @param projectName Human-readable project name
     * @param version Optional version or build tag
     */
    static void logStartupBanner(const String& projectName, const String& version = "dev");

    /**
     * @brief Log a compact subsystem summary line for the serial monitor and web log.
     * @param subsystem Name of the subsystem
     * @param status Human-readable status text
     * @param details Optional details appended to the status line
     */
    static void logSubsystemStatus(const String& subsystem, const String& status, const String& details = "");
    
    /**
     * @brief Convert binary data to hexadecimal string representation
     * @param data Pointer to binary data
     * @param len Length of data in bytes
     * @return String containing hexadecimal representation
     */
    static String hex(const uint8_t* data, uint8_t len);
    
    /**
     * @brief Acknowledge received logs (remove from buffer)
     * @param count Number of logs to acknowledge (0 = clear all)
     */
    static void acknowledgeLogs(int count = 0);
    
private:
    static String webLogBuffer;           // Buffer for web logs
    static const int MAX_LOG_SIZE = 18192;  // Maximum size of web log buffer
    static unsigned long deviceTimeOffset; // Offset for device time synchronization
    static int timezoneOffsetMinutes;      // Local offset from UTC supplied by the browser
    static int currentDebugLevel;         // Current debug level (0-4)
    
    /**
     * @brief Internal method to add a message to web log buffer with timestamp
     * @param message Message to add
     * @param level Debug level for color coding
     */
    static void addToWebLog(const String& message, int level = 3);
    
    /**
     * @brief Internal method to rotate the web log buffer if needed
     */
    static void rotateWebLogBuffer();
    
    /**
     * @brief Get color code for web log based on debug level
     * @param level Debug level
     * @return HTML color code string
     */
    static String getWebLogColor(int level);
};