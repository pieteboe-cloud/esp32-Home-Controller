#pragma once
#include <Arduino.h>
#include "../HardwareManager/HardwareManager.h"

class TempCore {
public:
    TempCore(HardwareManager& hw);

    void init();
    void update();

private:
    HardwareManager& hardware;   // member reference to the hardware manager

    void handleKaku(const RFCommand& cmd);

// IR echo scheduling
    unsigned long _echoTime = 0;   // when to send the echo (millis)
    IRCommand _echoCmd;            // full command (with raw timings) to echo back
};
