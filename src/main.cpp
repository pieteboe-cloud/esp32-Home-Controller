#include <Arduino.h>
#include "../lib/Core/Core.h"
#include "../lib/Debug/Debug.h"

namespace {
constexpr unsigned long kSerialBaudRate = 115200;
}

// The app shell is intentionally thin: it just boots the debug logger,
// starts the Core subsystem, and then repeatedly updates the runtime loop.
class App {
public:
    void begin() {
        // Serial debug is the main observability tool during bring-up and runtime debugging.
        Debug::begin(kSerialBaudRate);
        Debug::logStartupBanner("HomeController", "ESP32");

#if DEBUG_LEVEL >= 1
        Debug::println(1, "[MAIN] Boot sequence started");
        Debug::println(1, "[MAIN] Starting application");
#endif

        Debug::logSubsystemStatus("System", "initializing", "storage -> hardware -> webui -> scripts");

        // Core owns the high-level startup order: hardware, translator, web UI, and bus subscriptions.
        core.init();

#if DEBUG_LEVEL >= 1
        Debug::println(1, "[MAIN] Core startup complete, application is running");
        Debug::println(1, "[MAIN] Application started");
#endif

        Debug::logSubsystemStatus("System", "ready", "Core, WebUI, and event pipeline are online");
    }

    void update() {
        // The main loop stays simple; the system is event-driven and periodically ticked here.
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
