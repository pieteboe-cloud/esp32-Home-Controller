#ifndef RFReceiver_h
#define RFReceiver_h

#include <Arduino.h>
#include <RCSwitch.h>
#include <functional>

#ifdef DEBUG_LEVEL
#include "../../Debug/Debug.h"
#endif

// Callback type: delivers the raw data received by the rc-switch library.
typedef std::function<void(unsigned long value, int bits, int protocol, int pulse)> RFReceiveCallback;

// RFReceiver owns the rc-switch library instance, starts the receiver,
// and forwards every raw signal it receives to a registered callback.
class RFReceiver {
  public:
    RFReceiver(uint8_t rxPin);

    // Start the rc-switch receiver on the given pin (call in setup()).
    void begin();

    // Poll for received signals (call in loop()).
    void update();

    // Register the callback that receives raw RF data.
    void onReceive(RFReceiveCallback callback);

  private:
    uint8_t _rxPin;
    RFReceiveCallback _callback;
    RCSwitch _rcSwitch;
};

#endif
