# Debug System Guide

## Current Debug System Overview

### Debug Levels
Your system uses a 4-level debug hierarchy:

1. **Level 1** (Red) - Errors only
   - Critical failures that need immediate attention
   - System-level problems that affect functionality
   - Example: `Debug::println(1, "[MODULE][ERROR] Failed to initialize hardware");`

2. **Level 2** (Yellow) - Warnings and important startup info
   - Configuration issues and warnings
   - Important system state changes
   - Startup/shutdown sequences
   - Example: `Debug::println(2, "[MODULE][INIT] Initializing with config X");`

3. **Level 3** (Blue) - Debug / normal runtime trace
   - Normal operation tracking
   - Key state transitions
   - User-triggered actions
   - Example: `Debug::println(3, "[MODULE][EVENT] User pressed button");`

4. **Level 4** (Grey) - Verbose / very noisy internals
   - Detailed internal state
   - Function entry/exit points
   - Low-level data dumps
   - Example: `Debug::println(4, "[MODULE][INTERNAL] Processing data packet with size: 42");`

### Current Implementation Issues

1. **Inconsistent Usage**: Some code uses `Debug::println(level, message)` while others use just `Debug::println(message)`
2. **Missing Conditional Compilation**: Not all debug statements are properly wrapped with `#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= X`
3. **Inconsistent Module Prefixes**: Format varies between `[MODULE][FUNCTION]` and other patterns
4. **WebUI Color Mapping**: The level-to-color mapping isn't consistently applied

### Recommended Implementation

#### 1. Consistent Debug Statement Format

```cpp
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
Debug::println(2, "[STORAGE][listDir] Directory contents:");
#endif
```

**Why this format:**
- Conditional compilation ensures no performance impact when debug is disabled
- Explicit level parameter enables proper color coding in WebUI
- Module and function prefixes make logs easy to filter and understand

#### 2. Standardized Prefix Convention

Use this format: `[MODULE][FUNCTION] Message`

Examples:
```cpp
// Error
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 1
Debug::println(1, "[STORAGE][load] Failed to open configuration file");
#endif

// Warning/Important Info
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
Debug::println(2, [STORAGE][init] Using fallback configuration");
#endif

// Debug/Trace
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 3
Debug::println(3, "[STORAGE][listDir] Found 7 files in directory");
#endif

// Verbose
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 4
Debug::println(4, "[STORAGE][listDir] Processing file: config.json with size: 1024 bytes");
#endif
```

#### 3. Debug Helper Macros (Optional)

For even more consistency, consider adding helper macros:

```cpp
// In a common header file
#define DEBUG_ERROR(msg)     do {         if (defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 1)             Debug::println(1, String("[") + __MODULE__ + "][ERROR] " + msg);     } while(0)

#define DEBUG_WARN(msg)     do {         if (defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2)             Debug::println(2, String("[") + __MODULE__ + "][WARN] " + msg);     } while(0)

#define DEBUG_INFO(msg)     do {         if (defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 3)             Debug::println(3, String("[") + __MODULE__ + "][INFO] " + msg);     } while(0)

#define DEBUG_DEBUG(msg)     do {         if (defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 4)             Debug::println(4, String("[") + __MODULE__ + "][DEBUG] " + msg);     } while(0)
```

Usage:
```cpp
#define __MODULE__ "STORAGE"
DEBUG_ERROR("Failed to open file");
DEBUG_WARN("Using fallback configuration");
DEBUG_INFO("File loaded successfully");
DEBUG_DEBUG("Processing file with size: 1024 bytes");
```

#### 4. WebUI Integration

Ensure your WebUI properly maps debug levels to colors:
- Level 1 → Red (errors)
- Level 2 → Yellow (warnings)
- Level 3 → Blue (debug/info)
- Level 4 → Grey (verbose)

### Migration Plan

1. **Phase 1: Standardize Existing Debug Statements**
   - Update all existing `Debug::println` calls to use the consistent format
   - Add proper conditional compilation
   - Ensure module prefixes are consistent

2. **Phase 2: Add Missing Debug Statements**
   - Identify key operations that need better logging
   - Add appropriate debug statements at the right level
   - Focus on error handling and state transitions first

3. **Phase 3: Implement Helper Macros (Optional)**
   - Add the debug helper macros to a common header
   - Update modules to use the macros
   - This provides compile-time safety and reduces boilerplate

4. **Phase 4: WebUI Enhancements**
   - Ensure WebUI properly filters and colors debug messages
   - Consider adding a debug level selector in the WebUI
   - Group messages by module for better organization

### Benefits of This Approach

1. **Consistency**: All debug messages follow the same format
2. **Performance**: Conditional compilation ensures zero runtime overhead when debug is disabled
3. **Readability**: Clear prefixes make it easy to identify the source of messages
4. **Filtering**: Easy to filter by module or log level in both serial and WebUI
5. **Maintainability**: Clear structure makes it easier to add and modify debug statements
