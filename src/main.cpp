#include <Arduino.h>
#include "Debug.h"
#include "Core.h"


// Read this first when debugging:
// -----------------------------------------------------------------------------
// Startup flow:
//   1) App::begin() boots the logger and starts Core
//   2) Core::init() initializes hardware, translator, scenes, and WebUI
//   3) WebUI starts ScriptManager for action scripts and API routes
//   4) EventBus subscriptions are registered last
//   5) loop() stays simple and calls Core::update()
//
// Ownership:
//   App            -> top-level shell, keeps startup thin
//   Core           -> orchestration owner and runtime coordinator
//   HardwareManager-> physical devices + storage + event publishing
//   SceneManager   -> scenes, persistence, scene execution
//   ScriptManager  -> scripts, triggers, action execution
//   WebUI          -> dashboard + HTTP API + script runtime wiring
//   EventBus       -> shared event broker for all subsystems
// -----------------------------------------------------------------------------

// App is the top-level orchestration shell.
// It boots the debug console, launches the Core runtime, and then hands control
// to the main loop. Keeping this class thin makes the startup path easier to follow.
class App {
public:
    /**
     * Bootstraps the firmware and initializes the system runtime.
     *
     * This method should stay intentionally small: it configures logging,
     * then delegates all intelligent startup coordination to Core::init().
     */
    void begin() {
        Debug::begin(115200, Debug::INFO);

        #ifdef DEBUG_LEVEL
            if (Debug::getDebugLevel() >=  0) {
                Debug::println(1, "[MAIN] Debug level set to " + String(Debug::getDebugLevel()));
            }
        #endif

        

        Debug::logStartupBanner("HomeController", "ESP32");

        // Core owns the startup sequence and the single status log for the system lifecycle.
        core.init();

    }

    /**
     * Runs the runtime loop for the application.
     *
     * The system is event-driven, so this method remains deliberately minimal and
     * simply forwards control to the Core runtime tick.
     */
    void update() {
        core.update();
    }

private:
    Core core;
};

App app;

void setup() {
    app.begin();
}

void loop() {
    app.update();
}
