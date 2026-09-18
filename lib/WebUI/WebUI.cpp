#include "WebUI.h"

WebUI::WebUI(ScriptManager &scriptManager, SceneManager &sceneManager,
             CommandSink &commandSink)
    : server(80), scriptManagerRef(scriptManager),
      sceneManagerRef(sceneManager), commandSinkRef(commandSink) {
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 3
  Debug::println(3, "[WebUI][Constructor] WebUI object created");
#endif
}

// ============================================================
// BEGIN
// ============================================================

void WebUI::begin() {
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
  Debug::println(2, "[WebUI][begin] Starting WebUI initialization");
#endif

  beginAP();

  // Initialize the Access Point for web interface

  /*
   * Managers are initialized by Core.
   *
   * WebUI deliberately does not call ScriptManager::begin()
   * or SceneManager::begin().
   *
   * Core owns the initialization order.
   */

  Debug::println(2, "[WEBUI][INIT] WebUI initialization complete");

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
  Debug::println(2, "[WebUI][begin] WebUI initialization complete");
#endif
}

// ============================================================
// ACCESS POINT
// ============================================================

void WebUI::beginAP() {
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 2
  Debug::println(2, "[WebUI][beginAP] Initializing WebUI in Access Point mode");
#endif

  // Configure ESP32 as Access Point
  WiFi.mode(WIFI_AP);

  // Set up IP configuration for the Access Point
  IPAddress localIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  // Start Access Point with SSID and password
  if (!WiFi.softAP("HomeController", apPassword.c_str())) {
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 1
    Debug::println(
        1, "[WebUI][beginAP] Failed to start Access Point with password: " +
               apPassword);
#endif
    // Log error and return early
    return;
  }

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 3
  Debug::println(
      3, "[WebUI][beginAP] Access Point started with SSID=HomeController");
  Debug::println(3, "[WebUI][beginAP] Access Point IP: " +
                        WiFi.softAPIP().toString());
#endif

  // Setup web server routes
  setupRoutes();

  // Start the web server
  server.begin();

  setupRoutes();

  server.begin();

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 3
  Debug::println(3, "[WebUI][beginAP] Web server started on port 80");
#endif
}

// ============================================================
// POST BODY HELPER
// ============================================================

bool WebUI::getPostBody(AsyncWebServerRequest *request, String &body) {
#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= 1
  Debug::println(1, "[WebUI][getPostBody] Processing POST request");
#endif
  // Log request content information
  Debug::println(1, "[WEBUI][getPostBody] Content-Type: " +
                        String(request->contentType()));
  Debug::println(1, "[WEBUI][getPostBody] Content-Length: " +
                        String(request->contentLength()));

  // Check for JSON content type
  if (request->contentType() == "application/json") {
    // For JSON content, the body should be in _tempObject
    if (request->_tempObject) {
      body = String((char *)request->_tempObject);
    } else {
      // Fallback to reading the request stream
      body = "";
      int len = request->contentLength();
      if (len > 0) {
        char *buf = new char[len + 1];
        request->getParam(0)->value().toCharArray(buf, len + 1);
        buf[len] = 0;
        body = String(buf);
        delete[] buf;
      }
    }
  } else if (request->hasParam("body", true)) {
    body = request->getParam("body", true)->value();
  } else if (request->_tempObject) {
    body = String((char *)request->_tempObject);
  } else {
    body = request->arg("plain");
  }

  if (body.length() > 0) {
    Debug::println(3, "[WEBUI][POST] Parsed body: " + body);
    return true;
  }

  Debug::println(1, "[WEBUI][getPostBody] Empty POST body");
  return false;
}

// ============================================================
// capturePostBody
// ============================================================

