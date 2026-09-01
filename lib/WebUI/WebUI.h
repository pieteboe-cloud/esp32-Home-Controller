#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "Debug.h"
#include "../HardwareManager/HardwareManager.h"
#include "../ScriptManager/ScriptManager.h"
#include "../SceneManager/SceneManager.h"

class WebUI {
public:
    // Start the access point, mount the web filesystem, and register HTTP routes.
    explicit WebUI(HardwareManager& hardware, SceneManager& sceneManager);
    void begin();
    void beginAP();
    void loop();

    // The dashboard uses this heartbeat value to distinguish a live device from
    // a lost connection. It is intentionally separate from action diagnostics.
    void setHeartbeatState(bool state) { heartbeatState = state; }
    SceneManager& getSceneManager() { return sceneManagerRef; }

private:
    AsyncWebServer server;
    ScriptManager scriptManager;
    SceneManager& sceneManagerRef;  // Reference to Core's SceneManager
    volatile bool heartbeatState = false;

    // Keep route registration in one place so every web action has a clear
    // owner and can emit useful diagnostics through Debug.
    void setupRoutes();
};
