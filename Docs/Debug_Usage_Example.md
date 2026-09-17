# Debug System Usage Guide

This guide explains how to use the updated debug system in your project.

## Overview

The debug system now supports two main usage patterns:

1. **Preprocessor Directive Pattern** (Recommended for performance):
   ```cpp
   #define DEBUG_LEVEL 3

   #if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 1
   Debug::println("[STORAGE][ERROR] Failed to open configuration file");
   #endif

   #if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
   Debug::println("[STORAGE][WARN] Configuration file is empty");
   #endif

   #if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 3
   Debug::println("[STORAGE][INFO] Loading configuration with default values");
   #endif
   ```

2. **Runtime Pattern** (For dynamic debug levels):
   ```cpp
   // Set debug level at runtime
   Debug::setDebugLevel(Debug::INFO);  // Level 3

   // Direct calls without preprocessor checks
   Debug::println("[STORAGE][ERROR] Failed to open configuration file");
   Debug::println("[STORAGE][INFO] Configuration loaded successfully");
   ```

## Debug Levels

The system supports four debug levels:

- `Debug::ERROR` (1) - Critical errors (red)
- `Debug::WARN` (2) - Warnings and important info (yellow)
- `Debug::INFO` (3) - Normal runtime trace (blue)
- `Debug::VERBOSE` (4) - Verbose details (grey)

## Initialization

Initialize the debug system in your setup:

```cpp
void setup() {
    // Initialize with default debug level (INFO)
    Debug::begin(115200, Debug::INFO);

    // Or specify a different level
    // Debug::begin(115200, Debug::WARN);
}
```

## Changing Debug Level at Runtime

You can change the debug level during execution:

```cpp
// Change debug level to ERROR only
Debug::setDebugLevel(Debug::ERROR);

// Get current debug level
int currentLevel = Debug::getDebugLevel();
```

## Best Practices

1. Use preprocessor directives for performance-critical code paths
2. Use meaningful prefixes in your log messages (e.g., `[STORAGE]`, `[NETWORK]`)
3. Use appropriate debug levels for your messages
4. Avoid logging sensitive information
