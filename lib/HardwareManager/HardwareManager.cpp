#include "HardwareManager.h"

HardwareManager::HardwareManager(uint8_t lc_cs, uint8_t lc_sck, uint8_t lc_miso, uint8_t lc_mosi, uint8_t lc_gdo2,
                                 uint8_t rf_rx, uint8_t rf_tx,
                                 uint8_t ir_rx, uint8_t ir_tx)
    : _living(lc_cs, lc_sck, lc_miso, lc_mosi, lc_gdo2),
      _rf(rf_rx, rf_tx),
      _kaku(),
      _ir(ir_rx, ir_tx)
{
}

void HardwareManager::init() {
#if DEBUG_LEVEL >= 1
    Debug::println("[HW] Init LivingColors...");
#endif
    _living.begin();

#if DEBUG_LEVEL >= 1
    Debug::println("[HW] Init RF Controller...");
#endif
    _rf.init();

    // RF → HardwareManager → Core
    Debug::println("[HW][DEBUG] Setting up RF callback in HardwareManager");
    _rf.onCommand([this](const RFSignal& signal) {
        Debug::println("[HW][DEBUG] RF callback triggered in HardwareManager");
        this->handleRFDecoded(signal);
        
        // Also pass to Kaku decoder for Kaku-specific processing
        Debug::println("[HW][DEBUG] Passing RF signal to Kaku decoder");
        _kaku.onRawData(signal.value, signal.bits, signal.protocol, signal.pulse);
    });
    
    // Kaku → HardwareManager → Core
    _kaku.onCommand([this](const RFCommand& cmd) {
        if (_kakuCb) {
            _kakuCb(cmd);
        }
    });

#if DEBUG_LEVEL >= 1
    Debug::println("[HW] Init IR Controller...");
#endif
    _ir.init();

    // IR → HardwareManager → Core
    _ir.onCommand([this](const IRCommand& cmd) {
        this->handleIRDecoded(cmd);
    });

#if DEBUG_LEVEL >= 1
    Debug::println("[HW] HardwareManager ready.");
#endif
}

void HardwareManager::update() {
   // Debug::println("[HW][DEBUG] Calling HardwareManager::update()");
    _rf.update();
   // Debug::println("[HW][DEBUG] Calling IRController::update()");
    _ir.update();   // poll the IR receiver for new codes
}

void HardwareManager::onRFCommand(RFCallback callback) {
    _rfCb = callback;
}

void HardwareManager::onIRCommand(IRCallback callback) {
    _irCb = callback;
}

void HardwareManager::onKakuCommand(KakuCallback callback) {
    _kakuCb = callback;
}

void HardwareManager::handleRFDecoded(const RFSignal& signal) {
    Debug::println("[HW][DEBUG] Received RF signal in HardwareManager with value: 0x" + String(signal.value, HEX));
    if (_rfCb) {
        Debug::println("[HW][DEBUG] Calling RF callback in HardwareManager");
        _rfCb(signal);
    } else {
        Debug::println("[HW][DEBUG] No RF callback set in HardwareManager");
    }
}

void HardwareManager::handleIRDecoded(const IRCommand& cmd) {
    if (_irCb) _irCb(cmd);
}




