#include "Core.h"
#include <stdlib.h>
#include <time.h>

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
//
// Construction order is important.
//
// HardwareManager
//      ↓
// ScriptManager(CommandSink)
//      ↓
// SceneManager(ScriptManager)
//      ↓
// WebUI(ScriptManager, SceneManager)
//
// Core is the CommandSink, so ScriptManager can safely store a reference to
// Core without ever knowing about HardwareManager.
//
Core::Core()
    : hardware(),
      scriptManager(*this),
      sceneManager(scriptManager),
      webui (scriptManager, sceneManager, *this)
{
}


// -----------------------------------------------------------------------------
// init
// -----------------------------------------------------------------------------
//
// Core owns the startup order.
//
// HardwareManager owns:
//   - Storage
//   - LivingColors
//   - RF
//   - Kaku
//   - IR
//   - Audio
//
// Therefore Core must NOT initialize Storage separately.
//
void Core::init()
{
    Debug::logSubsystemStatus(
        "System",
        "initializing",
        "hardware -> translator -> scenes -> webui -> eventbus"
    );

    // -------------------------------------------------------------------------
    // Heartbeat
    // -------------------------------------------------------------------------

    pinMode(HEARTBEAT_PIN, OUTPUT);

    heartbeatState = LOW;
    digitalWrite(HEARTBEAT_PIN, heartbeatState);


    // -------------------------------------------------------------------------
    // Hardware
    // -------------------------------------------------------------------------
    //
    // HardwareManager owns storage initialization as well as all physical
    // controllers.
    //

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE] Initializing HardwareManager");
#endif

    hardware.init();


    // -------------------------------------------------------------------------
    // Translator
    // -------------------------------------------------------------------------
    //
    // Translator is responsible for converting external representations into
    // the canonical command vocabulary used by Core.
    //

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE] Initializing Translator");
#endif

    Translator::init();

    // -------------------------------------------------------------------------
    // ScriptManager
    // -------------------------------------------------------------------------

#if DEBUG_LEVEL  >= 2
    Debug::println(2, "[CORE] Initializing ScriptManager");
#endif

    scriptManager.begin();


    // -------------------------------------------------------------------------
    // SceneManager
    // -------------------------------------------------------------------------

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE] Initializing SceneManager");
#endif

    sceneManager.begin();


    // -------------------------------------------------------------------------
    // WebUI
    // -------------------------------------------------------------------------
    //
    // WebUI receives references to ScriptManager and SceneManager.
    //
    // It does NOT initialize ScriptManager itself.
    //

    String fixedPassword = "admin123";

    webui.setAuthentication(true, fixedPassword);
    webui.setAPPassword(fixedPassword);
    webui.setFixedPassword(fixedPassword);

#if DEBUG_LEVEL >= 2
    Debug::println(
        2,
        "[CORE] Starting WebUI"
    );
#endif

    webui.begin();


    // -------------------------------------------------------------------------
    // EventBus
    // -------------------------------------------------------------------------
    //
    // HardwareManager publishes events.
    //
    // Core listens to those events and routes only the events that belong to
    // application orchestration.
    //
    // Kaku is handled here and forwarded to SceneManager.
    //

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE] Registering EventBus listener");
#endif

    EventBus::getInstance().subscribe(
        [this](const SystemEvent& evt)
        {
            this->handleSystemEvent(evt);
        }
    );


#if DEBUG_LEVEL >= 2
    Debug::println(2, "[CORE] Initialization complete");
#endif
}


// -----------------------------------------------------------------------------
// update
// -----------------------------------------------------------------------------
//
// Keep the main loop deliberately small.
//
// HardwareManager is responsible for polling the physical controllers.
// Core only performs application-level periodic work.
//
void Core::update()
{
    hardware.update();


    // -------------------------------------------------------------------------
    // Heartbeat
    // -------------------------------------------------------------------------

    if (millis() - lastHeartbeatTime >= 500)
    {
        lastHeartbeatTime = millis();

        heartbeatState = !heartbeatState;

        digitalWrite(
            HEARTBEAT_PIN,
            heartbeatState
        );

        webui.setHeartbeatState(heartbeatState);

#if DEBUG_LEVEL >= 3
        Debug::println(
            3,
            "[CORE][heartbeat] Heartbeat toggled to " +
            String(heartbeatState ? "HIGH" : "LOW")
        );
#endif
    }
}


