#include "WebUI.h"
#include "Debug.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <algorithm>

WebUI::WebUI(
    ScriptManager &scriptManager,
    SceneManager &sceneManager
)
    : server(80),
      scriptManagerRef(scriptManager),
      sceneManagerRef(sceneManager)
{
}

// ============================================================
// BEGIN
// ============================================================

void WebUI::begin()
{
    Debug::println(
        2,
        "[WEBUI][INIT] Starting WebUI initialization"
    );
    
    beginAP();

    /*
     * Managers are initialized by Core.
     *
     * WebUI deliberately does not call ScriptManager::begin()
     * or SceneManager::begin().
     *
     * Core owns the initialization order.
     */
    
    Debug::println(
        2,
        "[WEBUI][INIT] WebUI initialization complete"
    );
}

// ============================================================
// ACCESS POINT
// ============================================================

void WebUI::beginAP()
{
    Debug::println(
        2,
        "[WEBUI][INIT] Initializing WebUI in Access Point mode"
    );

    WiFi.mode(WIFI_AP);

    IPAddress localIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(
        localIP,
        gateway,
        subnet
    );

    if (!WiFi.softAP(
        "HomeController",
        apPassword.c_str()
    ))
    {
        Debug::println(
            1,
            "[WEBUI][ERROR] Failed to start Access Point with password: " + apPassword
        );

        return;
    }

    Debug::println(
        3,
        "[WEBUI][INIT] Access Point started with SSID=HomeController"
    );

    Debug::println(
        3,
        "[WEBUI][INIT] Access Point IP: " +
        WiFi.softAPIP().toString()
    );

    // if (!LittleFS.begin())
    // {
    //     Debug::println(
    //         1,
    //         "[WEBUI][ERROR] LittleFS mount failed - filesystem may be corrupted or missing"
    //     );

    //     return;
    // }
    
    // Debug::println(
    //     3,
    //     "[WEBUI][INIT] LittleFS mounted successfully"
    // );

    setupRoutes();

    server.begin();

    Debug::println(
        3,
        "[WEBUI][INIT] Web server started on port 80"
    );
}

// ============================================================
// POST BODY HELPER
// ============================================================

bool WebUI::getPostBody(AsyncWebServerRequest *request, String &body)
{
    if (request->hasParam("body", true)) {
        body = request->getParam("body", true)->value();
    }
    else if (request->_tempObject) {
        body = String((char *)request->_tempObject);
    }
    else {
        body = request->arg("plain");
    }

    if (body.length() > 0) {
        Debug::println(3, "[WEBUI][POST] Parsed body: " + body);
        return true;
    }

    Debug::println(1, "[WEBUI][ERROR] Empty POST body");
    return false;
}

// ============================================================
// capturePostBody
// ============================================================

static void capturePostBody(
    AsyncWebServerRequest *request,
    uint8_t *data,
    size_t len,
    size_t index,
    size_t total)
{
    if (index == 0) {
        char *body = (char *)malloc(total + 1);
        if (!body) {
            Debug::println(1, "[WEBUI][ERROR] Failed to allocate POST body");
            return;
        }

        request->_tempObject = body;
        body[0] = '\0';

        Debug::println(3, "[WEBUI][POST] Receiving body, size=" + String(total));
    }

    if (request->_tempObject) {
        char *body = (char *)request->_tempObject;
        memcpy(body + index, data, len);
        body[index + len] = '\0';

        if (index + len == total) {
            Debug::println(3, "[WEBUI][POST] Body received: " + String(body));
        }
    }
}

// ============================================================
// ROUTES
// ============================================================