static void capturePostBody(AsyncWebServerRequest *request, uint8_t *data,
                            size_t len, size_t index, size_t total) {
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

void WebUI::setupRoutes() {
  // --------------------------------------------------------
  // DASHBOARD
  // --------------------------------------------------------

  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!isClientAuthenticated(request)) {
      request->send(401, "text/html",
                    "<html><body>"
                    "<h1>Authentication Required</h1>"
                    "<form method='POST' action='/login'>"
                    "<input type='password' name='password'>"
                    "<input type='submit' value='Login'>"
                    "</form>"
                    "</body></html>");

      return;
    }

    authenticateClient(request);

    request->send(LittleFS, "/index.html", "text/html");
  });

  // --------------------------------------------------------
  // SCRIPT EDITOR
  // --------------------------------------------------------

  server.on("/action_scripts.html", HTTP_GET,
            [this](AsyncWebServerRequest *request) {
              if (!isClientAuthenticated(request)) {
                request->send(401, "text/html",
                              "<html><body>"
                              "<h1>Authentication Required</h1>"
                              "</body></html>");

                return;
              }

              authenticateClient(request);

              if (!LittleFS.exists("/action_scripts.html")) {
                request->send(404, "text/plain",
                              "action_scripts.html missing from filesystem");

                return;
              }

              request->send(LittleFS, "/action_scripts.html", "text/html");
            });

  // --------------------------------------------------------
  // AUDIO TEST PAGE
  // --------------------------------------------------------

  server.on("/audio_test.html", HTTP_GET,
            [this](AsyncWebServerRequest *request) {
              if (!isClientAuthenticated(request)) {
                request->send(401, "text/html",
                              "<html><body>"
                              "<h1>Authentication Required</h1>"
                              "</body></html>");

                return;
              }

              authenticateClient(request);

              if (!LittleFS.exists("/audio_test.html")) {
                request->send(404, "text/plain",
                              "audio_test.html missing from filesystem");

                return;
              }

              request->send(LittleFS, "/audio_test.html", "text/html");
            });

  // --------------------------------------------------------
  // LOGIN
  // --------------------------------------------------------

  server.on("/login", HTTP_POST, [this](AsyncWebServerRequest *request) {
    String password;

    if (request->hasParam("password", true)) {
      password = request->getParam("password", true)->value();
    } else {
      password = request->arg("password");
    }

    if (password == adminPassword) {
      authenticateClient(request);
      request->redirect("/");

      return;
    }

    Debug::println(2, "[WEBUI][LOGIN] Failed login attempt from " +
                          request->client()->remoteIP().toString());

    request->send(401, "text/html",
                  "<html><body>"
                  "<h1>Authentication Failed</h1>"
                  "<form method='POST' action='/login'>"
                  "<input type='password' name='password'>"
                  "<input type='submit' value='Login'>"
                  "</form>"
                  "</body></html>");
  });

  // --------------------------------------------------------
  // AUTH WRAPPER
  // --------------------------------------------------------

  auto withAuth = [this](std::function<void(AsyncWebServerRequest *)> handler) {
    return [this, handler](AsyncWebServerRequest *request) {
      if (!isClientAuthenticated(request)) {
        request->send(401, "application/json",
                      "{\"success\":false,"
                      "\"message\":\"Authentication required\"}");

        return;
      }

      authenticateClient(request);

      handler(request);
    };
  };

  // ========================================================
  // TIME
  // ========================================================

  server.on("/api/set-time", HTTP_POST,
            withAuth([this](AsyncWebServerRequest *request) {
              String body;

              if (!getPostBody(request, body)) {
                request->send(400, "application/json",
                              "{\"success\":false,"
                              "\"message\":\"Missing time payload\"}");

                return;
              }

              JsonDocument doc;

              DeserializationError err = deserializeJson(doc, body);

              if (err || !doc["time"].is<unsigned long>()) {
                request->send(400, "application/json",
                              "{\"success\":false,"
                              "\"message\":\"Invalid time payload\"}");

                return;
              }

              unsigned long unixTime = doc["time"].as<unsigned long>();

              int timezoneOffsetMinutes = doc["timezoneOffsetMinutes"] | 0;

              Debug::setTimezoneOffsetMinutes(timezoneOffsetMinutes);

              Debug::setDeviceTime(unixTime);

              request->send(200, "application/json", "{\"success\":true}");
            }));

  // ========================================================
  // STATUS
  // ========================================================

  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    JsonDocument doc;

    int core0Busy = 100 - (int)ulTaskGetIdleRunTimePercentForCore(0);

    int core1Busy = 100 - (int)ulTaskGetIdleRunTimePercentForCore(1);

    doc["uptimeMs"] = millis();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["cpuLoadPercent"] = (core0Busy + core1Busy) / 2;

    doc["ssid"] = "HomeController";
    doc["ip"] = WiFi.softAPIP().toString();

    doc["mode"] = "AP";

    doc["timestampMode"] =
        (Debug::getTimestamp().length() > 0) ? "wall-clock" : "uptime";

    doc["heartbeat"] = heartbeatState;

    String json;

    serializeJson(doc, json);

    request->send(200, "application/json", json);
  });

  // ========================================================
  // LOGS
  // ========================================================

  server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", Debug::getWebLogs());
  });

  server.on("/api/logs/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
    Debug::println(3, "[WEBUI][POST /api/logs/clear] clearing log buffer");

    Debug::clearLogs();

    request->send(200, "application/json", "{\"success\":true}");
  });

  // ========================================================
  // DEBUG LEVEL
  // ========================================================

  // POST endpoint to update debug level
  server.on(
      "/api/set-debug-level", HTTP_POST,
      withAuth([this](AsyncWebServerRequest *request) {
        String body;

        if (!getPostBody(request, body)) {
          request->send(
              400, "application/json",
              R"json({"success":false,"message":"Missing debug level payload"})json");
          return;
        }

        // Upgraded to true ArduinoJson v7 style JsonDocument (Fixed capacity no
        // longer needed)
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
          request->send(
              400, "application/json",
              R"json({"success":false,"message":"Invalid JSON"})json");
          return;
        }

        // Get debug level
        int level = doc["level"] | 3; // Default to INFO level if not specified

        // Validate debug level
        if (level < 1 || level > 5) {
          request->send(
              400, "application/json",
              R"json({"success":false,"message":"Invalid debug level. Must be between 1 and 5."})json");
          return;
        }

        // Set debug level
        Debug::setDebugLevel(level);

        // FIXED: Properly formatted dynamic JSON string with snprintf
        char responseBuffer[64];
        snprintf(responseBuffer, sizeof(responseBuffer),
                 "{\"success\":true,\"level\":%d}", level);

        request->send(200, "application/json", responseBuffer);
      }));

  // GET endpoint to retrieve current debug level
  server.on("/api/debug-level", HTTP_GET,
            withAuth([this](AsyncWebServerRequest *request) {
              int level = Debug::getDebugLevel();

              // Upgraded to true ArduinoJson v7 style JsonDocument
              JsonDocument doc;
              doc["level"] = level;

              String response;
              serializeJson(doc, response);

              request->send(200, "application/json", response);
            }));

  // ========================================================
  // SCRIPTS - LIST
  // ========================================================

  server.on("/api/action-scripts", HTTP_GET,
            withAuth([this](AsyncWebServerRequest *request) {
              String json = scriptManagerRef.getScriptsAsJson();

              request->send(200, "application/json", json);
            }));

  // ========================================================
  // SCRIPTS - ADD / UPDATE
  // ========================================================

  server.on(
      "/api/action-scripts", HTTP_POST,
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
          request->send(200, "application/json", "{\"success\":true}");
        } else {
          Debug::println(1, "[WEBUI][SCRIPTS][ERROR] Save failed");
          request->send(500, "application/json",
                        "{\"success\":false,\"message\":\"Save failed\"}");
        }
      }),
      nullptr, capturePostBody);

  // ========================================================
  // SCRIPTS - DELETE
  // ========================================================

  server.on("/api/action-scripts", HTTP_DELETE,
            withAuth([this](AsyncWebServerRequest *request) {
              if (!request->hasParam("id")) {
                request->send(400, "application/json",
                              "{\"success\":false,"
                              "\"message\":\"Missing id\"}");

                return;
              }

              int scriptId = request->getParam("id")->value().toInt();

              bool success = scriptManagerRef.deleteScript(scriptId);

              request->send(success ? 200 : 404, "application/json",
                            success ? "{\"success\":true}"
                                    : "{\"success\":false,"
                                      "\"message\":\"Script not found\"}");
            }));

  // ========================================================
  // SCRIPTS - APPLY
  //
  // Kept as an alias for the existing UI.
  // There is no separate "apply to hardware" concept anymore.
  // Saving the script is enough.
  // ========================================================

  server.on("/api/action-scripts/apply", HTTP_POST,
            withAuth([this](AsyncWebServerRequest *request) {
              String body;

              if (!getPostBody(request, body)) {
                request->send(400, "application/json",
                              "{\"success\":false,"
                              "\"message\":\"Missing body\"}");

                return;
              }

              if (!saveOrUpdateScript(body)) {
                request->send(500, "application/json",
                              "{\"success\":false,"
                              "\"message\":\"Failed to save script\"}");

                return;
              }

              request->send(200, "application/json", "{\"success\":true}");
            }),
            nullptr, capturePostBody);

  // ========================================================
  // EXECUTE SCRIPT
  //
  // This endpoint allows executing scripts by ID, name, or full JSON object.
  // The endpoint supports three request formats:
  // 1. {"id": 1} - Execute script by ID (preferred method)
  // 2. {"name": "Script Name"} - Execute script by name or alias
  // 3. {"script": "{\"name\": \"Script Name\", ...}"} - Execute with full JSON
  //    This is used by the editor test functionality.
  //
  // Execution flow:
  // ScriptManager -> CommandSink -> Core -> HardwareManager
  //
  // WebUI never directly interacts with hardware.
  // ========================================================

  server.on(
      "/api/execute-script", HTTP_POST,
      withAuth([this](AsyncWebServerRequest *request) {
        String body;

        if (!getPostBody(request, body)) {
          request->send(400, "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Missing body\"}");

          return;
        }

        JsonDocument doc;

        if (deserializeJson(doc, body) != DeserializationError::Ok) {
          request->send(400, "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Invalid JSON\"}");

          return;
        }

        bool success = false;

        // Execute script by ID (integer)
        // This is the preferred method when the script ID is known
        if (doc["id"].is<int>()) {
          const int scriptId = doc["id"].as<int>();
          Debug::println(2,
                         "[WebUI] Executing script by ID: " + String(scriptId));
          success = scriptManagerRef.runScript(scriptId);
        }
        // Execute script by name (string)
        // This is used when the script name or alias is known
        else if (doc["name"].is<String>()) {
          const String scriptName = doc["name"].as<String>();
          Debug::println(2, "[WebUI] Executing script by name: " + scriptName);
          success = scriptManagerRef.runScript(scriptName);
        } else if (doc["script"].is<String>()) {
          // Handle script execution with full JSON object
          // This is used by the editor test functionality where the web
          // interface sends a complete script JSON object instead of just a
          // name or ID
          Debug::println(
              2, "[WebUI] Processing script execution with full JSON object");

          const String scriptJson = doc["script"].as<String>();
          Debug::println(3, "[WebUI] Script JSON: " + scriptJson);

          // Parse the script JSON to extract the name
          JsonDocument scriptDoc;
          DeserializationError error = deserializeJson(scriptDoc, scriptJson);

          if (error || !scriptDoc.is<JsonObject>()) {
            Debug::println(1, "[WebUI] Failed to parse script JSON: " +
                                  String(error.c_str()));
            request->send(
                400, "application/json",
                R"json({"success":false,"message":"Invalid script JSON"})json");
            return;
          }

          // Extract script name if available
          if (scriptDoc["name"].is<String>()) {
            const String scriptName = scriptDoc["name"].as<String>();
            Debug::println(2,
                           "[WebUI] Executing script by name: " + scriptName);

            // Special case for __EDITOR_TEST__ - extract and execute commands
            // directly
            if (scriptName == "__EDITOR_TEST__") {
              Debug::println(2, "[WebUI] Special case: __EDITOR_TEST__ - "
                                "executing commands directly");

              if (scriptDoc["commands"].is<JsonArray>()) {
                JsonArray commands = scriptDoc["commands"].as<JsonArray>();
                Debug::println(2, "[WebUI] Found " + String(commands.size()) +
                                      " commands to execute");

                success = true;
                for (JsonVariant command : commands) {
                  if (!commandSinkRef.executeCommand(command.as<String>())) {
                    Debug::println(1, "[WebUI] Command failed: " +
                                          String(command.as<String>()));
                    success = false;
                  }
                }
              } else {
                Debug::println(1,
                               "[WebUI] No commands found in __EDITOR_TEST__");
                success = false;
              }
            } else {
              // Normal script execution
              success = scriptManagerRef.runScript(scriptName);
            }
          } else {
            Debug::println(1, "[WebUI] Script JSON missing name field");
            request->send(
                400, "application/json",
                R"json({"success":false,"message":"Script JSON missing name field"})json");
            return;
          }
        }

        // Send response based on script execution result
        // 200 OK with success:true if script executed successfully
        // 404 Not Found with success:false and error message if script failed
        request->send(
            success ? 200 : 404, "application/json",
            success
                ? R"json({"success":true})json"
                : R"json({"success":false,"message":"Script not found or failed"})json");
      }),
      nullptr, capturePostBody);

  // ========================================================
  // SCENES - LIST
  // ========================================================

  server.on("/api/scenes", HTTP_GET,
            withAuth([this](AsyncWebServerRequest *request) {
              String json = sceneManagerRef.getScenesAsJson();

              request->send(200, "application/json", json);
            }));

  // ========================================================
  // ACTIVATE SCENE
  //
  // WebUI is one of the two allowed scene triggers:
  //
  // WebUI -> SceneManager -> ScriptManager -> Core
  //
  // ========================================================

  server.on(
      "/api/scenes/apply", HTTP_POST,
      withAuth([this](AsyncWebServerRequest *request) {
        String body;

        if (!getPostBody(request, body)) {
          request->send(400, "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Missing body\"}");

          return;
        }

        JsonDocument doc;

        if (deserializeJson(doc, body) != DeserializationError::Ok) {
          request->send(400, "application/json",
                        "{\"success\":false,"
                        "\"message\":\"Invalid JSON\"}");

          return;
        }

        bool success = false;

        if (doc["id"].is<int>()) {
          success = sceneManagerRef.activateScene(doc["id"].as<int>());
        } else if (doc["name"].is<String>()) {
          success = sceneManagerRef.activateScene(doc["name"].as<String>());
        } else if (doc["scene"].is<String>()) {
          success = sceneManagerRef.activateScene(doc["scene"].as<String>());
        }

        request->send(success ? 200 : 404, "application/json",
                      success ? "{\"success\":true}"
                              : "{\"success\":false,"
                                "\"message\":\"Scene not found or failed\"}");
      }));

  // ========================================================
  // SCENE EXPORT
  // ========================================================

  server.on("/api/export-scenes", HTTP_GET,
            withAuth([this](AsyncWebServerRequest *request) {
              String json = sceneManagerRef.getScenesAsJson();

              request->send(200, "application/json", json);
            }));

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

  server.on("/api/effect", HTTP_POST,
            withAuth([](AsyncWebServerRequest *request) {
              request->send(501, "application/json",
                            "{\"success\":false,"
                            "\"message\":\"Effects are not implemented yet\"}");
            }));

  // ========================================================
  // REBOOT
  // ========================================================

  server.on("/api/reboot", HTTP_POST,
            withAuth([](AsyncWebServerRequest *request) {
              request->send(501, "application/json",
                            "{\"success\":false,"
                            "\"message\":\"Reboot is not implemented\"}");
            }));

  // ========================================================
  // LOG SAVE
  // ========================================================

  server.on("/api/logs/save", HTTP_POST,
            withAuth([](AsyncWebServerRequest *request) {
              request->send(
                  501, "application/json",
                  "{\"success\":false,"
                  "\"message\":\"Persistent log saving is not implemented\"}");
            }));

  // ========================================================
  // NOT FOUND
  // ========================================================

  server.onNotFound([](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    String path = request->url();

    Debug::println(2, "[WEBUI][NOTFOUND] Client: " + clientIP +
                          " requested path: " + path);

    request->send(404, "text/plain", "Not found");
  });
}

