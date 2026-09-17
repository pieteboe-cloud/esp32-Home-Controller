# Log Smoothing for KAKU Button Press

## Current Log Output
When a KAKU button is pressed, the system logs every step of the process. This is useful for debugging but can be verbose for normal operation.

### Example Current Log:
```
[17:49:04] [RF][receive] Received code: 0x44014 / 24 bits / protocol=1 / pulse=366us
[17:49:04] [CORE][EVENT] RF -> RF_RAW_44014
[17:49:04] [KAKU][processValue] Decoded House D Button 4 - Event published via HardwareManager callback
[17:49:04] [CORE][EVENT] KAKU -> D_4
[17:49:04] [CORE][KAKU] House D Button 4
[17:49:04] [SceneManager] Kaku D4 → Scene 1
[17:49:04] [SceneManager] Activating scene: Movie
[17:49:04] [ScriptManager] runScript(name): Movie Start
[17:49:04] [ScriptManager] Looking for script: Movie Start
[17:49:04] [ScriptManager] Name match: Movie Start
[17:49:04] ================================================
[17:49:04] [ScriptManager] Running script: Movie Start
[17:49:04] [ScriptManager] Script ID: 1
[17:49:04] [ScriptManager] Commands JSON: ["living 1 rgb 255 100 20",...]
[17:49:04] [ScriptManager] Command count: 7
[17:49:04] [ScriptManager] Processing command #0
[17:49:04] [ScriptManager] Command #0: living 1 rgb 255 100 20
[17:49:04] [ScriptManager] → CommandSink
[17:49:04] [CORE][LIVING] Lamp 1 -> index 0 RGB 255 100 20
[17:49:04] [ScriptManager] ← CommandSink result: SUCCESS
[17:49:04] [ScriptManager] Processing command #1
[17:49:04] [ScriptManager] Command #1: living 2 rgb 255 255 20
[17:49:04] [ScriptManager] → CommandSink
[17:49:04] [CORE][LIVING] Lamp 2 -> index 1 RGB 255 255 20
[17:49:04] [ScriptManager] ← CommandSink result: SUCCESS
... (repeats for all 7 commands)
[17:49:05] [ScriptManager] Script completed: Movie Start
[17:49:05] [ScriptManager] Overall result: SUCCESS
[17:49:05] ================================================
```

## Proposed Smoother Log Output
To reduce log verbosity while maintaining essential information, we can implement a logging level system and group related messages.

### Example Optimized Log:
```
[17:49:04] [KAKU] House D Button 4 pressed
[17:49:04] [SCENE] Activating "Movie" scene → "Movie Start" script
[17:49:04] [SCRIPT] Executing "Movie Start" (7 commands)
[17:49:05] [SCRIPT] "Movie Start" completed successfully
```

## Implementation Options

### Option 1: Debug Level Filtering
Implement different debug levels:
- Level 1: Errors only
- Level 2: Warnings and errors
- Level 3: Normal operation (scene/script changes)
- Level 4: Detailed (current default)

### Option 2: Grouped Logging
Group related log messages:
```
[KAKU] House D Button 4 → [SCENE] Movie → [SCRIPT] Movie Start (7 commands, SUCCESS)
```

### Option 3: Event-Based Summary
Log only key events:
```
[EVENT] KAKU D-4 → Scene "Movie" → Script "Movie Start" (SUCCESS)
```

## Benefits of Smoother Logging
1. **Reduced Log Volume**: Less clutter in the output
2. **Faster Reading**: Easier to identify important events
3. **Better Debugging**: Still able to enable detailed logs when needed
4. **Improved Performance**: Less logging overhead

## Recommendation
A combination of Option 1 (debug levels) and Option 2 (grouped logging) would provide the best balance:
- Default to level 3 for normal operation
- Allow users to increase detail level when needed
- Group related messages for better readability
