#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "Debug.h"
#include "../HardwareManager/HardwareManager.h"
#include "../ScriptManager/ScriptManager.h"
#include "../SceneManager/SceneManager.h"

/**
 * WebUI owns the HTTP surface and API layer.
 *
 * It starts the access point, mounts the filesystem, serves dashboard assets,
 * and exposes the script management endpoints. It also wires the ScriptManager
 * into the SceneManager so learned triggers can resolve scenes when needed.
 */
class WebUI {
public:
    /**
     * Creates the web UI with direct references to the hardware and scene subsystems.
     */
    explicit WebUI(HardwareManager& hardware, SceneManager& sceneManager);

    /**
     * Starts the web server and initializes the script runtime needed by the API.
     */
    void begin();

    /**
     * Starts the access point and configures the route table.
     */
    void beginAP();

    /**
     * Legacy loop hook kept for compatibility; runtime traffic is event-driven
     * and most work happens in callbacks and route handlers.
     */
    void loop();

    /**
     * Updates the dashboard heartbeat so the front-end can show the device as alive.
     */
    void setHeartbeatState(bool state) { heartbeatState = state; }

    /**
     * Accessor for the scene manager used by the UI and learned-trigger flow.
     */
    SceneManager& getSceneManager() { return sceneManagerRef; }

    /**
     * Authentication configuration (called by Core at startup).
     */
    void setAuthentication(bool enabled, const String& password = "");
    void setAPPassword(const String& password);

    /**
     * Set a fixed password instead of generating a random one.
     */
    void setFixedPassword(const String& password);

    /**
     * Save a script from a JSON payload: update it when the payload carries
     * an existing script id, otherwise add it as a new script.
     */
    bool saveOrUpdateScript(const String& scriptJson);

private:
    AsyncWebServer server;
    ScriptManager scriptManager;
    SceneManager& sceneManagerRef;
    volatile bool heartbeatState = false;

    // Authentication settings
    bool authenticationEnabled = true;
    String adminPassword = "defaultadmin";
    String apPassword = "password123";
    std::vector<String> authenticatedClients;

    /**
     * Registers the HTTP routes for dashboard pages and API endpoints.
     */
    void setupRoutes();

    /**
     * Authentication methods
     */
    bool isClientAuthenticated(AsyncWebServerRequest* request);
    void authenticateClient(AsyncWebServerRequest* request);
};
