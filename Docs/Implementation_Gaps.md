# Implementation Gaps: Next Steps

## Current Understanding

| Component | Known Function |
|----------|---------------|
| HardwareManager | Physical hardware boundary; publishes system events |
| IRController | Receives/sends IR; no color semantics |
| Translator | Current proven role: IR raw code → VC_* |
| SceneManager | D-4 demonstrably selects scene |
| ScriptManager | D-4 demonstrably executes script commands |
| action_scripts.html | Builds/sends script commands through HTTP API |
| fallbackRemotes | Browser-side RGB-strip catalogue |

## Implementation Gaps to Address

### 1. rgbstrip backend handling
**Status**: Unknown  
**Question**: How are RGB strip commands processed on the backend?

**Investigation needed**:
- Search for `rgbstrip` command processing in the codebase
- Look for handlers that process commands starting with "rgbstrip"
- Identify the component that translates RGB strip commands to IR codes

**Expected findings**:
- A handler function that processes `rgbstrip <remote> <hex>` commands
- Translation from hex color to IR codes for specific remotes
- Integration with the IRController for sending commands

### 2. VC_* consumer
**Status**: Unknown  
**Question**: Which components consume virtual colors (VC_*)?

**Investigation needed**:
- Search for references to "VC_" in the codebase
- Identify components that use virtual color system
- Understand how virtual colors are converted to hardware commands

**Expected findings**:
- Components that translate VC_* to hardware-specific commands
- The relationship between Translator and other color consumers
- How the virtual color system integrates with the script execution

### 3. IRController::send() caller
**Status**: Unknown  
**Question**: Which components call IRController::send() to transmit IR commands?

**Investigation needed**:
- Search for calls to IRController::send() or equivalent methods
- Identify the components responsible for sending IR commands
- Understand the flow from script commands to IR transmission

**Expected findings**:
- The component that initiates IR transmission
- How script commands are converted to IR commands
- The relationship between ScriptManager/Translator and IRController

## Investigation Plan

### For rgbstrip backend handling:
1. Search the codebase for "rgbstrip" command handlers
2. Look for HTTP API endpoints that process RGB strip commands
3. Trace the execution flow from command reception to IR transmission

### For VC_* consumer:
1. Search the codebase for "VC_" references
2. Identify components that use the virtual color system
3. Trace how virtual colors are converted to hardware commands

### For IRController::send() caller:
1. Search for calls to IRController::send() method
2. Identify the component that initiates IR transmission
3. Understand the flow from script commands to IR commands

## Expected Outcomes

After addressing these gaps, we should have a complete understanding of:

1. How RGB strip commands are processed from web interface to IR transmission
2. Which components use the virtual color system and how they integrate with other components
3. The complete flow from script creation to hardware command execution

This will provide a comprehensive view of the system architecture and help identify any remaining implementation gaps.