// -----------------------------------------------------------------------------
// EventBus handling
// -----------------------------------------------------------------------------
//
// HardwareManager publishes low-level events.
//
// Core decides which application component should react.
//
// Currently:
//
//   KAKU  -> SceneManager
//   other -> observe/log
//
// IR does NOT activate scenes here.
// Audio beats do NOT directly execute scripts here.
// Those responsibilities belong to the higher-level systems.
//
void Core::handleSystemEvent(const SystemEvent& evt)
{
#if DEBUG_LEVEL >= 3

    if (evt.source.equalsIgnoreCase("AUDIO"))
    {
        Debug::println(
            3,
            "[CORE][AUDIO] " +
            evt.identifier +
            " | " +
            evt.rawData
        );
    }
    else
    {
        Debug::println(
            3,
            "[CORE][EVENT] " +
            evt.source +
            " -> " +
            evt.identifier
        );
    }

#endif


#if DEBUG_LEVEL >= 2

    if (!evt.source.equalsIgnoreCase("AUDIO"))
    {
        Debug::println(
            2,
            "[CORE][EVENT] " +
            evt.source +
            " -> " +
            evt.identifier
        );
    }

#endif

    // -------------------------------------------------------------------------
    // Kaku
    // -------------------------------------------------------------------------
    //
    // HardwareManager already decodes the RF packet into a Kaku command and
    // publishes the event.
    //
    // The current EventBus SystemEvent contains the house/button information
    // as:
    //
    //     identifier = "D_2"
    //
    // SceneManager owns the mapping from Kaku -> Scene.
    //
    if (evt.source.equalsIgnoreCase("KAKU"))
    {
        if (evt.identifier.length() >= 3)
        {
            int separator = evt.identifier.indexOf('_');

            if (separator > 0)
            {
                char house = evt.identifier.charAt(0);

                uint8_t button =
                    (uint8_t)evt.identifier.substring(separator + 1).toInt();

                handleKaku(
                    RFCommand{
                        house,
                        button
                    }
                );
            }
        }

        return;
    }

    // -------------------------------------------------------------------------
    // Everything else
    // -------------------------------------------------------------------------
    //
    // Core observes these events for now.
    //
    // They can later be routed to dedicated systems without giving those
    // systems access to HardwareManager.
    //
}


// -----------------------------------------------------------------------------
// Kaku handling
// -----------------------------------------------------------------------------
//
// SceneManager owns the Kaku -> Scene mapping.
//
// Core merely forwards the decoded Kaku input.
//
void Core::handleKaku(const RFCommand& cmd)
{
#if DEBUG_LEVEL >= 2
    Debug::println(
        2,
        "[CORE][KAKU] House " +
        String(cmd.house) +
        " Button " +
        String(cmd.button)
    );
#endif

    sceneManager.handleKaku(
        cmd.house,
        cmd.button
    );
}


// -----------------------------------------------------------------------------
// executeCommand
// -----------------------------------------------------------------------------
//
// This is the critical architecture boundary.
//
// ScriptManager and SceneManager send canonical commands here.
//
// They do not know:
//
//   - LivingColors implementation
//   - IR controller implementation
//   - RF implementation
//   - storage implementation
//   - physical pin assignments
//
// Core translates the canonical command into a HardwareManager call.
//
// Examples:
//
//   ir TV POWER
//   living 1 rgb 255 0 0
//   living 1 hsv 120 255 255
//   delay 1000
//
// If the command is not recognized, return false.
//
bool Core::executeCommand(const String& command)
{
    String cmd = command;

    cmd.trim();

    if (cmd.length() == 0)
    {
        return false;
    }


#if DEBUG_LEVEL >= 3
    Debug::println(
        3,
        "[CORE][COMMAND] " + cmd
    );
#endif


    // -------------------------------------------------------------------------
    // delay
    // -------------------------------------------------------------------------

    if (cmd.startsWith("delay "))
    {
        return executeDelayCommand(cmd);
    }


    // -------------------------------------------------------------------------
    // IR
    // -------------------------------------------------------------------------

    if (cmd.startsWith("ir "))
    {
        return executeIRCommand(cmd);
    }


    // -------------------------------------------------------------------------
    // LivingColors
    // -------------------------------------------------------------------------

    if (cmd.startsWith("living "))
    {
        return executeLivingColorsCommand(cmd);
    }


#if DEBUG_LEVEL >= 1
    Debug::println(
        1,
        "[CORE][COMMAND] Unknown command: " + cmd
    );
#endif

    return false;
}