// ============================================================
// LOOP
// ============================================================

void WebUI::loop() {
  /*
   * AsyncWebServer does not need a polling loop.
   *
   * Kept for compatibility with App/Core.
   */
}

// ============================================================
// HEARTBEAT
// ============================================================

void WebUI::setHeartbeatState(bool state) { heartbeatState = state; }

// ============================================================
// AUTHENTICATION
// ============================================================

void WebUI::setAuthentication(bool enabled, const String &password) {
  bool wasEnabled = authenticationEnabled;
  // Keep authentication disabled regardless of the enabled parameter
  authenticationEnabled = false;

  if (password.length() > 0) {
    adminPassword = password;
    Debug::println(3, "[WEBUI][AUTH] Admin password updated");
  }

  Debug::println(
      2, "[WEBUI][AUTH] Authentication disabled (automatic access enabled)");
}

void WebUI::setAPPassword(const String &password) {
  apPassword = password;

  if (WiFi.getMode() & WIFI_AP) {
    bool success = WiFi.softAP("HomeController", apPassword.c_str());

    Debug::println(
        2, "[WEBUI][AUTH] AP password " +
               String(success ? "updated successfully" : "update failed"));
  }
}

void WebUI::setFixedPassword(const String &password) {
  adminPassword = password;

  Debug::println(2, "[WEBUI][AUTH] Fixed password updated");
}

