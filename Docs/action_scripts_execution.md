# Action Scripts HTML - Hardware Command Execution

## Can action_scripts.html Execute Hardware Commands?

**Yes, but indirectly.** The action_scripts.html file is a web interface that creates and manages scripts, but it doesn't directly execute hardware commands. Instead, it sends commands to the backend system which then executes them.

## How Hardware Commands Are Executed

### 1. Command Creation in the WebUI
Users can create hardware commands through the web interface:

#### LivingColors Commands:
- Created via: `addLiving()` function (line 373)
- Format: `living <lamp> rgb <r> <g> <b>`
- Example: `living 1 rgb 255 100 20`

#### RGB LED Strip Commands:
- Created via: `addStrip()` function (line 424)
- Format: `rgbstrip <remote> <hex>`
- Example: `rgbstrip RGB_24KEY-R1 #FF6414`

### 2. Command Execution Flow

#### Test Commands:
- Triggered by: `testLiving()` (line 384) and `testStrip()` (line 432)
- Process: `executeTemporary()` function (line 548)
- API Endpoint: `/api/execute-script`
- Method: POST request with script JSON

#### Script Execution:
- Triggered by: `runCurrentScript()` function (line 540)
- Process: `executeTemporary()` function (line 548)
- API Endpoint: `/api/execute-script`
- Method: POST request with script JSON

### 3. Backend Processing
The web interface sends commands to the backend via these API endpoints:

- `POST /api/execute-script` - Executes a script
- `POST /api/action-scripts` - Saves a script
- `GET /api/action-scripts` - Loads scripts
- `DELETE /api/action-scripts?id=<id>` - Deletes a script

### 4. Command Execution Architecture
1. **WebUI** creates commands and sends them to the backend
2. **Backend** receives commands via API endpoints
3. **ScriptManager** processes the script
4. **Translator** converts commands to hardware-specific formats
5. **Hardware Controllers** (IRController, LivingColorsController, etc.) execute the commands

### 5. Key Functions for Command Execution

#### executeTemporary() (line 548)
- Creates a temporary script object
- Sends it to the backend via `/api/execute-script`
- Handles success/error responses

#### runCurrentScript() (line 540)
- Executes the currently selected script
- Calls `executeTemporary()` with the script's commands

#### testLiving() / testStrip() (line 384/432)
- Test individual commands before adding to a script
- Call `executeTemporary()` with a single command

## Summary

The action_scripts.html file is a user interface for creating and managing scripts that control hardware. It doesn't directly control hardware but rather:

1. Provides a UI for users to create commands
2. Sends these commands to the backend via API calls
3. The backend processes these commands through the ScriptManager and Translator
4. Hardware controllers then execute the actual hardware commands

This separation allows for:
- A clean user interface
- Secure command execution
- Centralized command processing
- Support for multiple hardware types
