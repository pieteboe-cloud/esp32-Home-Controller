#pragma once
#include <Arduino.h>
#include <functional>
#include "../Debug/Debug.h"
#include "LivingColors/LivingColors.h"
#include "RCSwitch/RFReceiver.h"
#include "KakuDecoder/KakuDecoder.h"
#include "IR/IRController.h"     // <-- NEW

class HardwareManager {
public:
    HardwareManager(uint8_t lc_cs, uint8_t lc_sck, uint8_t lc_miso, uint8_t lc_mosi, uint8_t lc_gdo2,
                    uint8_t rf_rx,
                    uint8_t ir_rx, uint8_t ir_tx);   // <-- NEW

    void init();
    void update();

    LivingColors& living() { return _living; }
    RFReceiver& rf() { return _rf; }
    KakuDecoder& kaku() { return _kaku; }
    IRController& ir() { return _ir; }                // <-- NEW

    // Upstream callbacks
    void onRFCommand(KakuCallback callback);
    void onIRCommand(IRCallback callback);            // <-- NEW

private:
    LivingColors _living;
    RFReceiver   _rf;
    KakuDecoder  _kaku;
    IRController _ir;                                 // <-- NEW

    // Upstream callbacks
    KakuCallback _rfCb;
    IRCallback   _irCb;                               // <-- NEW

    void handleRFDecoded(const RFCommand& cmd);
    void handleIRDecoded(const IRCommand& cmd);       // <-- NEW
};
