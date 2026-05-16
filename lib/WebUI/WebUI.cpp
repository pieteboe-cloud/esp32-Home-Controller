#include "WebUI.h"
#include "WebInterface.h"
#include "../Debug/Debug.h"
#include "../ControlLivingColors/ControlLivingColors.h"
#include "../SettingsManager/SettingsManager.h"
#include "../AutomationManager/AutomationManager.h"
#include "../LampManager/LampManager.h"
#include "../IRremote/IRController.h"
#include <HardwareConfig.h>
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>

WebUI::WebUI() : server(80) {}

void WebUI::init(const char* ssid, CommandCallback callback) {
    onCommand = callback;

    // Start secure AP with password
    const char* AP_PASSWORD = "HomeCtrl123!";  // Change this to a secure password
    WiFi.softAP(ssid, AP_PASSWORD);
    IPAddress IP = WiFi.softAPIP();

    // Captive Portal: Redirect all DNS queries to the ESP IP
    dnsServer.start(53, "*", IP);

    setupRoutes();
    server.begin();
    Debug::println("[INFO][WEBUI] Secure WebUI AP Started. SSID: " + String(ssid) + " Password: " + AP_PASSWORD);
}

void WebUI::setControllers(ControlLivingColors* c, AutomationManager* a, SettingsManager* s, LampManager* l) {
    control = c;
    automation = a;
    settings = s;
    lampManager = l;
}

void WebUI::setIRController(IRController* ir) {
    irController = ir;
}

void WebUI::setStatusProvider(StatusProvider provider) {
    statusProvider = provider;
}

void WebUI::setRebootCallback(RebootCallback callback) {
    onReboot = callback;
     // Set reboot callback
      //  settingsManager.reboot();
   
}

void WebUI::setSettingsCallback(SettingsCallback callback) {
    onGetSettings = callback;
}

void WebUI::setupRoutes() {
    // Captive Portal Redirect: Force OS to recognize the login page
    auto captiveRedirect = [this]() {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
        server.send(302, "text/plain", ""); // 302 Redirect is the standard trigger
    };

    server.on("/generate_204", captiveRedirect); // Android connectivity check
    server.on("/favicon.ico", [this]() { server.send(204); });

    server.on("/", [this]() {
        server.send(200, "text/html", INDEX_HTML);
    });

    server.on("/logs", [this]() {
        server.send(200, "text/plain", Debug::getWebLogs());
    });

    server.on("/status", [this]() {
        if (statusProvider) {
            server.send(200, "text/plain", statusProvider() ? "on" : "off");
        } else {
            server.send(200, "text/plain", "unknown");
        }
    });

    server.on("/command", [this]() {
        if (server.hasArg("val")) {
            String cmd = server.arg("val");
            processCommand(cmd);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing Command");
        }
    });

    server.on("/reboot", [this]() {
        if (onReboot) {
            server.send(200, "text/plain", "Rebooting...");
            onReboot();
        } else {
            server.send(500, "text/plain", "Reboot callback not set");
        }
    });

    server.on("/settings", [this]() {
        if (onGetSettings) {
            String settings = onGetSettings();
            server.send(200, "application/json", settings);
        } else {
            server.send(500, "text/plain", "Settings callback not set");
        }
    });

    server.on("/verbose", [this]() {
        if (server.hasArg("val")) {
            int val = server.arg("val").toInt();
            Debug::setVerbose(val == 1);
            server.send(200, "text/plain", Debug::isVerbose() ? "ON" : "OFF");
        } else {
            server.send(400, "text/plain", "Missing value");
        }
    });

    server.on("/clearlogs", [this]() {
        Debug::clearLogs();
        server.send(200, "text/plain", "OK");
    });

    server.on("/settime", [this]() {
        if (server.hasArg("time")) {
            unsigned long unixTime = server.arg("time").toInt();
            Debug::setDeviceTime(unixTime);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing time parameter");
        }
    });

    server.on("/lamp", [this]() {
        if (!server.hasArg("index") || !server.hasArg("cmd")) {
            server.send(400, "text/plain", "Missing parameters");
            return;
        }

        int lampIndex = server.arg("index").toInt();
        String cmd = server.arg("cmd");

        if (lampIndex < 0 || lampIndex > 10) {
            server.send(400, "text/plain", "Invalid lamp index");
            return;
        }

        // Forward the lamp command to the WebUI command processor
        String fullCmd = "lamp_" + cmd + ":" + String(lampIndex);
        if (cmd == "color" && server.hasArg("hue")) {
            fullCmd += ":" + server.arg("hue") + ":" + server.arg("sat") + ":" + server.arg("val");
        }
        processCommand(fullCmd);
        server.send(200, "text/plain", "OK");
    });

    server.onNotFound(captiveRedirect); // Redirect all other unknown requests
}

