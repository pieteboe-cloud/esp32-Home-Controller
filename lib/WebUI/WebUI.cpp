#include "WebUI.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

WebUI::WebUI(HardwareManager &hw, SceneManager &sceneManager)
    : server(80), scriptManager(hw), sceneManagerRef(sceneManager) {}

// Initialize managers after the network/filesystem are available. ScriptManager
// receives the scene manager reference so learned triggers can resolve scenes.
void WebUI::begin()
{
    beginAP();
    scriptManager.begin();
    // Wire scene manager into script manager for Kaku→Scene lookup
    scriptManager.setSceneManager(&sceneManagerRef);
}

void WebUI::beginAP()
{
    Debug::println(2, "[WEBUI][INIT] Initializing WebUI in Access Point mode");
    WiFi.mode(WIFI_AP);
    IPAddress localIP(192, 168, 4, 1), gateway(192, 168, 4, 1), subnet(255, 255, 255, 0);
    WiFi.softAPConfig(localIP, gateway, subnet);
    if (!WiFi.softAP("HomeController", apPassword.c_str()))
    {
        Debug::println(1, "[WEBUI][ERROR] Failed to start Access Point");
        return;
    }
    Debug::println(3, "[WEBUI][INIT] Access Point started with SSID=HomeController");
    Debug::println(3, "[WEBUI][INIT] Access Point IP: " + WiFi.softAPIP().toString());
    if (!LittleFS.begin())
    {
        Debug::println(1, "[WEBUI][ERROR] LittleFS mount failed");
        return;
    }
    setupRoutes();
    server.begin();
    Debug::println(3, "[WEBUI][INIT] Web server started on port 80");
}

static bool getPostBody(AsyncWebServerRequest *request, String &body)
{
    // The browser sends form-encoded `body`, while direct API clients may send
    // a raw request body. Accept both formats to keep the API easy to inspect.
    if (request->hasParam("body", true))
    {
        body = request->getParam("body", true)->value();
    }
    else
    {
        body = request->arg("plain");
    }
    return body.length() > 0;
}

