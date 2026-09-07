#include "HardwareManager.h"
#include "EventBus.h" // 1. Include the new EventBus

// pin definitions are from /HardwareManager/Config.h
HardwareManager::HardwareManager()
    : _living(LC_CS, LC_SCK, LC_MISO, LC_MOSI, LC_GDO2),
      _rf(RF_RX, RF_TX),
      _kaku(),
      _ir(IR_RX, IR_TX),
      _audio(AUDIO_ADC_PIN)
{
}

void HardwareManager::init() {
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing hardware components");
#endif

    // Storage is initialized first because the web UI and config files depend on it.
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing storage manager");
#endif
    if (_storage.begin()) {
#if DEBUG_LEVEL >= 2
        Debug::println(2, "[HW][INFO] Storage initialized successfully");
#endif
        _storage.setCallback([this](const String& event, const String& details) {
            this->handleStorageEvent(event, details);
        });
    } else {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[HW][ERROR] Storage initialization failed");
#endif
    }

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Starting LivingColors controller");
#endif
    _living.begin();

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing RF receiver/transmitter");
#endif
    _rf.init();

    // RF Callback -> Publish Event
    _rf.onCommand([this](const RFSignal& signal) {
        Debug::println(3, "[HW][RF] Incoming RF command callback registered");
        this->handleRFDecoded(signal);
        // Pass to Kaku decoder (Kaku is a specific protocol of RF)
        _kaku.onRawData(signal.value, signal.bits, signal.protocol, signal.pulse);
    });

    // Kaku Callback -> Publish Event
    _kaku.onCommand([this](const RFCommand& cmd) {
        // Emit Kaku event to EventBus with format "house_button" ie. "A_1"
        SystemEvent evt;
        evt.source = "KAKU";
        evt.identifier = String(cmd.house) + "_" + String(cmd.button);
        evt.rawData = "house=" + String(cmd.house) + ",button=" + String(cmd.button);

        Debug::println(3, "[HW][KAKU] Decoded Kaku event: " + evt.identifier);
        EventBus::getInstance().publish(evt);

        // Legacy callback support
        if (_kakuCb) _kakuCb(cmd);
    });

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing IR receiver/transmitter");
#endif
    _ir.init();

    // IR Callback -> Publish Event
    _ir.onCommand([this](const IRCommand& cmd) {
        Debug::println(3, "[HW][IR] Incoming IR command callback registered");
        this->handleIRDecoded(cmd);
    });

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing audio controller");
#endif
    _audio.init();

    // Audio Callbacks -> Publish Events
    _audio.onBeat([this]() {
        this->handleAudioBeat();
    });

    _audio.onSilence([this](bool isSilent) {
        this->handleAudioSilence(isSilent);
    });

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INFO] HardwareManager initialization complete");
#endif
}

void HardwareManager::update() {
    // RF and IR updates must be polled regularly so raw packets are decoded in time.
    _rf.update();
    _ir.update();
    // Audio update must also be polled for beat/silence detection.
    _audio.update();
}

// --- Callback Setters (Kept for compatibility, but mostly unused now) ---
void HardwareManager::onRFCommand(RFCallback callback) { _rfCb = callback; }
void HardwareManager::onIRCommand(IRCallback callback) { _irCb = callback; }
void HardwareManager::onKakuCommand(KakuCallback callback) { _kakuCb = callback; }
void HardwareManager::onStorageEvent(Storage::StorageCallback callback) {
    _storage.setCallback([this, callback](const String& event, const String& details) {
        callback(event, details);
    });
}

void HardwareManager::handleRFDecoded(const RFSignal& signal) {
    Debug::println(4,"[HW][DEBUG] Received RF signal: 0x" + String(signal.value, HEX));

    // 1. Create the Event using RAW data (House/Button unknown here)
    SystemEvent evt;
    evt.source = "RF";
    // Use Raw Value as identifier.
    // Example: "RF_RAW_123456"
    evt.identifier = "RF_RAW_" + String(signal.value, HEX);
    evt.rawData = String(signal.value, HEX);

    // 2. Publish to Core/Translator
    #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println(4,"[HW][handleRFDecoder] Publishing RF event to EventBus...");
        #endif
    #endif
    EventBus::getInstance().publish(evt);

    // 3. IMPORTANT: Still pass raw data to KakuDecoder so it can decode House/Button
    // The KakuDecoder will then trigger its OWN callback/event with the detailed info.
    _kaku.onRawData(signal.value, signal.bits, signal.protocol, signal.pulse);
}

void HardwareManager::handleIRDecoded(const IRCommand& cmd) {
    String codeHex = String(cmd.code, HEX);
    codeHex.toLowerCase();
    while (codeHex.length() < 8) codeHex = "0" + codeHex; // left-pad to 8 hex digits
    if (codeHex.length() > 8) codeHex = codeHex.substring(codeHex.length() - 8);

    Debug::println(4,"[HW][DEBUG] Received IR code: 0x" + codeHex);

    // 1. Create the Event
    SystemEvent evt;
    evt.source = "IR";
    evt.identifier = "IR_RAW_" + codeHex; // Translator will look this up
    evt.rawData = "0x" + codeHex;

    // 2. Publish to Core/Translator
    #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 1
            Debug::println("[HW][handleIRDecoded] Publishing IR event to EventBus...");
        #endif
    #endif

    EventBus::getInstance().publish(evt);

    // 3. (Optional) Keep old callback
    // if (_irCb) _irCb(cmd);
}

void HardwareManager::handleStorageEvent(const String& event, const String& details) {
    #if DEBUG_LEVEL >= 2
        Debug::println(3,"[HW][INFO] Storage event: " + event);
    #endif

    // Publish storage events too (e.g., "CONFIG_SAVED")
    SystemEvent evt;
    evt.source = "STORAGE";
    evt.identifier = event;
    evt.rawData = details;
    #ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 2
            Debug::println("[HW][handleStorageEvent] Publishing storage event to EventBus...");
        #endif
    #endif

    EventBus::getInstance().publish(evt); // Publish to Core/Translator
}
void HardwareManager::handleAudioBeat() {
    // Beat detected: publish event to EventBus
    SystemEvent evt;
    evt.source = "AUDIO";
    evt.identifier = "BEAT";
    evt.rawData = String(_audio.getCurrentEnergy(), 2) + "," + String(_audio.getCurrentBPM(), 1);  // Include energy and BPM for debugging

    // LED flashed in AudioController

    EventBus::getInstance().publish(evt);
    
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][AUDIO] Beat detected - Energy: " + String(_audio.getCurrentEnergy(), 2) + ", BPM: " + String(_audio.getCurrentBPM(), 1));
#endif
}

void HardwareManager::handleAudioSilence(bool isSilent) {
    // Silence state changed: publish event to EventBus
    SystemEvent evt;
    evt.source = "AUDIO";
    evt.identifier = isSilent ? "SILENCE_START" : "SILENCE_END";
    evt.rawData = isSilent ? "silent" : "audio_resumed";

    EventBus::getInstance().publish(evt);
    
#if DEBUG_LEVEL >= 2
    if (isSilent) {
        Debug::println(2, "[HW][AUDIO] Silence detected");
    } else {
        Debug::println(2, "[HW][AUDIO] Audio resumed");
    }
#endif
}