// ============================================================
// SCRIPT SAVE / UPDATE
// ============================================================

bool WebUI::saveOrUpdateScript(const String &scriptJson) {
  JsonDocument doc;

  if (deserializeJson(doc, scriptJson) != DeserializationError::Ok) {
    Debug::println(
        1, "[WEBUI][SCRIPTS] Invalid JSON: " +
               scriptJson.substring(0, min((unsigned int)scriptJson.length(),
                                           (unsigned int)100)) +
               (scriptJson.length() > 100 ? "..." : ""));

    return false;
  }

  String script;

  if (doc["script"].is<String>()) {
    script = doc["script"].as<String>();
  } else if (doc.is<JsonObject>()) {
    serializeJsonPretty(doc, script);
  } else {
    return false;
  }

  JsonDocument scriptDoc;

  if (deserializeJson(scriptDoc, script) != DeserializationError::Ok) {
    return false;
  }

  int id = scriptDoc["id"] | 0;

  if (id > 0 && scriptManagerRef.getScriptById(id).length() > 0) {
    return scriptManagerRef.updateScript(id, script);
  }

  return scriptManagerRef.addScript(script);
}

// ============================================================
// AUTH CLIENT TRACKING
// ============================================================

bool WebUI::isClientAuthenticated(AsyncWebServerRequest *request) {
  if (!authenticationEnabled) {
    return true;
  }

  String clientIP = request->client()->remoteIP().toString();

  return std::find(authenticatedClients.begin(), authenticatedClients.end(),
                   clientIP) != authenticatedClients.end();
}

void WebUI::authenticateClient(AsyncWebServerRequest *request) {
  if (!authenticationEnabled) {
    return;
  }

  String clientIP = request->client()->remoteIP().toString();

  if (std::find(authenticatedClients.begin(), authenticatedClients.end(),
                clientIP) == authenticatedClients.end()) {
    authenticatedClients.push_back(clientIP);

    Debug::println(2, "[WEBUI][AUTH] Client authenticated: " + clientIP +
                          " (total authenticated clients: " +
                          String(authenticatedClients.size()) + ")");
  } else {
    Debug::println(3,
                   "[WEBUI][AUTH] Client already authenticated: " + clientIP);
  }
}