void WebUI::setupRoutes()
{
    // --------------------------------------------------------
    // DASHBOARD
    // --------------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        [this](AsyncWebServerRequest *request)
        {
            if (!isClientAuthenticated(request))
            {
                request->send(
                    401,
                    "text/html",
                    "<html><body>"
                    "<h1>Authentication Required</h1>"
                    "<form method='POST' action='/login'>"
                    "<input type='password' name='password'>"
                    "<input type='submit' value='Login'>"
                    "</form>"
                    "</body></html>"
                );

                return;
            }

            authenticateClient(request);

            request->send(
                LittleFS,
                "/index.html",
                "text/html"
            );
        }
    );

    // --------------------------------------------------------
    // SCRIPT EDITOR
    // --------------------------------------------------------

    server.on(
        "/action_scripts.html",
        HTTP_GET,
        [this](AsyncWebServerRequest *request)
        {
            if (!isClientAuthenticated(request))
            {
                request->send(
                    401,
                    "text/html",
                    "<html><body>"
                    "<h1>Authentication Required</h1>"
                    "</body></html>"
                );

                return;
            }

            authenticateClient(request);

            if (!LittleFS.exists("/action_scripts.html"))
            {
                request->send(
                    404,
                    "text/plain",
                    "action_scripts.html missing from filesystem"
                );

                return;
            }

            request->send(
                LittleFS,
                "/action_scripts.html",
                "text/html"
            );
        }
    );

    // --------------------------------------------------------
    // AUDIO TEST PAGE
    // --------------------------------------------------------

    server.on(
        "/audio_test.html",
        HTTP_GET,
        [this](AsyncWebServerRequest *request)
        {
            if (!isClientAuthenticated(request))
            {
                request->send(
                    401,
                    "text/html",
                    "<html><body>"
                    "<h1>Authentication Required</h1>"
                    "</body></html>"
                );

                return;
            }

            authenticateClient(request);

            if (!LittleFS.exists("/audio_test.html"))
            {
                request->send(
                    404,
                    "text/plain",
                    "audio_test.html missing from filesystem"
                );

                return;
            }

            request->send(
                LittleFS,
                "/audio_test.html",
                "text/html"
            );
        }
    );

    // --------------------------------------------------------
    // LOGIN
    // --------------------------------------------------------

    server.on(
        "/login",
        HTTP_POST,
        [this](AsyncWebServerRequest *request)
        {
            String password;

            if (request->hasParam("password", true))
            {
                password =
                    request->getParam(
                        "password",
                        true
                    )->value();
            }
            else
            {
                password =
                    request->arg("password");
            }

            if (password == adminPassword)
            {
                authenticateClient(request);
                request->redirect("/");

                return;
            }

            Debug::println(
                2,
                "[WEBUI][LOGIN] Failed login attempt from " +
                request->client()->remoteIP().toString()
            );

            request->send(
                401,
                "text/html",
                "<html><body>"
                "<h1>Authentication Failed</h1>"
                "<form method='POST' action='/login'>"
                "<input type='password' name='password'>"
                "<input type='submit' value='Login'>"
                "</form>"
                "</body></html>"
            );
        }
    );

    // --------------------------------------------------------
    // AUTH WRAPPER
    // --------------------------------------------------------

    auto withAuth =
        [this](
            std::function<void(AsyncWebServerRequest *)> handler
        )
        {
            return [this, handler](
                AsyncWebServerRequest *request
            )
            {
                if (!isClientAuthenticated(request))
                {
                    request->send(
                        401,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Authentication required\"}"
                    );

                    return;
                }

                authenticateClient(request);

                handler(request);
            };
        };

    // ========================================================
    // TIME
    // ========================================================

    server.on(
        "/api/set-time",
        HTTP_POST,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                String body;

                if (!getPostBody(request, body))
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Missing time payload\"}"
                    );

                    return;
                }

                DynamicJsonDocument doc(512);

                DeserializationError err =
                    deserializeJson(doc, body);

                if (
                    err ||
                    !doc["time"].is<unsigned long>()
                )
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Invalid time payload\"}"
                    );

                    return;
                }

                unsigned long unixTime =
                    doc["time"].as<unsigned long>();

                int timezoneOffsetMinutes =
                    doc["timezoneOffsetMinutes"] | 0;

                Debug::setTimezoneOffsetMinutes(
                    timezoneOffsetMinutes
                );

                Debug::setDeviceTime(unixTime);

                request->send(
                    200,
                    "application/json",
                    "{\"success\":true}"
                );
            }
        )
    );

    // ========================================================
    // STATUS
    // ========================================================

    server.on(
        "/api/status",
        HTTP_GET,
        [this](AsyncWebServerRequest *request)
        {
            DynamicJsonDocument doc(512);

            int core0Busy =
                100 -
                (int)ulTaskGetIdleRunTimePercentForCore(0);

            int core1Busy =
                100 -
                (int)ulTaskGetIdleRunTimePercentForCore(1);

            doc["uptimeMs"] = millis();
            doc["freeHeap"] = ESP.getFreeHeap();
            doc["cpuLoadPercent"] =
                (core0Busy + core1Busy) / 2;

            doc["ssid"] = "HomeController";
            doc["ip"] =
                WiFi.softAPIP().toString();

            doc["mode"] = "AP";

            doc["timestampMode"] =
                (Debug::getTimestamp().length() > 0)
                    ? "wall-clock"
                    : "uptime";

            doc["heartbeat"] =
                heartbeatState;

            String json;

            serializeJson(
                doc,
                json
            );

            request->send(
                200,
                "application/json",
                json
            );
        }
    );

    // ========================================================
    // LOGS
    // ========================================================

    server.on(
        "/api/logs",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            request->send(
                200,
                "text/html",
                Debug::getWebLogs()
            );
        }
    );

    server.on(
        "/api/logs/clear",
        HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            Debug::println(
                3,
                "[WEBUI][POST /api/logs/clear] clearing log buffer"
            );

            Debug::clearLogs();

            request->send(
                200,
                "application/json",
                "{\"success\":true}"
            );
        }
    );

    // ========================================================
    // SCRIPTS - LIST
    // ========================================================

    server.on(
        "/api/action-scripts",
        HTTP_GET,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                String json =
                    scriptManagerRef.getScriptsAsJson();

                request->send(
                    200,
                    "application/json",
                    json
                );
            }
        )
    );

    // ========================================================
    // SCRIPTS - ADD / UPDATE
    // ========================================================

