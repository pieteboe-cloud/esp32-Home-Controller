#pragma once
#include <Arduino.h>
#include "HardwareManager.h"
#include "WebUI.h"
#include "Debug.h"
#include "EventBus.h"
#include "Translator.h"
#include "SceneManager.h"
#include <FS.h>

class Core {
public:
    Core();
    HardwareManager& getHardware() { return hardware; }
    SceneManager& getSceneManager() { return sceneManager; }
    void init();
    void update();
    void handleSystemEvent(const SystemEvent& evt);
private:
    unsigned long lastHeartbeatTime = 0;
    bool heartbeatState = LOW;
    const int HEARTBEAT_PIN = 32;
    HardwareManager hardware;
    SceneManager sceneManager;
    WebUI webui;
    unsigned long lastUpdateTime = 0;
    void handleKaku(const RFCommand& cmd);
};
