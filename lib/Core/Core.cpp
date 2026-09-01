#include "Core.h"

Core::Core() : hardware(), sceneManager(hardware), webui(hardware, sceneManager) {}

void Core::init() {
    // Heartbeat pin is used as a simple live indicator that the firmware is alive.
    pinMode(HEARTBEAT_PIN, OUTPUT);
    heartbeatState = LOW;
    lastHeartbeatTime = millis();

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][init] Starting Core::init()...");
    Debug::println(2, "[CORE][init] Configuring heartbeat pin and startup state");
    Debug::println(2, "[CORE][init] Startup order: storage -> hardware -> translator -> webui -> eventbus");
    Debug::println(2, "[CORE][init] Heartbeat pin = " + String(HEARTBEAT_PIN) + ", period = 500ms");
#endif

    // The hardware layer owns RF/IR/storage and publishes device events to the bus.
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][init] Initializing hardware manager");
#endif
    hardware.init();

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][init] Initializing action translator");
#endif
    Translator::init();

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][init] Initializing scene manager");
#endif
    sceneManager.begin();

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][init] Starting web UI");
#endif
    webui.begin();

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][init] Subscribing to EventBus");
#endif
    EventBus::getInstance().subscribe([this](const SystemEvent& evt) {
        this->handleSystemEvent(evt);
    });

    hardware.onStorageEvent([this](const String& event, const String& details) {
#if DEBUG_LEVEL >= 2
        Debug::println(2, "[CORE][STORAGE] Event received: " + event + " - " + details);
#endif
    });

#if DEBUG_LEVEL >= 1
    Debug::println(1, "[CORE][SUMMARY] System ready: hardware initialized, translator loaded, WebUI online, EventBus active");
#endif

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE] Core initialization complete");
#endif
}

void Core::update() {
    // HardwareManager remains the single owner of the hardware polling.
    // The Core loop is deliberately lightweight so the board can handle the event-driven logic.
    hardware.update();

    if (millis() - lastHeartbeatTime >= 500) {
        lastHeartbeatTime = millis();
        heartbeatState = !heartbeatState;
        digitalWrite(HEARTBEAT_PIN, heartbeatState);
        webui.setHeartbeatState(heartbeatState);

#if DEBUG_LEVEL >= 3
        Debug::println(3, "[CORE][heartbeat] Heartbeat toggled to " + String(heartbeatState ? "HIGH" : "LOW"));
#endif
    }
}

void Core::handleKaku(const RFCommand& cmd) {
    Debug::println(2, "[CORE][KAKU] House " + String(cmd.house) + " Button " + String(cmd.button));
}

void Core::handleSystemEvent(const SystemEvent& evt) {
    // Action execution is owned by ScriptManager. Core only logs the event,
    // preventing the old hard-coded C-2/virtual-color rules from firing twice.
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][EVENT] " + evt.source + " -> " + evt.identifier);
#endif
}
