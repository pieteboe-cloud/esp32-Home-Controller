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
#include "AudioController/AudioController.h"

// HardwareManager owns the physical device layer and the filesystem-backed storage
// that feeds the rest of the system. This class is the single owner of RF, IR,
// Kaku, LivingColors, and storage initialization.
class HardwareManager {
public:
    HardwareManager();

    /**
     * Initializes every hardware component and subscribes the callbacks that publish
     * device events into the EventBus.
     */
    void init();

    /**
     * Polls the RF and IR devices so decoded events keep flowing into the system.
     */
    void update();

    /**
     * Accessors for the low-level device controllers.
     */
    LivingColors& living() { return _living; }
    RFController& rf() { return _rf; }
    KakuDecoder& kaku() { return _kaku; }
    IRController& ir() { return _ir; }
    Storage& storage() { return _storage; }
    AudioController& audio() { return _audio; }

    /**
     * Registers external callbacks for incoming RF/IR/Kaku/storage events.
     */
    void onRFCommand(RFCallback callback);
    void onIRCommand(IRCallback callback);
    void onKakuCommand(KakuCallback callback);
    void onStorageEvent(Storage::StorageCallback callback);

private:
    // Physical hardware and storage owners. No other subsystem should initialize these.
    LivingColors _living;
    RFController _rf;
    KakuDecoder  _kaku;
    IRController _ir;
    Storage _storage;
    AudioController _audio;

    // Preserved compatibility callbacks for legacy listeners.
    KakuCallback _kakuCb;
    RFCallback   _rfCb;
    IRCallback   _irCb;

    // Event handlers convert low-level device frames into EventBus messages.
    void handleRFDecoded(const RFSignal& signal);
    void handleIRDecoded(const IRCommand& cmd);
    void handleStorageEvent(const String& event, const String& details);
    void handleAudioBeat();
    void handleAudioSilence(bool isSilent);
};