server.on("/api/action-scripts", HTTP_POST,
    withAuth([this](AsyncWebServerRequest *request) {
        String body;

        if (!getPostBody(request, body)) {
            request->send(400, "application/json",
                          "{\"success\":false,\"message\":\"Missing body\"}");
            return;
        }

        Debug::println(2, "[WEBUI][SCRIPTS] Save request received");

        if (saveOrUpdateScript(body)) {
            Debug::println(2, "[WEBUI][SCRIPTS] Save successful");
            request->send(200, "application/json",
                          "{\"success\":true}");
        } else {
            Debug::println(1, "[WEBUI][SCRIPTS][ERROR] Save failed");
            request->send(500, "application/json",
                          "{\"success\":false,\"message\":\"Save failed\"}");
        }
    }),
    nullptr,
    capturePostBody);

    // ========================================================
    // SCRIPTS - DELETE
    // ========================================================

    server.on(
        "/api/action-scripts",
        HTTP_DELETE,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                if (!request->hasParam("id"))
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Missing id\"}"
                    );

                    return;
                }

                int scriptId =
                    request->getParam(
                        "id"
                    )->value().toInt();

                bool success =
                    scriptManagerRef.deleteScript(
                        scriptId
                    );

                request->send(
                    success ? 200 : 404,
                    "application/json",
                    success
                        ? "{\"success\":true}"
                        : "{\"success\":false,"
                          "\"message\":\"Script not found\"}"
                );
            }
        )
    );

    // ========================================================
    // SCRIPTS - APPLY
    //
    // Kept as an alias for the existing UI.
    // There is no separate "apply to hardware" concept anymore.
    // Saving the script is enough.
    // ========================================================

    server.on(
        "/api/action-scripts/apply",
        HTTP_POST,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                String body;

                if (!getPostBody(request, body))
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Missing body\"}"
                    );

                    return;
                }

                if (!saveOrUpdateScript(body))
                {
                    request->send(
                        500,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Failed to save script\"}"
                    );

                    return;
                }

                request->send(
                    200,
                    "application/json",
                    "{\"success\":true}"
                );
            }
        ),
        nullptr,
        capturePostBody);

    // ========================================================
    // EXECUTE SCRIPT
    //
    // This is a macro invocation.
    //
    // ScriptManager -> CommandSink -> Core -> HardwareManager
    //
    // WebUI never sees the hardware.
    // ========================================================

    server.on(
        "/api/execute-script",
        HTTP_POST,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                String body;

                if (!getPostBody(request, body))
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Missing body\"}"
                    );

                    return;
                }

                DynamicJsonDocument doc(512);

                if (
                    deserializeJson(doc, body) !=
                    DeserializationError::Ok
                )
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Invalid JSON\"}"
                    );

                    return;
                }

                bool success = false;

                if (doc["id"].is<int>())
                {
                    success =
                        scriptManagerRef.runScript(
                            doc["id"].as<int>()
                        );
                }
                else if (doc["name"].is<String>())
                {
                    success =
                        scriptManagerRef.runScript(
                            doc["name"].as<String>()
                        );
                }
                else if (doc["script"].is<String>())
                {
                    success =
                        scriptManagerRef.runScript(
                            doc["script"].as<String>()
                        );
                }

                request->send(
                    success ? 200 : 404,
                    "application/json",
                    success
                        ? "{\"success\":true}"
                        : "{\"success\":false,"
                          "\"message\":\"Script not found or failed\"}"
                );
            }
        ),
        nullptr,
        capturePostBody);

    // ========================================================
    // SCENES - LIST
    // ========================================================

    server.on(
        "/api/scenes",
        HTTP_GET,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                String json =
                    sceneManagerRef.getScenesAsJson();

                request->send(
                    200,
                    "application/json",
                    json
                );
            }
        )
    );

    // ========================================================
    // ACTIVATE SCENE
    //
    // WebUI is one of the two allowed scene triggers:
    //
    // WebUI -> SceneManager -> ScriptManager -> Core
    //
    // ========================================================

    server.on(
        "/api/scenes/apply",
        HTTP_POST,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                String body;

                if (!getPostBody(request, body))
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Missing body\"}"
                    );

                    return;
                }

                DynamicJsonDocument doc(512);

                if (
                    deserializeJson(doc, body) !=
                    DeserializationError::Ok
                )
                {
                    request->send(
                        400,
                        "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Invalid JSON\"}"
                    );

                    return;
                }

                bool success = false;

                if (doc["id"].is<int>())
                {
                    success =
                        sceneManagerRef.activateScene(
                            doc["id"].as<int>()
                        );
                }
                else if (doc["name"].is<String>())
                {
                    success =
                        sceneManagerRef.activateScene(
                            doc["name"].as<String>()
                        );
                }
                else if (doc["scene"].is<String>())
                {
                    success =
                        sceneManagerRef.activateScene(
                            doc["scene"].as<String>()
                        );
                }

                request->send(
                    success ? 200 : 404,
                    "application/json",
                    success
                        ? "{\"success\":true}"
                        : "{\"success\":false,"
                          "\"message\":\"Scene not found or failed\"}"
                );
            }
        )
    );

    // ========================================================
    // SCENE EXPORT
    // ========================================================

    server.on(
        "/api/export-scenes",
        HTTP_GET,
        withAuth(
            [this](AsyncWebServerRequest *request)
            {
                String json =
                    sceneManagerRef.getScenesAsJson();

                request->send(
                    200,
                    "application/json",
                    json
                );
            }
        )
    );

    // ========================================================
    // EFFECT
    //
    // Temporary endpoint.
    //
    // Once EffectManager exists, this should become:
    //
    // WebUI -> Core -> EffectManager
    //
    // It must NOT become:
    //
    // WebUI -> HardwareManager
    // ========================================================

    server.on(
        "/api/effect",
        HTTP_POST,
        withAuth(
            [](AsyncWebServerRequest *request)
            {
                request->send(
                    501,
                    "application/json",
                    "{\"success\":false,"
                    "\"message\":\"Effects are not implemented yet\"}"
                );
            }
        )
    );

    // ========================================================
    // REBOOT
    // ========================================================

    server.on(
        "/api/reboot",
        HTTP_POST,
        withAuth(
            [](AsyncWebServerRequest *request)
            {
                request->send(
                    501,
                    "application/json",
                    "{\"success\":false,"
                    "\"message\":\"Reboot is not implemented\"}"
                );
            }
        )
    );

    // ========================================================
    // LOG SAVE
    // ========================================================

    server.on(
        "/api/logs/save",
        HTTP_POST,
        withAuth(
            [](AsyncWebServerRequest *request)
            {
                request->send(
                    501,
                    "application/json",
                    "{\"success\":false,"
                    "\"message\":\"Persistent log saving is not implemented\"}"
                );
            }
        )
    );

    // ========================================================
    // NOT FOUND
    // ========================================================

    server.onNotFound(
        [](AsyncWebServerRequest *request)
        {
            String clientIP = request->client()->remoteIP().toString();
            String path = request->url();
            
            Debug::println(
                2,
                "[WEBUI][NOTFOUND] Client: " + clientIP + " requested path: " + path
            );
            
            request->send(
                404,
                "text/plain",
                "Not found"
            );
        }
    );
}

