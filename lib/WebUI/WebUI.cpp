#include "WebUI.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

WebUI::WebUI(HardwareManager& hw, SceneManager& sceneManager)
    : server(80), scriptManager(hw), sceneManagerRef(sceneManager) {}

// Initialize managers after the network/filesystem are available. ScriptManager
// receives the scene manager reference so learned triggers can resolve scenes.
void WebUI::begin() {
    beginAP();
    scriptManager.begin();
    // Wire scene manager into script manager for Kaku→Scene lookup
    scriptManager.setSceneManager(&sceneManagerRef);
}

void WebUI::beginAP() {
    Debug::println("[WEBUI] Initializing WebUI in Access Point mode...");
    WiFi.mode(WIFI_AP);
    IPAddress localIP(192,168,4,1), gateway(192,168,4,1), subnet(255,255,255,0);
    WiFi.softAPConfig(localIP, gateway, subnet);
    if (!WiFi.softAP("HomeController", "password123")) {
        Debug::println("[WEBUI][ERROR] Failed to start Access Point!");
        return;
    }
    Debug::println(3, "[WEBUI][beginAP] Access Point started with SSID=HomeController");
    Debug::println(3, "[WEBUI][beginAP] Access Point IP: " + WiFi.softAPIP().toString());
    if (!LittleFS.begin()) {
        Debug::println(1, "[WEBUI][ERROR] LittleFS mount failed!");
        return;
    }
    setupRoutes();
    server.begin();
    Debug::println(3, "[WEBUI][beginAP] Web server started on port 80");
}

static bool getPostBody(AsyncWebServerRequest* request, String& body) {
    // The browser sends form-encoded `body`, while direct API clients may send
    // a raw request body. Accept both formats to keep the API easy to inspect.
    if (request->hasParam("body", true)) {
        body = request->getParam("body", true)->value();
    } else {
        body = request->arg("plain");
    }
    return body.length() > 0;
}

