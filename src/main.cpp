#include <Arduino.h>
#include <IRremote.hpp> // Primary implementation: ONLY in main.cpp!
#include <RF433.h>
#include <IRController.h>
#include "WebUI.h"
#include "Debug.h"
#include <AutomationManager.h>
#include <SettingsManager.h>
#include "CC2500.h"
#include "ControlLivingColors.h"
#include "LampManager.h"
#include <LampTypes.h>
#include <HardwareConfig.h>


// Pin setup used sofar 33 26 35 25 2 4 5 18 19 23 27 14
RF433 rf(PIN_RF433_RX, PIN_RF433_TX);//   33 26 35 25 2 4 5
IRController ir(PIN_IR_RX, PIN_IR_TX);
CC2500 radio(PIN_CC2500_CS, PIN_CC2500_SCK, PIN_CC2500_MISO, PIN_CC2500_MOSI);
ControlLivingColors control(radio);
WebUI webInterface;
AutomationManager automation(rf, ir);
SettingsManager settingsManager;
LampManager lampManager;

// Global System Timers
unsigned long ledTimer = 0;
unsigned long runningLedTimer = 0;
bool runningLedState = HIGH;

// Debouncing for RF/IR signals
static unsigned long lastRFCode = 0;
static unsigned long lastRFTime = 0;
static unsigned long lastIRCode = 0;
static unsigned long lastIRTime = 0;

void pulseLed(int ms) {
    digitalWrite(PIN_STATUS_LED, HIGH);
    ledTimer = millis() + ms;
}

static void initializeHardware() {
    radio.begin();
    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(PIN_RUNNING_LED, OUTPUT);
    digitalWrite(PIN_RUNNING_LED, runningLedState);
    digitalWrite(PIN_STATUS_LED, HIGH);
    rf.init();
    ir.init();
}

static void initializeModules() {
    if (!lampManager.init(&control, &settingsManager, SettingsManager::lampAddr)) {
        Debug::println("[ERROR][MAIN] LampManager failed to initialize");
    }

    automation.setLampController(&control, SettingsManager::lampAddr);
    automation.initDefaultScenes();
    automation.initDefaultTriggers();
}

static void initializeWebUI() {
    webInterface.init("Home-Controller-AP", [](String command) {
        if (command == "pulse") {
            pulseLed(100);
        }
    });

    webInterface.setControllers(&control, &automation, &settingsManager, &lampManager);
    webInterface.setIRController(&ir);
    webInterface.setStatusProvider([]() {
        return runningLedState == LOW;
    });
    webInterface.setRebootCallback([]() {
        settingsManager.reboot();
    });
    webInterface.setSettingsCallback([]() -> String {
        return settingsManager.loadSettingsWithPresetSummary(lampManager.getPresets());
    });
}

static void updateStatusLED() {
    if (ledTimer != 0 && (long)(millis() - ledTimer) >= 0) {
        digitalWrite(PIN_STATUS_LED, LOW);
        ledTimer = 0;
    }

    if (millis() - runningLedTimer >= 500) {
        runningLedState = !runningLedState;
        digitalWrite(PIN_RUNNING_LED, runningLedState);
        runningLedTimer = millis();
    }
}

static void pollRemoteSignals() {
    unsigned long received = rf.receive();
    if (received != 0) {
        if (received != lastRFCode || (millis() - lastRFTime) > 500) {
            pulseLed(100);
            Debug::print("[INFO][MAIN] RF Received: ");
            Debug::print(received);
            Debug::print(" / ");
            Debug::print(rf.getReceivedBitlength());
            Debug::print("bit / Protocol: ");
            Debug::println(rf.getReceivedProtocol());

            automation.checkTrigger(received, rf.getReceivedBitlength(), rf.getReceivedProtocol());
            lastRFCode = received;
            lastRFTime = millis();
        }
    }

    unsigned long receivedIR = ir.receive();
    if (receivedIR != 0) {
        if (receivedIR != lastIRCode || (millis() - lastIRTime) > 500) {
            pulseLed(100);
            Debug::print("[INFO] IR Received: 0x");
            Debug::print(String(receivedIR, HEX));
            Debug::print(" / ");
            Debug::print(ir.getReceivedBitlength());
            Debug::print("bit / Protocol: ");
            Debug::println(ir.getReceivedProtocol());

            lastIRCode = receivedIR;
            lastIRTime = millis();
        }
    }
}

void setup() {
    Debug::setVerbose(true);
    Debug::init();

    initializeHardware();
    initializeModules();
    initializeWebUI();

    Debug::println("[INFO][MAIN] System Online: RF433, IR, and 11-Lamp Controller ready.");
    Debug::setVerbose(false);
}

void loop() {
    webInterface.handle();
    lampManager.update();
    updateStatusLED();
    pollRemoteSignals();
}
