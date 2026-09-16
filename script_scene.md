# Script Manager and Scene Manager

## Overview
This document explains the purpose and functionality of the Script Manager and Scene Manager in the ESP32 Home Controller system.

---

## Script Manager

### Purpose
The Script Manager is responsible for managing custom automation scripts in your home controller system. Scripts allow you to create sequences of commands that can be executed with a single trigger, making it easy to automate complex routines.

### Key Features
- **Script Storage**: Stores scripts in JSON format on the filesystem
- **Script Execution**: Executes scripts by sending commands to connected devices
- **Script Management**: Provides methods to add, update, delete, and list scripts
- **Command Parsing**: Parses and validates script commands before execution

### Script Structure
Each script contains:
- **ID**: Unique identifier for the script
- **Name**: Human-readable name for the script
- **Aliases**: Alternative names that can trigger the script
- **Commands**: Array of commands to execute when the script is triggered

### Example Script
```json
[
  {
    "id": 1,
    "name": "Movie Start",
    "aliases": [],
    "commands": [
      "living 1 rgb 255 100 20",
      "living 2 rgb 255 255 20",
      "living 3 rgb 255 100 220"
    ]
  }
]
```

### Common Commands
- `living <id> rgb <r> <g> <b>`: Set LivingColors light color
- `delay <ms>`: Pause execution for specified milliseconds
- `audio <command>`: Control audio playback

---

## Scene Manager

### Purpose
The Scene Manager manages predefined scenes in your home controller system. Scenes are named configurations that can be activated to instantly change the state of multiple devices, creating specific atmospheres or moods.

### Key Features
- **Scene Storage**: Stores scenes in JSON format on the filesystem
- **Scene Activation**: Activates scenes by sending device commands
- **Scene Management**: Provides methods to add, update, delete, and list scenes
- **Preset Configurations**: Stores predefined device settings for each scene

### Scene Structure
Each scene contains:
- **ID**: Unique identifier for the scene
- **Name**: Human-readable name for the scene
- **Type**: Category of the scene (e.g., "lighting", "mood", "activity")
- **Devices**: Array of device configurations for the scene

### Example Scene
```json
[
  {
    "id": 1,
    "name": "Movie Night",
    "type": "activity",
    "devices": [
      {
        "id": 1,
        "type": "light",
        "value": 50
      },
      {
        "id": 2,
        "type": "light",
        "value": 20
      }
    ]
  }
]
```

### Common Scene Types
- **Lighting**: Control brightness and colors of lights
- **Mood**: Create atmospheric settings
- **Activity**: Set up for specific activities (movie, dinner, etc.)
- **Security**: Configure alarm and surveillance settings

---

## Integration

### How They Work Together
While Scripts and Scenes serve different purposes, they can be used together:

1. **Scripts can activate scenes**: A script can include commands to activate specific scenes
2. **Scenes can be triggered by scripts**: Scenes can be part of larger automation scripts
3. **Web Interface**: Both can be controlled through the WebUI
4. **Event System**: Both can be triggered by system events

### Use Cases
- **Script Example**: "Good Morning" script that turns on lights, sets thermostat, and plays news
- **Scene Example**: "Movie Night" scene that dims lights and closes blinds

---

## File Locations
- Scripts: `/scripts.json`
- Scenes: `/scenes.json`

---

## API Endpoints
- Scripts: `/api/action-scripts` (GET for listing, POST for adding/updating)
- Scenes: `/api/scenes` (GET for listing, POST for adding/updating)

---

## Conclusion
The Script Manager and Scene Manager provide powerful automation capabilities for your home controller system. Scripts are ideal for complex sequences of actions, while scenes are perfect for instant atmosphere changes. Together, they create a flexible and user-friendly home automation system.
