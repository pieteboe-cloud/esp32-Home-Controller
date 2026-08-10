#include "TempCore.h"
#include "../Debug/Debug.h"

TempCore::TempCore(HardwareManager& hw)
    : hardware(hw)
{
}

void TempCore::init() {
    // Register the Kaku (RF433) callback
    hardware.onRFCommand([this](const RFCommand& cmd) {
        this->handleKaku(cmd);
    });

// Register the IR callback: received IR code -> core (store + schedule echo)
    hardware.onIRCommand([this](const IRCommand& cmd) {
        Debug::println("[CORE] IR received: 0x" + String(cmd.code, HEX));

        // Schedule the echo after 2 seconds (store the full command incl. raw timings)
        _echoTime = millis() + 2000;
        _echoCmd  = cmd;
    });
}

void TempCore::update() {
    // Poll hardware (RF + IR)
    hardware.update();

    // Echo the IR code after the 2s delay
    if (_echoTime && (long)(millis() - _echoTime) >= 0) {
        Debug::println("[CORE] Echoing IR code: 0x" + String(_echoCmd.code, HEX));
        hardware.ir().echo(_echoCmd);

        _echoTime = 0;
        _echoCmd  = IRCommand();   // reset to an empty command
    }
}

void TempCore::handleKaku(const RFCommand& cmd) {
#if DEBUG_LEVEL >= 2
    Debug::println("[KAKU] House " + String(cmd.house) +
                   " Button " + String(cmd.button));
#endif

    if (cmd.button == 2) {
#if DEBUG_LEVEL >= 1
        Debug::println("[CORE] Button 2 → BLUE");
#endif
        for (int i = 0; i < 12; i++) {
            hardware.living().setColor(i, 160, 255, 255);   // BLUE
        }
    }
}
