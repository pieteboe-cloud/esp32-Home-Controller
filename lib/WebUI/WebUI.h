#ifndef WEB_UI_H
#define WEB_UI_H

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <functional>
#include <LampTypes.h>

// Forward declarations
class SettingsManager;
class ControlLivingColors;
class AutomationManager;
class LampManager;
class IRController;

class WebUI {
public:
    typedef std::function<void(String)> CommandCallback;
    typedef std::function<bool()> StatusProvider;
    typedef std::function<void()> RebootCallback;
    typedef std::function<String()> SettingsCallback;

    WebUI();
    void init(const char* ssid, CommandCallback callback);
    void setStatusProvider(StatusProvider provider);
    void setRebootCallback(RebootCallback callback);
    void setSettingsCallback(SettingsCallback callback);
    
    // Link core modules for command processing
    void setControllers(ControlLivingColors* c, AutomationManager* a, SettingsManager* s, LampManager* l);
    void setIRController(IRController* ir);

    void handle();
    void processCommand(String cmd);

private:
    WebServer server;
    DNSServer dnsServer;
    CommandCallback onCommand;
    StatusProvider statusProvider;
    RebootCallback onReboot;
    SettingsCallback onGetSettings;

    // Pointers to main application objects
    ControlLivingColors* control = nullptr;
    AutomationManager* automation = nullptr;
    SettingsManager* settings = nullptr;
    LampManager* lampManager = nullptr;
    IRController* irController = nullptr;

    void setupRoutes();
};

#endif
