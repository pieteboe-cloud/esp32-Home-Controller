#pragma once
#include <Arduino.h>
#include <functional>
#include "../Debug/Debug.h"
#include "LivingColors/LivingColors.h"
#include "RF/RFController.h"
#include "KakuDecoder/KakuDecoder.h"
#include "IR/IRController.h"

class HardwareManager {
public:
    HardwareManager(uint8_t lc_cs, uint8_t lc_sck, uint8_t lc_miso, uint8_t lc_mosi, uint8_t lc_gdo2,
                    uint8_t rf_rx, uint8_t rf_tx,
                    uint8_t ir_rx, uint8_t ir_tx);

    void init();
    void update();

    LivingColors& living() { return _living; }
    RFController& rf() { return _rf; }
    KakuDecoder& kaku() { return _kaku; }
    IRController& ir() { return _ir; }

    // Upstream callbacks
    void onRFCommand(RFCallback callback);
    void onIRCommand(IRCallback callback);
    void onKakuCommand(KakuCallback callback);

private:
    LivingColors _living;
    RFController _rf;
    KakuDecoder  _kaku;
    IRController _ir;

    // Upstream callbacks
    KakuCallback _kakuCb;
    RFCallback   _rfCb;
    IRCallback   _irCb;

    void handleRFDecoded(const RFSignal& signal);
    void handleIRDecoded(const IRCommand& cmd);
    
private:
};
