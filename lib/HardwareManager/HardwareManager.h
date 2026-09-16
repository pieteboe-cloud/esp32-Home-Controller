#pragma once
#include <Arduino.h>
#include "Config.h"
#include <functional>
#include "Debug.h"
#include "../HardwareManager/Storage/StorageManager.h"

#include "LivingColors/LivingColors.h"
#include "RF/RFController.h"
#include "KakuDecoder/KakuDecoder.h"
#include "IR/IRController.h"
#include "AudioController/AudioController.h"

#include "../Core/EventBus.h"



// Restore precise legacy types
typedef std::function<void(const RFSignal&)> RFCallback;
typedef std::function<void(const IRCommand&)> IRCallback;
typedef std::function<void(const RFCommand&)> KakuCallback;

class HardwareManager {
public:
    HardwareManager();

    /**
     * Initializes every physical hardware component.
     * Storage initialization is omitted here since it is now centralized in Core::init().
     */
    void init();

    /**
     * Polls the active RF, IR, and Audio controllers inside the main loop.
     */
    void update();

    /**
     * Controlled low-level accessors for physical peripherals.
     */
    LivingColors& living() { return _living; }
    RFController& rf() { return _rf; }
    KakuDecoder& kaku() { return _kaku; }
    IRController& ir() { return _ir; }
    AudioController& audio() { return _audio; }

    /**
     * Callbacks kept for legacy implementation compatibility.
     */
    void onRFCommand(RFCallback callback);
    void onIRCommand(IRCallback callback);
    void onKakuCommand(KakuCallback callback);

private:
    // Low-level hardware drivers (No Storage instance owned here anymore)
    LivingColors _living;
    RFController _rf;
    KakuDecoder  _kaku;
    IRController _ir;
    AudioController _audio;

    // Backwards compatibility legacy callbacks
    KakuCallback _kakuCb;
    RFCallback   _rfCb;
    IRCallback   _irCb;

    // Event handlers mapping frames into EventBus messages
    void handleRFDecoded(const RFSignal& signal);
    void handleIRDecoded(const IRCommand& cmd);
    void handleKakuDecoded(const RFCommand& cmd);
    void handleAudioBeat();
    void handleAudioSilence(bool isSilent);
};
