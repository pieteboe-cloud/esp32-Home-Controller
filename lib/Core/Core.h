#pragma once

#include <Arduino.h>

#include "../HardwareManager/HardwareManager.h"
#include "../ScriptManager/ScriptManager.h"
#include "../SceneManager/SceneManager.h"
#include "../WebUI/WebUI.h"

#include "Debug.h"
#include "EventBus.h"

// -----------------------------------------------------------------------------
// CommandSink
// -----------------------------------------------------------------------------
//
// ScriptManager, SceneManager and future effect/automation components can send
// canonical commands without knowing anything about HardwareManager.
//
// Core is the ONLY class that implements this interface and therefore the only
// layer above HardwareManager that knows how hardware commands are executed.
//
// Example commands:
//
//   ir TV POWER
//   living 1 rgb 255 0 0
//   living 1 hsv 120 255 255
//   delay 1000
//
// The exact command vocabulary belongs to Core/the translator layer.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Core
// -----------------------------------------------------------------------------
//
// Core is the application orchestrator.
//
// Responsibilities:
//
//   - Own HardwareManager
//   - Own ScriptManager
//   - Own SceneManager
//   - Own WebUI
//   - Define startup order
//   - Receive EventBus events
//   - Route Kaku events to SceneManager
//   - Execute canonical commands against HardwareManager
//   - Keep the main update loop small
//
// Important architecture rule:
//
//   WEBUI / KAKU / AUDIO / OTHER INPUTS
//                    |
//                Translator
//                    |
//                  Core
//                 /    \
//             Scenes   Scripts
//                 \    /
//               Core commands
//                    |
//             HardwareManager
//
// SceneManager and ScriptManager MUST NOT know about HardwareManager.
// -----------------------------------------------------------------------------
class Core : public CommandSink
{
public:
    Core();

    // -------------------------------------------------------------------------
    // Runtime
    // -------------------------------------------------------------------------

    void init();
    void update();

    // -------------------------------------------------------------------------
    // CommandSink implementation
    // -------------------------------------------------------------------------

    bool executeCommand(const String& command) override;

    // -------------------------------------------------------------------------
    // Controlled access
    // -------------------------------------------------------------------------
    //
    // HardwareManager is intentionally exposed only through Core.
    //
    // Other subsystems should NOT receive this reference. They should send
    // commands through CommandSink instead.
    //
    HardwareManager& getHardware() { return hardware; }

    SceneManager& getSceneManager() { return sceneManager; }
    ScriptManager& getScriptManager() { return scriptManager; }

    // -------------------------------------------------------------------------
    // EventBus
    // -------------------------------------------------------------------------

    void handleSystemEvent(const SystemEvent& evt);

private:

    // -------------------------------------------------------------------------
    // Subsystems
    // -------------------------------------------------------------------------

    HardwareManager hardware;
    ScriptManager scriptManager;
    SceneManager sceneManager;
    WebUI webui;

    // -------------------------------------------------------------------------
    // Heartbeat
    // -------------------------------------------------------------------------

    unsigned long lastHeartbeatTime = 0;
    bool heartbeatState = LOW;

    const int HEARTBEAT_PIN = HEARTBEAT_LED_PIN;

    // -------------------------------------------------------------------------
    // Event handling
    // -------------------------------------------------------------------------

    void handleKaku(const RFCommand& cmd);

    // -------------------------------------------------------------------------
    // Command execution
    // -------------------------------------------------------------------------

    bool executeIRCommand(const String& command);
    bool executeLivingColorsCommand(const String& command);
    bool executeDelayCommand(const String& command);

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    String generateSecurePassword();
};

