#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include <map>

#include "../ScriptManager/ScriptManager.h"
#include "../SceneManager/SceneManager.h"

class WebUI
{
public:
    WebUI(ScriptManager &scriptManager, SceneManager &sceneManager);

    void begin();
    void beginAP();
    void loop();

    void setHeartbeatState(bool state);

    SceneManager &getSceneManager();
    ScriptManager &getScriptManager();

    void setAuthentication(bool enabled, const String &password = "");
    void setAPPassword(const String &password);
    void setFixedPassword(const String &password);

    bool saveOrUpdateScript(const String &scriptJson);

private:
    AsyncWebServer server;

    ScriptManager &scriptManagerRef;
    SceneManager &sceneManagerRef;

    volatile bool heartbeatState = false;

    bool authenticationEnabled = true;

    String adminPassword = "defaultadmin";
    String apPassword = "password123";

    std::vector<String> authenticatedClients;

    // Raw POST bodies received by ESPAsyncWebServer.
    //
    // The key is the request pointer because multiple requests can
    // potentially be in progress at the same time.
    std::map<AsyncWebServerRequest *, String> requestBodies;

    void setupRoutes();

    bool isClientAuthenticated(AsyncWebServerRequest *request);
    void authenticateClient(AsyncWebServerRequest *request);

    // Receives raw HTTP POST body chunks.
    void handleRequestBody(
        AsyncWebServerRequest *request,
        uint8_t *data,
        size_t len,
        size_t index,
        size_t total
    );

    // Gets the POST body.
    //
    // Supports:
    //   1. form field named "body"
    //   2. raw application/json body
    //   3. request->arg("plain") fallback
    bool getPostBody(
        AsyncWebServerRequest *request,
        String &body
    );
};