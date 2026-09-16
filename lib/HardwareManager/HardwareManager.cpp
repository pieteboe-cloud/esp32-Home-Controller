#include "HardwareManager.h"
// Pins definitions map exactly to your project definitions
HardwareManager::HardwareManager()
    : _living(LC_CS, LC_SCK, LC_MISO, LC_MOSI, LC_GDO2),
      _rf(RF_RX, RF_TX),
      _kaku(),
      _ir(IR_RX, IR_TX),
      _audio(AUDIO_ADC_PIN),
      _kakuCb(nullptr), 
      _rfCb(nullptr), 
      _irCb(nullptr)
      
{
}

void HardwareManager::init() {
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing hardware components");
    Debug::println(2, "[HW][INIT] Initializing storage system");
#endif
    
    // Initialize storage first - this will mount LittleFS and create standard config files
    if (!Storage::begin()) {
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

    // Map RF signals into the local callback system and Kaku decoder
    _rf.onCommand([this](const RFSignal& signal) {
#if DEBUG_LEVEL >= 3
        Debug::println(3, "[HW][RF] Incoming RF command callback registered");
#endif
        this->handleRFDecoded(signal);
    });

    // Map Kaku outputs into the system EventBus
    _kaku.onCommand([this](const RFCommand& cmd) {
        this->handleKakuDecoded(cmd);
    });

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing IR receiver/transmitter");
#endif
    _ir.init();

    _ir.onCommand([this](const IRCommand& cmd) {
#if DEBUG_LEVEL >= 3
        Debug::println(3, "[HW][IR] Incoming IR command callback registered");
#endif
        this->handleIRDecoded(cmd);
    });

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][INIT] Initializing audio controller");
#endif
    _audio.init();

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
    _rf.update();
    _ir.update();
    _audio.update();
}

void HardwareManager::handleRFDecoded(const RFSignal& signal) {
#if DEBUG_LEVEL >= 4
    Debug::println(4, "[HW][DEBUG] Received RF signal: 0x" + String(signal.value, HEX));
#endif

    SystemEvent evt;
    evt.source = "RF";
    evt.sourceType = EventSource::RF;
    evt.identifier = "RF_RAW_" + String(signal.value, HEX);
    evt.rawData = String(signal.value, HEX);

    EventBus::getInstance().publish(evt);

    // Pass the raw data packet straight to the Kaku decoder sub-routine
    _kaku.onRawData(signal.value, signal.bits, signal.protocol, signal.pulse);
    
    if (_rfCb) _rfCb(signal);
}

void HardwareManager::handleIRDecoded(const IRCommand& cmd) {
    String codeHex = String(cmd.code, HEX);
    codeHex.toLowerCase();
    while (codeHex.length() < 8) codeHex = "0" + codeHex; 
    if (codeHex.length() > 8) codeHex = codeHex.substring(codeHex.length() - 8);

#if DEBUG_LEVEL >= 4
    Debug::println(4, "[HW][DEBUG] Received IR code: 0x" + codeHex);
#endif

    SystemEvent evt;
    evt.source = "IR";
    evt.sourceType = EventSource::IR;
    evt.identifier = "IR_RAW_" + codeHex;
    evt.rawData = "0x" + codeHex;

    EventBus::getInstance().publish(evt);
    if (_irCb) _irCb(cmd);
}

void HardwareManager::handleKakuDecoded(const RFCommand& cmd) {
    SystemEvent evt;
    evt.source = "KAKU";
    evt.sourceType = EventSource::Kaku;
    evt.identifier = String(cmd.house) + "_" + String(cmd.button);
    evt.rawData = "house=" + String(cmd.house) + ",button=" + String(cmd.button);

#if DEBUG_LEVEL >= 3
    Debug::println(3, "[HW][KAKU] Decoded Kaku event: " + evt.identifier);
#endif

    EventBus::getInstance().publish(evt);

    if (_kakuCb) _kakuCb(cmd);
}

void HardwareManager::handleAudioBeat() {
    SystemEvent evt;
    evt.source = "AUDIO";
    evt.identifier = "BEAT";
    evt.rawData = String(_audio.getCurrentEnergy(), 2) + "," + String(_audio.getCurrentBPM(), 1);

    EventBus::getInstance().publish(evt);
    
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[HW][AUDIO] Beat detected - Energy: " + String(_audio.getCurrentEnergy(), 2) + ", BPM: " + String(_audio.getCurrentBPM(), 1));
#endif
}

void HardwareManager::handleAudioSilence(bool isSilent) {
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

void HardwareManager::onRFCommand(RFCallback callback) { _rfCb = callback; }
void HardwareManager::onIRCommand(IRCallback callback) { _irCb = callback; }
void HardwareManager::onKakuCommand(KakuCallback callback) { _kakuCb = callback; }