void WebUI::setupRoutes()
{
    // Dashboard pages: these requests should stay quiet after the page is served;
    // API activity below is what matters when diagnosing a user interaction.
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        Debug::println(3, "[WEBUI][GET /] serving dashboard");
        if (!isClientAuthenticated(request)) {
            request->send(401, "text/html", "<html><body><h1>Authentication Required</h1><form method='POST' action='/login'><input type='password' name='password'><input type='submit' value='Login'></form></body></html>");
            return;
        }
        authenticateClient(request);
        request->send(LittleFS, "/index.html", "text/html"); });

    // Static page for the script editor (linked from the dashboard).
    server.on("/action_scripts.html", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!isClientAuthenticated(request)) {
            request->send(401, "text/html", "<html><body><h1>Authentication Required</h1></body></html>");
            return;
        }
        authenticateClient(request);
        if (!LittleFS.exists("/action_scripts.html")) {
            request->send(404, "text/plain", "action_scripts.html missing from filesystem");
            return;
        }
        request->send(LittleFS, "/action_scripts.html", "text/html"); });

    server.on("/login", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        String password;
        if (request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        } else {
            password = request->arg("password");
        }

        if (password == adminPassword) {
            authenticateClient(request);
            request->send(302, "text/plain", "Login successful");
            request->redirect("/");
        } else {
            Debug::println(2, "[WEBUI][LOGIN] Failed login attempt from " + request->client()->remoteIP().toString());
            request->send(401, "text/html", "<html><body><h1>Authentication Failed</h1><form method='POST' action='/login'><input type='password' name='password'><input type='submit' value='Login'></form></body></html>");
        } });

    // Authentication guard applied to API routes via withAuth()
    auto withAuth = [this](std::function<void(AsyncWebServerRequest *)> handler)
    {
        return [this, handler](AsyncWebServerRequest *request)
        {
            if (!isClientAuthenticated(request))
            {
                request->send(401, "application/json", "{\"success\":false,\"message\":\"Authentication required\"}");
                return;
            }
            authenticateClient(request);
            handler(request);
        };
    };

    server.on("/api/set-time", HTTP_POST, withAuth([this](AsyncWebServerRequest *request)
                                                   {
        Debug::println(3, "[WEBUI][POST /api/set-time] request received");
        String body;
        if (request->hasParam("body", true)) {
            body = request->getParam("body", true)->value();
        } else {
            body = request->arg("plain");
        }

        if (body.length() == 0) {
            Debug::println(1, "[WEBUI][POST /api/set-time] missing payload");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing time payload\"}");
            return;
        }

        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err || !doc["time"].is<unsigned long>()) {
            Debug::println(1, "[WEBUI][POST /api/set-time] invalid time payload");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid time payload\"}");
            return;
        }

        unsigned long unixTime = doc["time"].as<unsigned long>();
        int timezoneOffsetMinutes = doc["timezoneOffsetMinutes"] | 0;
        Debug::setTimezoneOffsetMinutes(timezoneOffsetMinutes);
        Debug::setDeviceTime(unixTime);
        Debug::println(3, "[WEBUI][POST /api/set-time] clock synchronized with browser timezone offset " + String(timezoneOffsetMinutes) + " minutes");
        request->send(200, "application/json", "{\"success\":true}"); }));

    // Audio test page
    server.on("/audio_test.html", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!isClientAuthenticated(request)) {
            request->send(401, "text/html", "<html><body><h1>Authentication Required</h1></body></html>");
            return;
        }
        authenticateClient(request);
        if (!LittleFS.exists("/audio_test.html")) {
            request->send(404, "text/plain", "audio_test.html missing from filesystem");
            return;
        }
        request->send(LittleFS, "/audio_test.html", "text/html");
    });

    server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", Debug::getWebLogs()); });

    // Log maintenance is deliberately logged before clearing because clearing
    // the buffer removes the diagnostic line that would otherwise describe it.
    server.on("/api/logs/clear", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        Debug::println(3, "[WEBUI][POST /api/logs/clear] clearing web log buffer");
        Debug::clearLogs();
        request->send(200, "application/json", "{\"success\":true}"); });

    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        // Status is polled twice per second, so it provides data only and does
        // not create a log line for every heartbeat request.
        DynamicJsonDocument doc(512);
        // FreeRTOS tracks idle time for each core. Invert and average both idle
        // percentages to show total processor activity without adding a task.
        int core0Busy = 100 - (int)ulTaskGetIdleRunTimePercentForCore(0);
        int core1Busy = 100 - (int)ulTaskGetIdleRunTimePercentForCore(1);
        doc["uptimeMs"] = millis();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["cpuLoadPercent"] = (core0Busy + core1Busy) / 2;
        doc["ssid"] = "HomeController";
        doc["ip"] = WiFi.softAPIP().toString();
        doc["mode"] = "AP";
        doc["timestampMode"] = (Debug::getTimestamp().length() > 0) ? "wall-clock" : "uptime";
        doc["heartbeat"] = heartbeatState;
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json); });

    // These controls are present in the dashboard, but no effect engine,
    // reboot policy, or persistent log export exists in this firmware yet.
    // Return explicit errors and record them instead of allowing silent 404s.
    server.on("/api/effect", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        Debug::println(2, "[WEBUI][POST /api/effect] effect request rejected: no effect engine is configured");
        request->send(501, "application/json", "{\"success\":false,\"message\":\"Effects are not implemented in this build\"}"); });

    server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        Debug::println(2, "[WEBUI][POST /api/reboot] reboot request rejected: reboot endpoint is not implemented");
        request->send(501, "application/json", "{\"success\":false,\"message\":\"Reboot is not implemented in this build\"}"); });

    server.on("/api/logs/save", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        Debug::println(2, "[WEBUI][POST /api/logs/save] save request rejected: logs are RAM-only");
        request->send(501, "application/json", "{\"success\":false,\"message\":\"Persistent log saving is not implemented\"}"); });

    // Action-script CRUD and execution routes. Log payload sizes and outcomes,
    // but leave detailed action semantics to ScriptManager.
    server.on("/api/action-scripts", HTTP_GET, withAuth([this](AsyncWebServerRequest *request)
                                                        {
        String json = scriptManager.getScriptsAsJson();
        if (DEBUG_LEVEL >= 3) {
            Debug::println(4, "[WEBUI][GET /api/action-scripts] returning " + String(json.length()) + " bytes");
        }
        request->send(200, "application/json", json); }));

    server.on("/api/action-scripts/raw", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        String raw = scriptManager.getRawScriptsFile();
        Debug::println(4, "[WEBUI][ACTION-SCRIPTS][RAW] returning " + String(raw.length()) + " bytes");
        if (raw.length() == 0) {
            Debug::println(1, "[WEBUI][ACTION-SCRIPTS][RAW] scripts file unavailable");
            request->send(404, "text/plain", "Scripts file is empty or unavailable.");
            return;
        }
        request->send(200, "application/json", raw); });

    server.on("/api/action-scripts", HTTP_POST, withAuth([this](AsyncWebServerRequest *request)
                                                         {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] missing body param");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Send form field 'body' containing JSON\"}");
            return;
        }
        Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] payload length=" + String(body.length()) + " body=" + body);
        DynamicJsonDocument doc(16384);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            Debug::println(1, "[WEBUI][ACTION-SCRIPTS][POST] invalid JSON");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON payload\"}");
            return;
        }

            String script;
            if (doc["script"].is<String>()) {
                // Wrapped format: {"script": "<json>"}
                script = doc["script"].as<String>();
            } else if (doc.is<JsonObject>()) {
                // Direct format: the script object itself (used by action_scripts.html)
                serializeJson(doc, script);
            } else {
                Debug::println(1, "[WEBUI][ACTION-SCRIPTS][POST] missing script field");
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing 'script' field\"}");
                return;
            }
            bool success = saveOrUpdateScript(script);
            if (success) {
                Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] scripts updated");
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                Debug::println(1, "[WEBUI][ACTION-SCRIPTS][POST] scripts update failed");
                request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to update scripts\"}");
            } }));

    server.on("/api/action-scripts", HTTP_DELETE, withAuth([this](AsyncWebServerRequest *request)
                                                           {
            if (!request->hasParam("id")) {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing id\"}");
                return;
            }
            int scriptId = request->getParam("id")->value().toInt();
            bool success = scriptManager.deleteScript(scriptId);
            Debug::println(2, String("[WEBUI][ACTION-SCRIPTS][DELETE] id=") + scriptId + " -> " + (success ? "ok" : "not found"));
            request->send(success ? 200 : 404, "application/json",
                          success ? "{\"success\":true}" : "{\"success\":false,\"message\":\"Script not found\"}"); }));

    server.on("/api/action-scripts/apply", HTTP_POST, withAuth([this](AsyncWebServerRequest *request)
                                                               {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] missing body param");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Send form field 'body' containing JSON\"}");
            return;
        }
        Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] payload length=" + String(body.length()) + " body=" + body);
        DynamicJsonDocument doc(16384);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            Debug::println(1, "[WEBUI][ACTION-SCRIPTS][POST] invalid JSON");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON payload\"}");
            return;
        }

        if (!doc["script"].is<String>()) {
            Debug::println(1, "[WEBUI][ACTION-SCRIPTS][POST] missing script field");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing 'script' field\"}");
            return;
        }

        String script = doc["script"].as<String>();
        bool success = saveOrUpdateScript(script);
        if (success) {
            Debug::println(2, "[WEBUI][ACTION-SCRIPTS][APPLY] scripts updated and applied");
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            Debug::println(1, "[WEBUI][ACTION-SCRIPTS][APPLY] scripts update failed");
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to update scripts\"}");
        } }));

    server.on("/api/execute-script", HTTP_POST, withAuth([this](AsyncWebServerRequest *request)
                                                         {
        String body;
        if (!getPostBody(request, body)) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
            return;
        }
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body) != DeserializationError::Ok || !doc["id"].is<int>()) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing 'id' field\"}");
            return;
        }
        int scriptId = doc["id"].as<int>();
        bool success = scriptManager.executeScript(scriptId);
        Debug::println(2, String("[WEBUI][EXECUTE-SCRIPT] id=") + scriptId + " -> " + (success ? "ok" : "failed"));
        request->send(success ? 200 : 404, "application/json",
                      success ? "{\"success\":true}" : "{\"success\":false,\"message\":\"Script not found\"}"); }));

    // Test a single action (used by the editor's Test/Preview buttons).
    server.on("/api/test-action", HTTP_POST, withAuth([this](AsyncWebServerRequest *request)
                                                      {
        String body;
        if (!getPostBody(request, body)) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
            return;
        }
        bool success = scriptManager.executeAction(body);
        Debug::println(2, String("[WEBUI][TEST-ACTION] ") + (success ? "ok" : "failed"));
        request->send(success ? 200 : 400, "application/json",
                      success ? "{\"success\":true}" : "{\"success\":false,\"message\":\"Action execution failed\"}"); }));

    // IR/RF trigger learning. POST starts capture (?filter=IR|RF|KAKU|ANY),
    // GET polls the capture status until an event has been captured.
    server.on("/api/learn", HTTP_POST, withAuth([this](AsyncWebServerRequest *request)
                                                {
        String filter = request->hasParam("filter") ? request->getParam("filter")->value() : String("ANY");
        scriptManager.startCapture(filter);
        Debug::println(2, "[WEBUI][LEARN] capture started, filter=" + filter);
        request->send(200, "application/json", "{\"success\":true,\"waiting\":true}"); }));

    server.on("/api/learn", HTTP_GET, withAuth([this](AsyncWebServerRequest *request)
                                               {
        String status = scriptManager.getCaptureStatus();
        request->send(200, "application/json", status); }));

    server.on("/api/scenes", HTTP_GET, withAuth([this](AsyncWebServerRequest *request)
                                                {
        String json = sceneManagerRef.getScenesAsJson();
        if (DEBUG_LEVEL >= 3) {
            Debug::println(4, "[WEBUI][GET /api/scenes] returning " + String(json.length()) + " bytes");
        }
        request->send(200, "application/json", json); }));

    server.on("/api/scenes/apply", HTTP_POST, withAuth([this](AsyncWebServerRequest *request)
                                                       {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(2, "[WEBUI][SCENES][POST] missing body param");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Send form field 'body' containing JSON\"}");
            return;
        }
        Debug::println(2, "[WEBUI][SCENES][POST] payload length=" + String(body.length()) + " body=" + body);
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            Debug::println(1, "[WEBUI][SCENES][POST] invalid JSON");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON payload\"}");
            return;
        }

        if (!doc["scene"].is<String>()) {
            Debug::println(1, "[WEBUI][SCENES][POST] missing scene field");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing 'scene' field\"}");
            return;
        }

        int sceneId = doc["scene"].as<int>();
        bool success = sceneManagerRef.applyScene(sceneId);
        if (success) {
            Debug::println(2, "[WEBUI][SCENES][POST] scene applied: " + String(sceneId));
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            Debug::println(1, "[WEBUI][SCENES][POST] scene apply failed: " + String(sceneId));
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to apply scene\"}");
        } }));

    server.on("/api/export-scenes", HTTP_GET, withAuth([this](AsyncWebServerRequest *request)
                                                       {
        String json = sceneManagerRef.getScenesAsJson();
        if (DEBUG_LEVEL >= 3) {
            Debug::println(4, "[WEBUI][GET /api/export-scenes] returning " + String(json.length()) + " bytes");
        }
        request->send(200, "application/json", json); }));

    // Fallback route for any unhandled path that doesn't match above.
    // This prevents the server from returning 404 for valid API endpoints
    // that are not explicitly configured above.
    server.onNotFound([this](AsyncWebServerRequest *request)
                      {
        Debug::println(2, "[WEBUI][404] path not found: " + request->url());
        request->send(404, "text/plain", "Not found"); });
}