// -----------------------------------------------------------------------------
// executeDelayCommand
// -----------------------------------------------------------------------------

bool Core::executeDelayCommand(const String& command)
{
    String value = command.substring(6);
    value.trim();

    unsigned long milliseconds =
        (unsigned long)value.toInt();

    delay(milliseconds);

    return true;
}


// -----------------------------------------------------------------------------
// executeIRCommand
// -----------------------------------------------------------------------------
//
// Expected canonical format:
//
//     ir <remote> <button>
//
// Example:
//
//     ir TV POWER
//
// IMPORTANT:
//
// This function is intentionally the only place outside HardwareManager that
// knows how a high-level IR command eventually reaches the IR controller.
//
// The exact HardwareManager/IRController API must match your implementation.
//
bool Core::executeIRCommand(const String& command)
{
    String payload = command.substring(3);
    payload.trim();

    int separator = payload.indexOf(' ');

    if (separator <= 0)
    {
#if DEBUG_LEVEL >= 1
        Debug::println(
            1,
            "[CORE][IR] Invalid command: " + command
        );
#endif
        return false;
    }

    String remote = payload.substring(0, separator);
    String button = payload.substring(separator + 1);

    remote.trim();
    button.trim();

    if (remote.length() == 0 || button.length() == 0)
    {
        return false;
    }


    // -------------------------------------------------------------------------
    // IMPORTANT
    // -------------------------------------------------------------------------
    //
    // Do not put raw IR codes into ScriptManager or SceneManager.
    //
    // The remote/button lookup belongs at the hardware boundary.
    //
    // We need to adapt this one call to the actual IRController API.
    //
    // For now this intentionally returns false rather than guessing an API
    // that may not exist in your current IRController.
    //

#if DEBUG_LEVEL >= 1
    Debug::println(
        1,
        "[CORE][IR] IR command received: " +
        remote +
        " " +
        button
    );
#endif

    return false;
}


// -----------------------------------------------------------------------------
// executeLivingColorsCommand
// -----------------------------------------------------------------------------
//
// Expected formats:
//
//     living <lamp> rgb <r> <g> <b>
//     living <lamp> hsv <h> <s> <v>
//
// Example:
//
//     living 1 rgb 255 0 0
//
// The exact LivingColors API is intentionally isolated here.
//
// If the current LivingColors class uses a different method signature, only
// this function needs to change.
//

