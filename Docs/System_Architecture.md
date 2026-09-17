# System Architecture Overview

## Object Creation and Initialization

### Main Application
- **Created by**: System entry point (main.cpp)
- **Purpose**: Initializes the entire system and manages the main loop

### HardwareManager
- **Created by**: Main application
- **Purpose**: Manages all hardware controllers (IR, LivingColors, NeoPixel, KAKU)
- **Creates**: 
  - IRController (for IR communication)
  - LivingColorsController (for RF lamp control)
  - NeoPixelController (for addressable LEDs)
  - KAKUController (for RF switches)

### SceneManager
- **Created by**: Main application or HardwareManager
- **Purpose**: Manages scenes and maps triggers to scripts
- **Creates**: Scene objects

### ScriptManager
- **Created by**: Main application or SceneManager
- **Purpose**: Manages scripts and executes commands
- **Creates**: Script objects

### Translator
- **Created by**: Main application or ScriptManager
- **Purpose**: Converts virtual colors to hardware-specific commands
- **Does not create objects**: Acts as a conversion layer

### WebUI
- **Created by**: Main application
- **Purpose**: Provides web interface for control and configuration
- **Creates**: Web server and API endpoints

## Update Loop Execution

### Main Loop
- **Called by**: System entry point (main.cpp)
- **Purpose**: Runs the main system loop
- **Calls**:
  - HardwareManager.update()
  - SceneManager.update()
  - ScriptManager.update()
  - WebUI.update()

### HardwareManager.update()
- **Called by**: Main loop
- **Purpose**: Updates all hardware controllers
- **Calls**:
  - IRController.update()
  - LivingColorsController.update()
  - NeoPixelController.update()
  - KAKUController.update()

### Component Updates
- **Called by**: HardwareManager.update()
- **Purpose**: Process hardware-specific updates
- **Examples**:
  - IRController.update(): Checks for incoming IR signals
  - KAKUController.update(): Checks for incoming RF signals

## Callback Registration

### IRController
- **Registered by**: SceneManager or Translator
- **Method**: onCommand()
- **Purpose**: Handle incoming IR commands

### KAKUController
- **Registered by**: SceneManager
- **Method**: onCommand()
- **Purpose**: Handle incoming KAKU RF events

### LivingColorsController
- **Registered by**: ScriptManager or Translator
- **Method**: onCommand()
- **Purpose**: Handle lamp commands

### NeoPixelController
- **Registered by**: ScriptManager or Translator
- **Method**: onCommand()
- **Purpose**: Handle LED strip commands

## EventBus Usage

### Publishers
- **IRController**: Publishes IR_RECEIVED events with decoded IR data
- **KAKUController**: Publishes KAKU events with house/button information
- **ScriptManager**: Publishes SCRIPT events with script execution status
- **SceneManager**: Publishes SCENE events with scene activation information

### Subscribers
- **SceneManager**: Subscribes to IR_RECEIVED and KAKU events
- **ScriptManager**: Subscribes to SCENE events
- **Translator**: Subscribes to events requiring color translation
- **WebUI**: Subscribes to various events for real-time updates

## Event Flow Example (KAKU Button Press)

1. **KAKUController** receives RF signal
2. **KAKUController** publishes KAKU event (e.g., "House D Button 4")
3. **SceneManager** consumes the KAKU event
4. **SceneManager** finds matching scene and publishes SCENE event
5. **ScriptManager** consumes the SCENE event
6. **ScriptManager** loads and executes script commands
7. **ScriptManager** publishes SCRIPT events for each command
8. **Translator** consumes SCRIPT events and converts to hardware commands
9. **Hardware controllers** (LivingColors, NeoPixel, etc.) consume converted commands
10. **WebUI** may consume events to update UI in real-time

## Data Flow

### Virtual Color Processing
1. **Script** references virtual color (e.g., "VC_RED")
2. **Translator** converts to hex value (e.g., "#FF0000")
3. **Translator** converts hex to hardware-specific command:
   - For LivingColors: RGB values
   - For NeoPixels: RGB values
   - For IR strips: IR codes

### Scene Processing
1. **KAKU/IR event** triggers scene
2. **SceneManager** finds matching scene
3. **SceneManager** activates associated script
4. **ScriptManager** executes script commands
5. **Hardware controllers** execute commands

## Debug System

### Debug Levels
- **Level 1**: Errors only
- **Level 2**: Warnings and important events
- **Level 3**: Detailed operation information
- **Level 4**: Flood of all possible information

### Debug Output
- **Published by**: All system components
- **Consumed by**: Debug system and WebUI
- **Purpose**: System monitoring and troubleshooting
