# ESP32 Home Controller - Automation System

## Overview

The automation system allows you to create sequences of actions (called "scenes") that can be triggered by:
- RF remote buttons
- Web UI commands

## How It Works

### Scenes
A **Scene** is a sequence of actions that execute one after another. Each action can be:
- **RF_SEND**: Send a 433MHz code to control RF switches/dimmers
- **IR_SEND**: Send an IR code to control TVs, AC, etc.
- **DELAY**: Wait for a specified time (in milliseconds)
- **RGB_SET**: Set RGB lights to a specific color

### Triggers
A **Trigger** maps an RF button press to a scene. When you press a button on your RF remote, the system checks if there's a trigger for that button code and executes the associated scene.

## Example Usage

### Example 1: Movie Night Scene

```cpp
// Create the scene
AutomationSequence* movieNight = automation.createSequence("movie_night");

// Add actions to the scene
movieNight->addAction(ACTION_RF_SEND, 1234567, 24, 1);  // Turn on RF switch
movieNight->addAction(ACTION_IR_SEND, 0x00FF45BA, 32, 1); // TV power
movieNight->addAction(ACTION_DELAY, 30000, 0, 0);         // Wait 30 seconds
movieNight->addAction(ACTION_IR_SEND, 0x00FF12ED, 32, 1); // TV HDMI input
movieNight->addAction(ACTION_RGB_SET, 255, 50, 50);      // Set RGB to dim red

// Map RF button to the scene
automation.addTrigger(1234567, 24, 1, "movie_night");
```

### Example 2: All Off Scene

```cpp
// Create the scene
AutomationSequence* allOff = automation.createSequence("all_off");

// Add actions to turn everything off
allOff->addAction(ACTION_RF_SEND, 7654321, 24, 1);  // Turn off RF switch 1
allOff->addAction(ACTION_RF_SEND, 7654322, 24, 1);  // Turn off RF switch 2
allOff->addAction(ACTION_IR_SEND, 0x00FF45BA, 32, 1); // TV power

// Map RF button to the scene
automation.addTrigger(7654321, 24, 1, "all_off");
```

## Capturing RF/IR Codes

To capture the codes from your remotes:

1. **RF Codes**: Watch the serial monitor when you press RF buttons. You'll see output like:
   ```
   Received: 1234567 / 24bit / Protocol: 1
   ```

2. **IR Codes**: Watch the serial monitor when you press IR buttons. You'll see output like:
   ```
   IR Received: 45BA / 32bit / Protocol: 1
   ```

Use these values when creating your automation scenes.

## Triggering Scenes from Web UI

You can also trigger scenes from your web UI by sending the scene name as a command. For example:
- Send command: `movie_night` → Executes the movie night scene
- Send command: `all_off` → Executes the all off scene

## Adding Your Own Scenes

1. Open `main.cpp`
2. Find the `setupAutomation()` function
3. Add your scene creation code following the examples
4. Upload to your ESP32

## Tips

- Use descriptive scene names like "morning_routine", "movie_night", "bedtime"
- Test each action individually before creating complex scenes
- Use delays between IR commands to give devices time to respond
- Document your RF/IR codes for future reference
