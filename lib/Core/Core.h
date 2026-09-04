#pragma once
#include <Arduino.h>
#include "HardwareManager.h"
#include "WebUI.h"
#include "Debug.h"
#include "EventBus.h"
#include "Translator.h"
#include "SceneManager.h"
#include "FileSystemManager.h"
#include <FS.h>

// Firmware architecture map
// -----------------------------------------------------------------------------
// App
//   - boots the logger and starts the runtime
//   - stays thin and intentionally avoids subsystem implementation details
//
// Core
//   - owns the runtime orchestration and init order
//   - initializes hardware, scenes, WebUI, and EventBus subscriptions
//   - keeps the main loop minimal and event-driven
//
// HardwareManager
//   - owns physical I/O: RF, IR, Kaku, LivingColors, and storage
//   - publishes low-level device events to the EventBus
//
// SceneManager
//   - owns scene persistence, lookup, and execution
//   - loads scene data from the scene file and applies scenes to hardware
//
// ScriptManager
//   - owns action scripts, trigger matching, and action execution
//   - does not own scene loading or scene persistence
//
// WebUI
//   - owns the HTTP/API layer and dashboard assets
//   - starts the script runtime once the filesystem and network are ready
//
// EventBus
//   - is the communication broker between inputs and listeners
//   - keeps message flow centralized and easy to inspect
//
// Ownership rules
// - one subsystem should own one responsibility
// - Core decides startup order, but each subsystem owns its own setup work
// - ScriptManager and SceneManager should not duplicate each other's startup tasks
// - if the logic belongs to the hardware layer, keep it in HardwareManager
// -----------------------------------------------------------------------------

// Core is the runtime orchestrator for the firmware.
// It owns the startup sequence, event subscriptions, and the lightweight main-loop tick.
// Keeping the ownership boundaries here makes it easier to reason about which subsystem
// initializes first and which subsystem is responsible for what.
class Core {
public:
    Core();

    /**
     * Returns the hardware manager for direct subsystem access when needed.
     */
    HardwareManager& getHardware() { return hardware; }

    /**
     * Returns the scene manager for scene lookup and direct runtime configuration.
     */
    SceneManager& getSceneManager() { return sceneManager; }

    /**
     * Initializes the runtime in a single, documented order.
     *
     * Startup order:
     * 1. Storage + hardware devices
     * 2. Translator / protocol mapping
     * 3. Scene loading
     * 4. Web UI
     * 5. EventBus subscriptions
     */
    void init();

    /**
     * Runs the periodic runtime tick for the firmware.
     * This keeps the main loop simple while preserving the event-driven architecture.
     */
    void update();

    /**
     * Receives system events published by the EventBus.
     * Core only logs and coordinates, while action execution is handled elsewhere.
     */
    void handleSystemEvent(const SystemEvent& evt);

private:
    // Simple live heartbeat used during bring-up and runtime checks.
    unsigned long lastHeartbeatTime = 0;
    bool heartbeatState = LOW;
    const int HEARTBEAT_PIN = HEARTBEAT_LED_PIN;

    // Subsystems owned by Core.
    HardwareManager hardware;
    SceneManager sceneManager;
    WebUI webui;

    unsigned long lastUpdateTime = 0;

    // Kept for compatibility with the legacy Kaku event flow.
    void handleKaku(const RFCommand& cmd);
    
    // Generate a secure password for authentication
    String generateSecurePassword();
};
