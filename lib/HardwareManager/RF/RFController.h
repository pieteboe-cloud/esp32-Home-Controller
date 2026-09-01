#ifndef RF_CONTROLLER_H
#define RF_CONTROLLER_H

#include <Arduino.h>
#include <RCSwitch.h>
#include "../../Debug/Debug.h"
#include <functional>

// ============================================================================
//  RFController - wraps the RCSwitch library for this project.
//
//  PURPOSE
//  --------
//  Provide a clean interface to receive and transmit RF signals, with the
//  special ability to "echo" (replay) a received signal after a delay.
// ============================================================================

// ----------------------------------------------------------------------------
// RFSignal
//   Everything about one received RF signal.
//   - 'value/bits/protocol/pulse' : the raw data from the RCSwitch library.
// ----------------------------------------------------------------------------
struct RFSignal {
  unsigned long value;     // decoded raw data
  int bits;               // number of data bits
  int protocol;           // protocol from the RCSwitch library
  int pulse;              // pulse timing in microseconds
  uint32_t timestamp;     // millis() when the signal arrived
};

// Callback signature. Called on the main loop whenever a valid RF frame arrives.
typedef std::function<void(const RFSignal&)> RFCallback;

// ----------------------------------------------------------------------------
// RFController
// ----------------------------------------------------------------------------
class RFController
{
public:
    // rxPin: input from the RF receiver module.
    // txPin: output that drives the RF transmitter.
    RFController(int rxPin, int txPin);

    // Begin the RF receiver + sender. Call once from setup().
    void init();

    // Poll the receiver for new data and fire the callback if a code arrived.
    // Call this every loop() iteration.
    void update();

    // Transmit RF signals directly
    void send(unsigned long value, int bits);
    void setProtocol(int protocol);
    void setPulseLength(int pulse);
    void enableReceive();
    void disableReceive();

    // Register the function invoked by update() when a new code is received.
    void onCommand(RFCallback callback);

private:
    int rxPin;                            // RF receiver module data pin
    int txPin;                            // RF transmitter module data pin

    RCSwitch* _rfSwitch;                  // RCSwitch instance
    RFCallback _callback;                 // user-supplied handler



    // Simple debounce: ignore the same code arriving again within this window,
    // so a remote that repeats its frame while held down doesn't spam us.
    unsigned long _lastCodeValue;
    unsigned long _lastCodeTime;
    unsigned long _debounceMs;
};

#endif
