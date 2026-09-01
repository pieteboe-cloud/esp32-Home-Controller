#pragma once
#include <Arduino.h>
#include "Config.h"
#include <functional>
#include "Debug.h"
#include "LivingColors/LivingColors.h"
#include "RF/RFController.h"
#include "KakuDecoder/KakuDecoder.h"
#include "IR/IRController.h"
#include "Storage/StorageManager.h"

class HardwareManager {
public:
    HardwareManager();

    void init();
    void update();

    LivingColors& living() { return _living; }
    RFController& rf() { return _rf; }
    KakuDecoder& kaku() { return _kaku; }
    IRController& ir() { return _ir; }
    Storage& storage() { return _storage; }
    
  

    // Upstream callbacks
    void onRFCommand(RFCallback callback);
    void onIRCommand(IRCallback callback);
    void onKakuCommand(KakuCallback callback);
    void onStorageEvent(Storage::StorageCallback callback);

private:
    // Hardware components
    LivingColors _living;
    RFController _rf;
    KakuDecoder  _kaku;
    IRController _ir;
    Storage _storage;

    // Upstream callbacks
    KakuCallback _kakuCb;
    RFCallback   _rfCb;
    IRCallback   _irCb;

    // Event handlers
    void handleRFDecoded(const RFSignal& signal);
    void handleIRDecoded(const IRCommand& cmd);
    
    // Storage event handler
    void handleStorageEvent(const String& event, const String& details);
    
private:
};