// Legacy loop hook kept for compatibility; runtime traffic is event-driven
// and most work happens in callbacks and route handlers.
void WebUI::loop()
{
    // The ESP32 async web server handles all networking internally.
    // This loop is kept only for compatibility with existing code.
}

void WebUI::setAuthentication(bool enabled, const String &password)
{
    authenticationEnabled = enabled;
    if (password.length() > 0)
    {
        adminPassword = password;
    }
    Debug::println(2, "[WEBUI][AUTH] Authentication " + String(enabled ? "enabled" : "disabled"));
}

void WebUI::setAPPassword(const String &password)
{
    apPassword = password;
    // If the AP is already running with an old password, restart it with the new one.
    if (WiFi.getMode() & WIFI_AP)
    {
        WiFi.softAP("HomeController", apPassword.c_str());
        Debug::println(2, "[WEBUI][AUTH] AP password updated and AP restarted");
    }
    else
    {
        Debug::println(2, "[WEBUI][AUTH] AP password updated");
    }
}

void WebUI::setFixedPassword(const String &password)
{
    adminPassword = password;
    Debug::println(2, "[WEBUI][AUTH] Fixed password set to: " + password);
}

bool WebUI::saveOrUpdateScript(const String &scriptJson)
{
    // Extract the id the payload claims (if any), then decide update vs add.
    // Doc must be generous: editor payloads run several hundred bytes with
    // nested arrays, and ArduinoJson needs headroom beyond the raw length.
    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, scriptJson) != DeserializationError::Ok)
    {
        Debug::println(1, "[WEBUI][SCRIPTS] payload is not valid JSON");
        return false;
    }
    int id = doc["id"] | 0;
    if (id > 0 && scriptManager.getScriptById(id).length() > 0)
    {
        return scriptManager.updateScript(id, scriptJson);
    }
    return scriptManager.addScript(scriptJson);
}

bool WebUI::isClientAuthenticated(AsyncWebServerRequest *request)
{
    if (!authenticationEnabled)
    {
        return true;
    }
    return std::find(authenticatedClients.begin(), authenticatedClients.end(), request->client()->remoteIP().toString()) != authenticatedClients.end();
}

void WebUI::authenticateClient(AsyncWebServerRequest *request)
{
    if (!authenticationEnabled)
    {
        return;
    }
    String clientIP = request->client()->remoteIP().toString();
    if (std::find(authenticatedClients.begin(), authenticatedClients.end(), clientIP) == authenticatedClients.end())
    {
        authenticatedClients.push_back(clientIP);
        Debug::println(2, "[WEBUI][AUTH] Client authenticated: " + clientIP);
    }
}
