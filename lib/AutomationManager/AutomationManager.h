#ifndef AUTOMATION_MANAGER_H
#define AUTOMATION_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "RF433.h"
#include "IRController.h"
#include "Debug.h"

// Forward declaration to avoid circular dependencies
class ControlLivingColors;

// Action types for automation sequences
enum ActionType {
    ACTION_RF_SEND,
    ACTION_IR_SEND,
    ACTION_DELAY,
    ACTION_RGB_SET
};

// Single action in a sequence
struct Action {
    ActionType type;
    unsigned long value;      // RF code or IR code or delay in ms
    int bitLength;            // Bit length for RF/IR
    int protocol;             // Protocol for RF/IR
    int rgbRed;               // RGB red value (for RGB actions)
    int rgbGreen;             // RGB green value (for RGB actions)
    int rgbBlue;              // RGB blue value (for RGB actions)
};

// Automation sequence (scene)
class AutomationSequence {
private:
    std::vector<Action> actions;
    String name;

public:
    AutomationSequence(String seqName) : name(seqName) {}

    void addAction(ActionType type, unsigned long value = 0, int bitLength = 0,
                   int protocol = 0, int r = 0, int g = 0, int b = 0) {
        actions.push_back({type, value, bitLength, protocol, r, g, b});
    }

    void execute(RF433& rf, IRController& ir, ControlLivingColors* lampControl, uint8_t (*addresses)[9]);

    String getName() const { return name; }
    int getActionCount() const { return actions.size(); }
};

// Scene trigger (RF button -> automation sequence)
struct SceneTrigger {
    unsigned long rfCode;
    int bitLength;
    int protocol;
    String sceneName;
};

// Main automation manager
class AutomationManager {
private:
    std::vector<AutomationSequence> sequences;
    std::vector<SceneTrigger> triggers;
    RF433& rfModule;
    IRController& irModule;
    ControlLivingColors* lampController = nullptr;
    uint8_t (*lampAddresses)[9] = nullptr;

    AutomationSequence* findSequence(String name);

public:
    AutomationManager(RF433& rf, IRController& ir) : rfModule(rf), irModule(ir) {}
    
    void setLampController(ControlLivingColors* control, uint8_t (*addresses)[9]) {
        lampController = control;
        lampAddresses = addresses;
    }

    // Create a new automation sequence
    AutomationSequence* createSequence(String name);

    // Setup default scenes (Movie Night, All Off, etc)
    void initDefaultScenes();

    // Initialize default RF triggers
    void initDefaultTriggers();

    // Add a trigger (RF button -> scene)
    void addTrigger(unsigned long rfCode, int bitLength, int protocol, String sceneName);

    // Check if received RF code triggers a scene
    bool checkTrigger(unsigned long rfCode, int bitLength, int protocol);

    // Execute a scene by name
    bool executeScene(String name);

    // Get list of all scene names
    std::vector<String> getSceneNames();
};

#endif // AUTOMATION_MANAGER_H
