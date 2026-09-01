#include "RFReceiver.h"

RFReceiver::RFReceiver(uint8_t rxPin)
  : _rxPin(rxPin) {
}

void RFReceiver::begin() {
#ifdef DEBUG_LEVEL
#if DEBUG_LEVEL >= 1
  Debug::println("[RFReceiver] begin() Pin " + String(_rxPin));
#endif
#endif
  pinMode(_rxPin, INPUT);
  _rcSwitch.enableReceive(_rxPin);
}

void RFReceiver::onReceive(RFReceiveCallback callback) {
  _callback = callback;
}

void RFReceiver::update() {
#if DEBUG_LEVEL >= 3
  //Debug::println("[RF] update() polling ...");
#endif
  if (_rcSwitch.available()) {
    unsigned long value   = _rcSwitch.getReceivedValue();
    int bits              = _rcSwitch.getReceivedBitlength();
    int protocol          = _rcSwitch.getReceivedProtocol();
    int pulse             = _rcSwitch.getReceivedDelay();

#if DEBUG_LEVEL >= 3
    Debug::println("[RF] RAW received value=0x" + String(value, HEX) +
                   " bits=" + String(bits) +
                   " protocol=" + String(protocol) +
                   " pulse=" + String(pulse) + "us");
#endif

    if (value != 0 && _callback) {
#if DEBUG_LEVEL >= 3
      Debug::println("[RF] Forwarding raw data to callback");
#endif
      _callback(value, bits, protocol, pulse);
    } else {
#if DEBUG_LEVEL >= 3
      Debug::println("[RF] value==0 or no callback, ignored");
#endif
    }

    _rcSwitch.resetAvailable();
  }
}
