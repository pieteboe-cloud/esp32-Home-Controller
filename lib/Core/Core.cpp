#include "Core.h"
#include <stdlib.h>
#include <time.h>

Core::Core() : hardware(), sceneManager(hardware), webui(hardware, sceneManager) {}

/**
 * Initializes the runtime in a clear, ownership-driven order.
 *
 * The goal is to keep one source of truth for startup ordering and ensure each
 * subsystem is started only by the layer that owns it. Hardware owns storage and
 * device I/O, Core owns scene loading and orchestration, and WebUI brings up the
 * script runtime once the filesystem and network are ready.
 */
void Core::init() {
    // Startup flow and ownership summary:
    // 1) Initialize filesystem first
    // 2) HardwareManager initializes storage + RF/IR/Kaku/LivingColors.
    // 2) Core initializes the translator, scene manager, and WebUI.
    // 3) WebUI starts ScriptManager so action scripts and web endpoints are ready.
    // 4) EventBus subscriptions are registered last so all publishers are active.
    // 5) The system stays event-driven: input publishers emit, ScriptManager reacts.
    Debug::logSubsystemStatus("System", "initializing", "filesystem -> hardware -> translator -> scene manager -> webui -> scripts -> eventbus");

    // Heartbeat pin is used as a simple live indicator that the firmware is alive.
    pinMode(HEARTBEAT_PIN, OUTPUT);
    heartbeatState = LOW;


    // The hardware layer owns RF/IR/storage and publishes device events to the bus.

    // Initialize filesystem first
    if (!Storage::begin()) {
        Debug::println(1, "[CORE][ERROR] Failed to initialize filesystem");
        return;
    }

    hardware.init();

    Translator::init();

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE][init] Initializing scene manager");
#endif
    sceneManager.begin();

    // Initialize authentication BEFORE the web UI starts the access point,
    // so softAP() is called with the real password rather than the default.
    String fixedPassword = "admin123"; // You can change this to any password you prefer
    webui.setAuthentication(true, fixedPassword);
    webui.setAPPassword(fixedPassword);
    webui.setFixedPassword(fixedPassword);

    webui.begin();
    Debug::println(2, "[CORE] Authentication enabled with fixed password: " + fixedPassword);

    EventBus::getInstance().subscribe([this](const SystemEvent& evt) {this->handleSystemEvent(evt);});
}

/**
 * Performs the periodic runtime tick for the app.
 *
 * The loop deliberately stays lightweight so the board can remain responsive to
 * event-driven hardware and UI callbacks.
 */
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

/**
 * Handles system events after they are published on the EventBus.
 *
 * Core keeps this responsibility intentionally narrow: it logs and observes the
 * event flow without re-implementing action execution logic that belongs to the
 * script system.
 */
void Core::handleSystemEvent(const SystemEvent& evt) {
    // Action execution is owned by ScriptManager. Core only logs the event,
#if DEBUG_LEVEL >= 3
    if (evt.source == "AUDIO") {
        Debug::println(3, "[CORE][AUDIO] " + evt.identifier + " | " + evt.rawData);
    } else {
        Debug::println(3, "[CORE][EVENT] " + evt.source + " -> " + evt.identifier);
    }
#endif
    
#if DEBUG_LEVEL >= 2
    // Only log non-audio events at level 2 to avoid flooding
    if (evt.source != "AUDIO") {
        Debug::println(2, "[CORE][EVENT] " + evt.source + " -> " + evt.identifier);
    }
#endif
}

String Core::generateSecurePassword() {
    // Initialize random seed if not already done
    static bool seedInitialized = false;
    if (!seedInitialized) {
        randomSeed(millis());
        seedInitialized = true;
    }

    // Generate a random 8-character password
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int length = 8;
    String password = "";

    for (int i = 0; i < length; i++) {
        int index = random(sizeof(charset) - 1);
        password += charset[index];
    }

    return password;
}