void WebUI::processCommand(String cmd) {
    if (!control || !automation || !settings || !lampManager) return;

    Debug::print("[INFO][WEBUI] Processing Command: ");
    Debug::println(cmd.c_str());

    if (cmd == "rainbow_effect") {
        cmd = "scene:rainbow_effect";
    }

    if (!cmd.startsWith("scene:") && cmd != "scene_off" && !cmd.startsWith("master_dim:") && !cmd.startsWith("strip:") && !cmd.startsWith("preset_")) {
        lampManager->clearEffect();
    }

    if (cmd.startsWith("scene:")) {
        String sceneName = cmd.substring(6);
        if (sceneName == "rainbow_effect" || sceneName == "disco_party" || sceneName == "neon_madness" ||
            sceneName == "psychedelic" || sceneName == "energy_burst" || sceneName == "volcanic_rage" ||
            sceneName == "ice_cold" || sceneName == "purple_dream" || sceneName == "sunset_fade" ||
            sceneName == "rave_mode") {
            lampManager->setEffect(sceneName);
        } else {
            lampManager->clearEffect();
            automation->executeScene(sceneName);
        }
    } else if (cmd == "scene_off") {
        lampManager->clearEffect();
    } else if (cmd.startsWith("lamp_")) {
        int firstColon = cmd.indexOf(':');
        int secondColon = cmd.indexOf(':', firstColon + 1);
        String lampCmd = cmd.substring(5, firstColon);
        int lampIndex = cmd.substring(firstColon + 1, secondColon > 0 ? secondColon : cmd.length()).toInt();

        if (lampIndex >= 0 && lampIndex < 11) {
            if (lampCmd == "on") {
                lampManager->turnLampOn(lampIndex);
            } else if (lampCmd == "off") {
                lampManager->turnLampOff(lampIndex);
            } else if (lampCmd == "white") {
                lampManager->turnLampWhite(lampIndex);
            } else if (lampCmd == "color") {
                int thirdColon = cmd.indexOf(':', secondColon + 1);
                int fourthColon = cmd.indexOf(':', thirdColon + 1);
                int hue = cmd.substring(secondColon + 1, thirdColon).toInt();
                int sat = cmd.substring(thirdColon + 1, fourthColon).toInt();
                int val = cmd.substring(fourthColon + 1).toInt();
                hue = constrain(hue, 0, 255);
                sat = constrain(sat, 0, 255);
                val = constrain(val, 0, 255);
                lampManager->setLampColor(lampIndex, hue, sat, val);
            }
        }
    } else if (cmd.startsWith("master_dim:")) {
        int val = cmd.substring(11).toInt();
        val = constrain(val, 0, 255);
        lampManager->setMasterBrightness((uint8_t)val);
    } else if (cmd.startsWith("preset_")) {
        int slot = cmd.substring(12).toInt();
        if (slot >= 0 && slot <= 4) {
            if (cmd.startsWith("preset_save")) {
                lampManager->savePreset(slot);
            } else {
                lampManager->loadPreset(slot);
            }
        }
    } else if (cmd.startsWith("strip:")) {
        int firstColon = cmd.indexOf(':');
        int secondColon = cmd.indexOf(':', firstColon + 1);
        String zone = cmd.substring(firstColon + 1, secondColon);
        uint32_t hexCode = strtoul(cmd.substring(secondColon + 1).c_str(), NULL, 16);

        int targetPin = PIN_IR_BOTTOM;
        if (zone == "mid") targetPin = PIN_IR_MID;
        else if (zone == "upper") targetPin = PIN_IR_UPPER;

        IrSender.setSendPin(targetPin);
        IrSender.sendNECMSB(hexCode, 32); // Use sendNECMSB for raw hex codes in IRremote v4
    } else {
        automation->executeScene(cmd);
    }

    if (onCommand) onCommand("pulse");
}

void WebUI::handle() {
    dnsServer.processNextRequest();
    server.handleClient();
}