// ============================================================
// LOOP
// ============================================================

void WebUI::loop()
{
    /*
     * AsyncWebServer does not need a polling loop.
     *
     * Kept for compatibility with App/Core.
     */
}

// ============================================================
// HEARTBEAT
// ============================================================

void WebUI::setHeartbeatState(bool state)
{
    heartbeatState = state;
}

// ============================================================
// AUTHENTICATION
// ============================================================

void WebUI::setAuthentication(
    bool enabled,
    const String &password
)
{
    bool wasEnabled = authenticationEnabled;
    authenticationEnabled = enabled;

    if (password.length() > 0)
    {
        adminPassword = password;
        Debug::println(
            3,
            "[WEBUI][AUTH] Admin password updated"
        );
    }

    Debug::println(
        2,
        "[WEBUI][AUTH] Authentication " +
        String(enabled ? "enabled" : "disabled") +
        (wasEnabled != enabled ? " (status changed)" : " (status unchanged)")
    );
}

void WebUI::setAPPassword(const String &password)
{
    apPassword = password;

    if (WiFi.getMode() & WIFI_AP)
    {
        bool success = WiFi.softAP(
            "HomeController",
            apPassword.c_str()
        );
        
        Debug::println(
            2,
            "[WEBUI][AUTH] AP password " + 
            String(success ? "updated successfully" : "update failed")
        );
    }
}