void WebUI::setupRoutes() {
    // Dashboard pages: these requests should stay quiet after the page is served;
    // API activity below is what matters when diagnosing a user interaction.
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        Debug::println(3, "[WEBUI][GET /] serving dashboard");
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/api/set-time", HTTP_POST, [](AsyncWebServerRequest* request) {
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
        request->send(200, "application/json", "{\"success\":true}");
    });

    server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", Debug::getWebLogs());
    });

    // Log maintenance is deliberately logged before clearing because clearing
    // the buffer removes the diagnostic line that would otherwise describe it.
    server.on("/api/logs/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
        Debug::println(3, "[WEBUI][POST /api/logs/clear] clearing web log buffer");
        Debug::clearLogs();
        request->send(200, "application/json", "{\"success\":true}");
    });

    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
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
        request->send(200, "application/json", json);
    });

    // These controls are present in the dashboard, but no effect engine,
    // reboot policy, or persistent log export exists in this firmware yet.
    // Return explicit errors and record them instead of allowing silent 404s.
    server.on("/api/effect", HTTP_POST, [](AsyncWebServerRequest* request) {
        Debug::println(2, "[WEBUI][POST /api/effect] effect request rejected: no effect engine is configured");
        request->send(501, "application/json", "{\"success\":false,\"message\":\"Effects are not implemented in this build\"}");
    });

    server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
        Debug::println(2, "[WEBUI][POST /api/reboot] reboot request rejected: reboot endpoint is not implemented");
        request->send(501, "application/json", "{\"success\":false,\"message\":\"Reboot is not implemented in this build\"}");
    });

    server.on("/api/logs/save", HTTP_POST, [](AsyncWebServerRequest* request) {
        Debug::println(2, "[WEBUI][POST /api/logs/save] save request rejected: logs are RAM-only");
        request->send(501, "application/json", "{\"success\":false,\"message\":\"Persistent log saving is not implemented\"}");
    });

    // Action-script CRUD and execution routes. Log payload sizes and outcomes,
    // but leave detailed action semantics to ScriptManager.
    server.on("/api/action-scripts", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = scriptManager.getScriptsAsJson();
        if (DEBUG_LEVEL >= 3) {
            Debug::println(4, "[WEBUI][GET /api/action-scripts] returning " + String(json.length()) + " bytes");
        }
        request->send(200, "application/json", json);
    });

    server.on("/api/action-scripts/raw", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String raw = scriptManager.getRawScriptsFile();
        Debug::println(4, "[WEBUI][ACTION-SCRIPTS][RAW] returning " + String(raw.length()) + " bytes");
        if (raw.length() == 0) {
            Debug::println(1, "[WEBUI][ACTION-SCRIPTS][RAW] scripts file unavailable");
            request->send(404, "text/plain", "Scripts file is empty or unavailable.");
            return;
        }
        request->send(200, "application/json", raw);
    });

    server.on("/api/action-scripts", HTTP_POST, [this](AsyncWebServerRequest* request) {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] missing body param");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Send form field 'body' containing JSON\"}");
            return;
        }
        Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] payload length=" + String(body.length()) + " body=" + body);
        DynamicJsonDocument doc(16384);
        if (deserializeJson(doc, body) || !doc.is<JsonObject>()) {
            Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] invalid JSON payload");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        int id = doc["id"] | 0;
        Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] id=" + String(id) + " name=" + String((const char*)(doc["name"] | "")));
        bool ok = id > 0 ? scriptManager.updateScript(id, body) : scriptManager.addScript(body);
        Debug::println(2, "[WEBUI][ACTION-SCRIPTS][POST] save result=" + String(ok ? "true" : "false"));
        request->send(ok ? 200 : 500, "application/json",
                      ok ? "{\"success\":true}" : "{\"success\":false,\"message\":\"Save failed\"}");
    });

    server.on("/api/action-scripts", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        if (!request->hasParam("id")) {
            Debug::println(2, "[WEBUI][ACTION-SCRIPTS][DELETE] missing id param");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing id\"}");
            return;
        }
        int scriptId = request->getParam("id")->value().toInt();
        Debug::println(2, "[WEBUI][ACTION-SCRIPTS][DELETE] scriptId=" + String(scriptId));
        bool ok = scriptManager.deleteScript(scriptId);
        Debug::println(2, "[WEBUI][ACTION-SCRIPTS][DELETE] delete result=" + String(ok ? "true" : "false"));
        request->send(ok ? 200 : 404, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
    });

    server.on("/api/execute-script", HTTP_POST, [this](AsyncWebServerRequest* request) {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(1, "[WEBUI][POST /api/execute-script] missing body");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
            return;
        }
        DynamicJsonDocument doc(4096);
        if (deserializeJson(doc, body)) {
            Debug::println(1, "[WEBUI][POST /api/execute-script] invalid JSON");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        int id = doc["id"] | 0;
        Debug::println(2, "[WEBUI][EXECUTE-SCRIPT] requested id=" + String(id));
        bool ok = id > 0 && scriptManager.executeScript(id);
        Debug::println(2, "[WEBUI][EXECUTE-SCRIPT] result=" + String(ok ? "true" : "false"));
        request->send(ok ? 200 : 500, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
    });

    server.on("/api/test-action", HTTP_POST, [this](AsyncWebServerRequest* request) {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(1, "[WEBUI][POST /api/test-action] missing action body");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing action body\"}");
            return;
        }
        Debug::println(3, "[WEBUI][POST /api/test-action] action bytes=" + String(body.length()));
        bool ok = scriptManager.executeAction(body);
        Debug::println(ok ? 3 : 1, "[WEBUI][POST /api/test-action] result=" + String(ok ? "true" : "false"));
        request->send(ok ? 200 : 500, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
    });

    server.on("/api/learn", HTTP_POST, [this](AsyncWebServerRequest* request) {
        Debug::println(3, "[WEBUI][POST /api/learn] capture requested");
        String filter = "ANY";

        if (request->hasParam("filter", true)) {
            filter = request->getParam("filter", true)->value();
        } else {
            String body;
            if (getPostBody(request, body)) {
                DynamicJsonDocument doc(512);
                if (!deserializeJson(doc, body) && doc.is<JsonObject>()) {
                    filter = doc["filter"] | "ANY";
                }
            }
        }

        filter.trim();
        if (filter.length() == 0) filter = "ANY";
        Debug::println(3, "[WEBUI][POST /api/learn] starting capture filter=" + filter);
        scriptManager.startCapture(filter);
        request->send(200, "application/json", "{\"success\":true}");
    });

    server.on("/api/learn", HTTP_GET, [this](AsyncWebServerRequest* request) {
        Debug::println(4, "[WEBUI][GET /api/learn] capture status requested");
        request->send(200, "application/json", scriptManager.getCaptureStatus());
    });

    server.on("/action-scripts.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        Debug::println(3, "[WEBUI][GET /action-scripts.html] serving action editor");
        request->send(LittleFS, "/action_scripts.html", "text/html");
    });

    // Scene endpoints persist and apply the editor's saved scene definitions.
    // Each validation and hardware result is recorded so failed clicks explain
    // themselves in the debug log rather than in the connection indicator.
    server.on("/api/scenes", HTTP_GET, [this](AsyncWebServerRequest* request) {
        Debug::println(3, "[WEBUI][GET /api/scenes] listing scenes");
        request->send(200, "application/json", sceneManagerRef.getScenesAsJson());
    });

    server.on("/api/scenes", HTTP_POST, [this](AsyncWebServerRequest* request) {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(1, "[WEBUI][POST /api/scenes] missing scene body");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Send form field 'body' containing JSON\"}");
            return;
        }
        DynamicJsonDocument doc(4096);
        if (deserializeJson(doc, body) || !doc.is<JsonObject>()) {
            Debug::println(1, "[WEBUI][POST /api/scenes] invalid JSON payload");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        int id = doc["id"] | 0;
        bool ok = id > 0 ? sceneManagerRef.updateScene(id, body) : sceneManagerRef.addScene(body);
        Debug::println(ok ? 3 : 1, "[WEBUI][POST /api/scenes] " + String(id > 0 ? "update" : "add") + " result=" + String(ok ? "true" : "false"));
        request->send(ok ? 200 : 500, "application/json",
                      ok ? "{\"success\":true}" : "{\"success\":false,\"message\":\"Save failed\"}");
    });

    server.on("/api/scenes", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        if (!request->hasParam("id")) {
            Debug::println(1, "[WEBUI][DELETE /api/scenes] missing id");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing id\"}");
            return;
        }
        int sceneId = request->getParam("id")->value().toInt();
        bool ok = sceneManagerRef.deleteScene(sceneId);
        Debug::println(ok ? 3 : 1, "[WEBUI][DELETE /api/scenes] id=" + String(sceneId) + " result=" + String(ok ? "true" : "false"));
        request->send(ok ? 200 : 404, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
    });

    server.on("/api/scenes/apply", HTTP_POST, [this](AsyncWebServerRequest* request) {
        String body;
        if (!getPostBody(request, body)) {
            Debug::println(1, "[WEBUI][POST /api/scenes/apply] missing body");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
            return;
        }
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) {
            Debug::println(1, "[WEBUI][POST /api/scenes/apply] invalid JSON");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        int id = doc["id"] | 0;
        bool ok = id > 0 && sceneManagerRef.applyScene(id);
        Debug::println(ok ? 3 : 1, "[WEBUI][POST /api/scenes/apply] id=" + String(id) + " result=" + String(ok ? "true" : "false"));
        request->send(ok ? 200 : 500, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
    });

    server.on("/api/export-scenes", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // Return all scenes + actions as JSON for backup/sync
        DynamicJsonDocument doc(16384);
        doc["scenes"] = serialized(sceneManagerRef.getScenesAsJson());
        doc["actions"] = serialized(scriptManager.getScriptsAsJson());
        String json;
        serializeJson(doc, json);
        Debug::println(3, "[WEBUI][GET /api/export-scenes] exported " + String(json.length()) + " bytes");
        request->send(200, "application/json", json);
    });

    Debug::println("[WEBUI] Web routes configured");
    Debug::println("[WEBUI][ROUTES] Scene endpoints: /api/scenes, /api/scenes/apply, /api/export-scenes");
}

void WebUI::loop() {}
