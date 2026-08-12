#include "TempCore.h"
#include "../Debug/Debug.h"

TempCore::TempCore(HardwareManager& hw)
    : hardware(hw)
{
}

void TempCore::init() {

    // ----------------------------------------------------
    // RF callback → store first RF code for test routine
    // ----------------------------------------------------
    hardware.onRFCommand([this](const RFSignal& signal) {

        Debug::println("[CORE][TEST][RF] Received RF: 0x" + String(signal.value, HEX));

        if (!testRf.value) {
            testRf = signal;
            Debug::println("[CORE][TEST][RF] Stored first RF code");
        }
    });

    // ----------------------------------------------------
    // Kaku decoded callback
    // ----------------------------------------------------
    hardware.onKakuCommand([this](const RFCommand& cmd) {
        this->handleKaku(cmd);
    });

    // ----------------------------------------------------
    // IR callback → store first IR code for test routine
    // ----------------------------------------------------
    hardware.onIRCommand([this](const IRCommand& cmd) {

        Debug::println("[CORE][TEST][IR] Received IR: 0x" + String(cmd.code, HEX));

        if (!testIr.code) {
            testIr = cmd;
            Debug::println("[CORE][TEST][IR] Stored first IR code");
        }
    });
}

void TempCore::update() {

    hardware.update();

    // ----------------------------------------------------
    // RF TEST ROUTINE (independent)
    // ----------------------------------------------------

    // Start RF timer when first RF code arrives
    if (!testRfWaiting && testRf.value) {
        testRfEchoTime = millis() + 2000;
        testRfWaiting = true;
        Debug::println("[CORE][TEST][RF] RF echo scheduled in 2 seconds");
    }

    // Echo RF when timer expires
    if (testRfWaiting && millis() >= testRfEchoTime) {

        Debug::println("[CORE][TEST][RF] Echoing RF: 0x" + String(testRf.value, HEX));
        Debug::println("[CORE][TEST][RF] Protocol=" + String(testRf.protocol) +
                       " Bits=" + String(testRf.bits) +
                       " Pulse=" + String(testRf.pulse));

        hardware.rf().disableReceive();
        hardware.rf().setProtocol(testRf.protocol);
        hardware.rf().setPulseLength(testRf.pulse);
        hardware.rf().send(testRf.value, testRf.bits);
        hardware.rf().enableReceive();

        Debug::println("[CORE][TEST][RF] RF echo complete");

        // Reset RF test routine
        testRf = RFSignal();
        testRfWaiting = false;
        testRfEchoTime = 0;
    }

    // ----------------------------------------------------
    // IR TEST ROUTINE (independent)
    // ----------------------------------------------------

    // Start IR timer when first IR code arrives
    if (!testIrWaiting && testIr.code) {
        testIrEchoTime = millis() + 2000;
        testIrWaiting = true;
        Debug::println("[CORE][TEST][IR] IR echo scheduled in 2 seconds");
    }

    // Echo IR when timer expires
    if (testIrWaiting && millis() >= testIrEchoTime) {

        Debug::println("[CORE][TEST][IR] Echoing IR: 0x" + String(testIr.code, HEX));

        hardware.ir().send(testIr);

        Debug::println("[CORE][TEST][IR] IR echo complete");

        // Reset IR test routine
        testIr = IRCommand();
        testIrWaiting = false;
        testIrEchoTime = 0;
    }

    // ----------------------------------------------------
    // REAL RF echo logic (your existing system)
    // ----------------------------------------------------
    if (_rfEchoTime && (long)(millis() - _rfEchoTime) >= 0) {

        Debug::println("[CORE][DEBUG][RF] Echoing RF code: 0x" + String(_rfEchoCmd.value, HEX));

        if (_rfEchoCmd.value != 0) {
            hardware.rf().disableReceive();
            hardware.rf().setProtocol(_rfEchoCmd.protocol);
            hardware.rf().setPulseLength(_rfEchoCmd.pulse);
            hardware.rf().send(_rfEchoCmd.value, _rfEchoCmd.bits);
            hardware.rf().enableReceive();
        }

        _rfEchoTime = 0;
        _rfEchoCmd = RFSignal();
    }

    // ----------------------------------------------------
    // REAL IR echo logic (your existing system)
    // ----------------------------------------------------
    if (_irEchoTime && (long)(millis() - _irEchoTime) >= 0) {

        Debug::println("[CORE][DEBUG][IR] Echoing IR code: 0x" + String(_irEchoCmd.code, HEX));

        if (_irEchoCmd.code != 0) {
            hardware.ir().send(_irEchoCmd);
        }

        _irEchoTime = 0;
        _irEchoCmd = IRCommand();
    }
}

void TempCore::handleKaku(const RFCommand& cmd) {
#if DEBUG_LEVEL >= 2
    Debug::println("[KAKU] House " + String(cmd.house) +
                   " Button " + String(cmd.button));
#endif

    if (cmd.button == 2) {
        Debug::println("[CORE] Button 2 → BLUE");
        for (int i = 0; i < 12; i++) {
            hardware.living().setColor(i, 160, 255, 255);
        }
    }
}
