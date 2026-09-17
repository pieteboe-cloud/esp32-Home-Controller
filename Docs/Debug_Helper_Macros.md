# Debug Helper Macros Implementation

## Combining Current Style with Helper Macros

We can enhance your current debug system with helper macros while maintaining your existing style. Here's how to implement it:

### 1. Create a Debug Helper Header

First, create a dedicated header file for debug helpers:

```cpp
// lib/Core/DebugHelper.h
#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <Arduino.h>
#include "Debug.h" // Your existing Debug class

// Define module name for each file
// Should be defined in each source file before including DebugHelper.h
#ifndef __MODULE__
#define __MODULE__ "UNKNOWN"
#endif

// Debug level definitions (matching your existing system)
#define DEBUG_LEVEL_ERROR     1
#define DEBUG_LEVEL_WARN      2
#define DEBUG_LEVEL_INFO      3
#define DEBUG_LEVEL_DEBUG     4

// Debug helper macros that match your current style
// Each macro automatically includes the module name and proper conditional compilation

// Error messages (Level 1)
#if defined(DEBUG_LEVEL) && (DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)
#define DEBUG_ERROR(msg)     do {         Debug::println(1, String("[") + __MODULE__ + "][ERROR] " + msg);     } while(0)
#else
#define DEBUG_ERROR(msg) do { } while(0)
#endif

// Warning/Important messages (Level 2)
#if defined(DEBUG_LEVEL) && (DEBUG_LEVEL >= DEBUG_LEVEL_WARN)
#define DEBUG_WARN(msg)     do {         Debug::println(2, String("[") + __MODULE__ + "][WARN] " + msg);     } while(0)
#else
#define DEBUG_WARN(msg) do { } while(0)
#endif

// Info/Runtime messages (Level 3)
#if defined(DEBUG_LEVEL) && (DEBUG_LEVEL >= DEBUG_LEVEL_INFO)
#define DEBUG_INFO(msg)     do {         Debug::println(3, String("[") + __MODULE__ + "][INFO] " + msg);     } while(0)
#else
#define DEBUG_INFO(msg) do { } while(0)
#endif

// Debug/Verbose messages (Level 4)
#if defined(DEBUG_LEVEL) && (DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG)
#define DEBUG_DEBUG(msg)     do {         Debug::println(4, String("[") + __MODULE__ + "][DEBUG] " + msg);     } while(0)
#else
#define DEBUG_DEBUG(msg) do { } while(0)
#endif

// Special macros for function entry/exit (useful for Level 4 tracing)
#if defined(DEBUG_LEVEL) && (DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG)
#define DEBUG_FUNCTION_ENTRY()     do {         Debug::println(4, String("[") + __MODULE__ + "][ENTER] " + __FUNCTION__);     } while(0)

#define DEBUG_FUNCTION_EXIT()     do {         Debug::println(4, String("[") + __MODULE__ + "][EXIT] " + __FUNCTION__);     } while(0)
#else
#define DEBUG_FUNCTION_ENTRY() do { } while(0)
#define DEBUG_FUNCTION_EXIT() do { } while(0)
#endif

// For temporary debugging (always shows when enabled)
#ifdef TEMP_DEBUG
#define DEBUG_TEMP(msg)     do {         Debug::println(3, String("[") + __MODULE__ + "][TEMP] " + msg);     } while(0)
#else
#define DEBUG_TEMP(msg) do { } while(0)
#endif

#endif // DEBUG_HELPER_H
```

### 2. Update Source Files to Use the Macros

Here's how to update your existing code to use these macros:

#### Before (Current Style):
```cpp
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][listDir] Directory contents:");
#endif
```

#### After (With Helper Macros):
```cpp
#define __MODULE__ "STORAGE"  // Define module name at the top of the file

#include "DebugHelper.h"

// In your functions:
void StorageManager::listDir(const String& path) {
    DEBUG_FUNCTION_ENTRY();  // Optional: for Level 4 tracing

    // Your code here...

    #if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][listDir] Directory contents:");
    #endif

    // Using the new macro:
    DEBUG_INFO("Directory contents:");

    DEBUG_FUNCTION_EXIT();  // Optional: for Level 4 tracing
}
```

### 3. Migration Strategy

#### Step 1: Add Module Definitions
Start by adding the module definition to each source file:
```cpp
#define __MODULE__ "STORAGE"  // At the top of StorageManager.cpp
#define __MODULE__ "IR"       // At the top of IRController.cpp
#define __MODULE__ "CORE"     // At the top of Core.cpp
// etc.
```

#### Step 2: Include DebugHelper.h
Add the include directive after the module definition:
```cpp
#include "DebugHelper.h"
```

#### Step 3: Replace Debug Statements
Replace existing debug statements with the appropriate macro:
```cpp
// Old:
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
    Debug::println(2, "[STORAGE][listDir] Directory contents:");
#endif

// New:
DEBUG_INFO("Directory contents:");
```

#### Step 4: Add Function Entry/Exit (Optional)
For better tracing in Level 4, add function entry/exit markers:
```cpp
void StorageManager::listDir(const String& path) {
    DEBUG_FUNCTION_ENTRY();
    // ... function code ...
    DEBUG_FUNCTION_EXIT();
}
```

### 4. Benefits of This Approach

1. **Consistency**: All debug statements follow the same format
2. **Less Typing**: No need to write the full `[MODULE][FUNCTION]` prefix each time
3. **Safety**: Macros prevent common mistakes like missing conditional compilation
4. **Readability**: Code is cleaner with `DEBUG_INFO("message")` instead of verbose debug statements
5. **Maintainability**: Easy to change debug format in one place (the header file)
6. **Performance**: Zero overhead when debug is disabled at compile time
7. **Flexibility**: Can add special features like function tracing

### 5. Example Usage

```cpp
// In IRController.cpp
#define __MODULE__ "IR"
#include "DebugHelper.h"

void IRController::update() {
    DEBUG_FUNCTION_ENTRY();

    if (IrReceiver.decode()) {
        unsigned long value = IrReceiver.decodedIRData.decodedRawData;

        // Error case
        if (value == 0) {
            DEBUG_ERROR("Invalid IR decode detected");
            return;
        }

        // Info case
        DEBUG_INFO("Received IR code: 0x" + String(value, HEX));

        // Verbose case
        DEBUG_DEBUG("Processing IR signal with " + 
                   String(IrReceiver.decodedIRData.numberOfBits) + " bits");
    }

    DEBUG_FUNCTION_EXIT();
}
```

### 6. WebUI Integration

The macros maintain compatibility with your existing WebUI color scheme:
- `DEBUG_ERROR()` → Level 1 → Red
- `DEBUG_WARN()` → Level 2 → Yellow
- `DEBUG_INFO()` → Level 3 → Blue
- `DEBUG_DEBUG()` → Level 4 → Grey

### 7. Special Considerations

1. **String Concatenation**: The macros use `String` objects which can be memory-intensive on Arduino. For memory-constrained environments, consider using `F()` macro for flash strings:
   ```cpp
   DEBUG_ERROR(F("Out of memory error"));
   ```

2. **Performance**: The macros are designed to have zero runtime overhead when debug is disabled.

3. **Module Naming**: Choose consistent module names that match your file structure or component names.

4. **Conditional Debugging**: The `TEMP_DEBUG` flag can be used for temporary debugging that's always visible when enabled, regardless of the DEBUG_LEVEL setting.
