#include "HardwareManager.h"

HardwareManager::HardwareManager(uint8_t lc_cs, uint8_t lc_sck, uint8_t lc_miso, uint8_t lc_mosi, uint8_t lc_gdo2,
                                 uint8_t rf_rx,
                                 uint8_t ir_rx, uint8_t ir_tx)
    : _living(lc_cs, lc_sck, lc_miso, lc_mosi, lc_gdo2),
      _rf(rf_rx),
      _kaku(),
      _ir(ir_rx, ir_tx)   // <-- NEW
{
}

void HardwareManager::init() {
#if DEBUG_LEVEL >= 1
    Debug::println("[HW] Init LivingColors...");
#endif
    _living.begin();

#if DEBUG_LEVEL >= 1
    Debug::println("[HW] Init RF433 receiver...");
#endif
    _rf.begin();

#if DEBUG_LEVEL >= 1
    Debug::println("[HW] Wiring RFReceiver -> KakuDecoder...");
#endif
    _rf.onReceive([this](unsigned long value, int bits, int protocol, int pulse) {
        _kaku.onRawData(value, bits, protocol, pulse);
    });

    _kaku.onCommand([this](const RFCommand& cmd) {
        this->handleRFDecoded(cmd);
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
    _rf.update();
    _ir.update();   // poll the IR receiver for new codes
}

void HardwareManager::onRFCommand(KakuCallback callback) {
    _rfCb = callback;
}

void HardwareManager::onIRCommand(IRCallback callback) {
    _irCb = callback;
}

void HardwareManager::handleRFDecoded(const RFCommand& cmd) {
    if (_rfCb) _rfCb(cmd);
}

void HardwareManager::handleIRDecoded(const IRCommand& cmd) {
    if (_irCb) _irCb(cmd);
}
