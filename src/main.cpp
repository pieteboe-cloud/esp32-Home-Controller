#include "Arduino.h"
#include "Debug.h"
#include "HardwareManager.h"
#include "TempCore.h"

HardwareManager hw(
    5,18,19,23,22,   // LivingColors CC2500
    33,26,           // RF433 receiver, RF433 transmitter
    35,25            // IR RX, IR TX
);

TempCore core(hw);

void setup() {
    Debug::init();
    hw.init();
    core.init();
}

void loop() {
    core.update();
}