void WebUI::setFixedPassword(const String &password)
{
    adminPassword = password;

    Debug::println(
        2,
        "[WEBUI][AUTH] Fixed password updated"
    );
}

// ============================================================
// SCRIPT SAVE / UPDATE
// ============================================================

bool WebUI::saveOrUpdateScript(
    const String &scriptJson
)
{
    DynamicJsonDocument doc(8192);

    if (
        deserializeJson(doc, scriptJson) !=
        DeserializationError::Ok
    )
    {
        Debug::println(
            1,
            "[WEBUI][SCRIPTS] Invalid JSON: " + scriptJson.substring(0, min((unsigned int)scriptJson.length(), (unsigned int)100)) + 
            (scriptJson.length() > 100 ? "..." : "")
        );

        return false;
    }

    /*
     * Accept both:
     *
     * {
     * "script": "{...}"
     * }
     *
     * and:
     *
     * {
     * "id": 1,
     * "name": "...",
     * ...
     * }
     *
     * This keeps the existing editor working while the
     * frontend is being simplified.
     */

    String script;

    if (doc["script"].is<String>())
    {
        script =
            doc["script"].as<String>();
    }
    else if (doc.is<JsonObject>())
    {
        serializeJson(
            doc,
            script
        );
    }
    else
    {
        return false;
    }

    DynamicJsonDocument scriptDoc(8192);

    if (
        deserializeJson(scriptDoc, script) !=
        DeserializationError::Ok
    )
    {
        return false;
    }

    int id =
        scriptDoc["id"] | 0;

    if (
        id > 0 &&
        scriptManagerRef.getScriptById(id).length() > 0
    )
    {
        return scriptManagerRef.updateScript(
            id,
            script
        );
    }

    return scriptManagerRef.addScript(
        script
    );
}

// ============================================================
// AUTH CLIENT TRACKING
// ============================================================

bool WebUI::isClientAuthenticated(
    AsyncWebServerRequest *request
)
{
    if (!authenticationEnabled)
    {
        return true;
    }

    String clientIP =
        request->client()->remoteIP().toString();

    return std::find(
        authenticatedClients.begin(),
        authenticatedClients.end(),
        clientIP
    ) != authenticatedClients.end();
}

void WebUI::authenticateClient(
    AsyncWebServerRequest *request
)
{
    if (!authenticationEnabled)
    {
        return;
    }

    String clientIP =
        request->client()->remoteIP().toString();

    if (
        std::find(
            authenticatedClients.begin(),
            authenticatedClients.end(),
            clientIP
        ) == authenticatedClients.end()
    )
    {
        authenticatedClients.push_back(
            clientIP
        );

        Debug::println(
            2,
            "[WEBUI][AUTH] Client authenticated: " +
            clientIP + " (total authenticated clients: " + String(authenticatedClients.size()) + ")"
        );
    }
    else
    {
        Debug::println(
            3,
            "[WEBUI][AUTH] Client already authenticated: " +
            clientIP
        );
    }
}