bool Core::executeLivingColorsCommand(const String& command)
{
    String payload = command.substring(7);
    payload.trim();

    // -------------------------------------------------------------------------
    // Extract lamp number
    // -------------------------------------------------------------------------

    int firstSpace = payload.indexOf(' ');

    if (firstSpace <= 0)
    {
        return false;
    }

    int lamp =
        payload.substring(0, firstSpace).toInt();

    String values =
        payload.substring(firstSpace + 1);

    values.trim();


    // -------------------------------------------------------------------------
    // Validate public lamp number
    // -------------------------------------------------------------------------
    //
    // Command layer:
    //
    //     living 1 ...
    //     living 12 ...
    //
    // Hardware layer:
    //
    //     0 ... 11
    //
    // Keep that conversion here.
    //

    if (lamp < 1 || lamp > LAMP_COUNT)
    {
#if DEBUG_LEVEL >= 1
        Debug::println(
            1,
            "[CORE][LIVING] Invalid lamp: " +
            String(lamp)
        );
#endif
        return false;
    }

    uint8_t lampIndex =
        (uint8_t)(lamp - 1);


    // -------------------------------------------------------------------------
    // RGB
    // -------------------------------------------------------------------------

    if (values.startsWith("rgb "))
    {
        String rgb =
            values.substring(4);

        rgb.trim();

        int p1 = rgb.indexOf(' ');

        if (p1 <= 0)
        {
            return false;
        }

        int p2 =
            rgb.indexOf(' ', p1 + 1);

        if (p2 <= p1)
        {
            return false;
        }

        String rString =
            rgb.substring(0, p1);

        String gString =
            rgb.substring(p1 + 1, p2);

        String bString =
            rgb.substring(p2 + 1);

        rString.trim();
        gString.trim();
        bString.trim();

        if (rString.length() == 0 ||
            gString.length() == 0 ||
            bString.length() == 0)
        {
            return false;
        }

        int rValue = rString.toInt();
        int gValue = gString.toInt();
        int bValue = bString.toInt();

        if (rValue < 0 || rValue > 255 ||
            gValue < 0 || gValue > 255 ||
            bValue < 0 || bValue > 255)
        {
#if DEBUG_LEVEL >= 1
            Debug::println(
                1,
                "[CORE][LIVING] Invalid RGB values: " +
                rgb
            );
#endif
            return false;
        }

        uint8_t r = (uint8_t)rValue;
        uint8_t g = (uint8_t)gValue;
        uint8_t b = (uint8_t)bValue;


#if DEBUG_LEVEL >= 2
        Debug::println(
            2,
            "[CORE][LIVING] Lamp " +
            String(lamp) +
            " -> index " +
            String(lampIndex) +
            " RGB " +
            String(r) + " " +
            String(g) + " " +
            String(b)
        );
#endif

        hardware.living().setColorRGB(
            lampIndex,
            r,
            g,
            b
        );

        return true;
    }


    // -------------------------------------------------------------------------
    // HSV
    // -------------------------------------------------------------------------

    if (values.startsWith("hsv "))
    {
        String hsv =
            values.substring(4);

        hsv.trim();

        int p1 = hsv.indexOf(' ');

        if (p1 <= 0)
        {
            return false;
        }

        int p2 =
            hsv.indexOf(' ', p1 + 1);

        if (p2 <= p1)
        {
            return false;
        }

        String hString =
            hsv.substring(0, p1);

        String sString =
            hsv.substring(p1 + 1, p2);

        String vString =
            hsv.substring(p2 + 1);

        hString.trim();
        sString.trim();
        vString.trim();

        if (hString.length() == 0 ||
            sString.length() == 0 ||
            vString.length() == 0)
        {
            return false;
        }

        int hValue = hString.toInt();
        int sValue = sString.toInt();
        int vValue = vString.toInt();

        if (hValue < 0 || hValue > 255 ||
            sValue < 0 || sValue > 255 ||
            vValue < 0 || vValue > 255)
        {
#if DEBUG_LEVEL >= 1
            Debug::println(
                1,
                "[CORE][LIVING] Invalid HSV values: " +
                hsv
            );
#endif
            return false;
        }

        uint8_t h = (uint8_t)hValue;
        uint8_t s = (uint8_t)sValue;
        uint8_t v = (uint8_t)vValue;


#if DEBUG_LEVEL >= 2
        Debug::println(
            2,
            "[CORE][LIVING] Lamp " +
            String(lamp) +
            " -> index " +
            String(lampIndex) +
            " HSV " +
            String(h) + " " +
            String(s) +
            " " +
            String(v)
        );
#endif

        hardware.living().setColor(
            lampIndex,
            h,
            s,
            v
        );

        return true;
    }


    // -------------------------------------------------------------------------
    // Unknown LivingColors command
    // -------------------------------------------------------------------------

#if DEBUG_LEVEL >= 1
    Debug::println(
        1,
        "[CORE][LIVING] Unknown LivingColors command: " +
        command
    );
#endif

    return false;
}


// -----------------------------------------------------------------------------
// Password helper
// -----------------------------------------------------------------------------

String Core::generateSecurePassword()
{
    static bool seedInitialized = false;

    if (!seedInitialized)
    {
        randomSeed(millis());
        seedInitialized = true;
    }

    const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";

    const int length = 8;

    String password;

    for (int i = 0; i < length; i++)
    {
        int index =
            random(sizeof(charset) - 1);

        password += charset[index];
    }

    return password;
}

