#pragma once
#include <Arduino.h>
#include "../HardwareManager/HardwareManager.h"

class TempCore {
public:
    TempCore(HardwareManager& hw);

    void init();
    void update();

private:
    HardwareManager& hardware;

    // --- RF test routine variables ---
    RFSignal  testRf;
    unsigned long testRfEchoTime = 0;
    bool testRfWaiting = false;

    // --- IR test routine variables ---
    IRCommand testIr;
    unsigned long testIrEchoTime = 0;
    bool testIrWaiting = false;

    // --- Real echo variables (your existing system) ---
    unsigned long _rfEchoTime = 0;
    RFSignal      _rfEchoCmd;

    unsigned long _irEchoTime = 0;
    IRCommand     _irEchoCmd;

    void handleKaku(const RFCommand& cmd);
};
