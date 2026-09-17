# IRController Component Relationships

## Object Creation
- **Created by**: HardwareManager or similar system component
- **Constructor parameters**: `rxPin` (receiver pin) and `txPin` (transmitter pin)
- **Purpose**: To handle all IR communication (receiving and transmitting)

## Initialization
- **Called by**: System initialization code (likely in HardwareManager or main setup)
- **Method**: `init()` 
- **Actions**:
  - Initializes the IR receiver with LED feedback
  - Initializes the IR transmitter without LED feedback
  - Logs initialization at debug level 2 or higher

## Update Loop
- **Called by**: Main system loop (likely via HardwareManager's update loop)
- **Method**: `update()`
- **Purpose**: Polls the IR receiver for incoming signals
- **Frequency**: Must be called regularly from the main loop

## Callback Registration
- **Registered by**: System components that need to handle IR commands (e.g., SceneManager, Translator)
- **Method**: `onCommand(IRCallback callback)`
- **Purpose**: Sets up a callback function to be invoked when an IR command is received

## Event Flow

### IR Reception Flow:
1. **IR Controller** receives IR signal
2. **IR Controller** processes and validates the signal
3. **IR Controller** calls the registered callback with an IRCommand structure
4. **Callback** (e.g., SceneManager/Translator) processes the command

### IR Transmission Flow:
1. **System component** calls `IRController::send()` with an IRCommand
2. **IR Controller** transmits the signal
3. **IR Controller** temporarily disables reception during transmission
4. **IR Controller** re-enables reception after transmission

## Data Structures
- **IRCommand**: Contains:
  - `code`: Decoded IR value
  - `bits`: Number of bits in the signal
  - `protocol`: Protocol type
  - `timestamp`: When the signal was received
  - `rawCode[]`: Raw waveform data
  - `rawCodeLength`: Length of raw data
  - `hasRaw`: Whether raw data is available

## Debug Output
- **Level 2**: Shows initialization and received codes
- **Level 3**: Shows receiver enable/disable operations
- **Transmission**: Shows sending mode and code details

## Key Interactions
1. **With IR Hardware**: Direct control via IRremote library
2. **With System Components**: Via callback functions
3. **With Debug System**: Via Debug::println calls
4. **With Event System**: Likely through the callback mechanism

## Debouncing
- **Purpose**: Prevents button-hold from flooding events
- **Implementation**: Ignores repeated codes within 300ms window
- **Managed by**: `_lastCodeValue` and `_lastCodeTime` tracking

## Raw Waveform Support
- **Purpose**: Bit-exact replay of received signals
- **Implementation**: Captures raw timing data during reception
- **Usage**: When sending, can replay exact waveform instead of decoded value
