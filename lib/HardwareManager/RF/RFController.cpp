
#include "RFController.h"

// ============================================================================
//  RFController.cpp - RF receiver/transmitter implementation.
//
//  Uses RC-Switch library. Stores all parameters for accurate replay.
//  Debouncing (300ms) prevents button-hold from flooding events.
// ============================================================================

RFController::RFController(int rxPin, int txPin)
    : rxPin(rxPin), txPin(txPin),
      _lastCodeValue(0), _lastCodeTime(0), _debounceMs(300)
{
}

void RFController::init() {
    // Initialize RF receiver (on rxPin) and RF sender (on txPin).
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[RF][INIT] RX pin " + String(rxPin) + " TX pin " + String(txPin));
#endif
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    _rfSwitch = new RCSwitch();
    _rfSwitch->enableReceive(rxPin);
    _rfSwitch->enableTransmit(txPin);
}

void RFController::onCommand(RFCallback callback) {
    _callback = callback;
}

// ----------------------------------------------------------------------------
// update()
//   Poll the RF receiver. Must be called regularly from loop().
//
//   Flow:
//     1. If no frame is pending, do nothing.
//     2. Drop empty/invalid decodes (the library sometimes reports a decode
//        when there is no real data - we filter those out here).
//     3. Debounce: if it's the same code again within _debounceMs, ignore it.
//     4. Build an RFSignal that carries the raw data, then fire the callback.
//     5. resetAvailable() the receiver so it can capture the next frame.
// ----------------------------------------------------------------------------
void RFController::update()
{
    if (_rfSwitch->available())
    {
        unsigned long value   = _rfSwitch->getReceivedValue();
        int bits              = _rfSwitch->getReceivedBitlength();
        int protocol          = _rfSwitch->getReceivedProtocol();
        int pulse             = _rfSwitch->getReceivedDelay();

#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[RF][receive] Received code: 0x" + String(value, HEX) +
                       " / " + String(bits) + " bits / protocol=" + String(protocol) +
                       " / pulse=" + String(pulse) + "us");
    #endif
#endif

        // Ignore empty/invalid decodes
        if (value == 0) {
            #ifdef DEBUG_LEVEL
            #if DEBUG_LEVEL >= 2
                Debug::println(1, "[RF][ERROR] Invalid RF decode received (value=0)");
            #endif
            #endif
            _rfSwitch->resetAvailable();
            return;
        }

        // Debounce: ignore the same code repeating within the debounce window.
        if (value == _lastCodeValue && (millis() - _lastCodeTime) < _debounceMs) {
            #ifdef DEBUG_LEVEL
            #if DEBUG_LEVEL >= 3
                Debug::println("[RF][DEBOUNCE] Ignoring repeated RF code: 0x" + String(value, HEX) + 
                              " (last received " + String(millis() - _lastCodeTime) + "ms ago)");
            #endif
            #endif
            _rfSwitch->resetAvailable();
            return;
        }

        // Remember this code/time so we can detect repeats on the next decode.
        _lastCodeValue = value;
        _lastCodeTime  = millis();

        if (_callback) {
            RFSignal signal;
            signal.value     = value;
            signal.bits      = bits;
            signal.protocol  = protocol;
            signal.pulse     = pulse;
            signal.timestamp = millis();

            // Debug: Print received RF signal (level 3 - verbose)
            #ifdef DEBUG_LEVEL
            #if DEBUG_LEVEL >= 3
                Debug::println("[RF][VERBOSE] Received signal with value: 0x" + String(value, HEX) +
                              ", bits: " + String(bits) +
                              ", protocol: " + String(protocol) +
                              ", pulse: " + String(pulse) + "us");
            #endif
            #endif

            // Hand the packet to the user handler.
            #ifdef DEBUG_LEVEL
            #if DEBUG_LEVEL >= 1
                Debug::println(4, "[RF][INFO] Processing RF code: 0x" + String(value, HEX));
            #endif
            #endif
            _callback(signal);
        }

        // Ready for the next frame.
        _rfSwitch->resetAvailable();
    }
}

// ----------------------------------------------------------------------------
// send()
//   Transmit an RF signal with the given value and number of bits.
// ----------------------------------------------------------------------------
void RFController::send(unsigned long value, int bits) {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println(3, "[RF][SEND] Transmitting code: 0x" + String(value, HEX) + 
                      " / " + String(bits) + " bits");
    #endif
    #endif
    _rfSwitch->send(value, bits);
}

// ----------------------------------------------------------------------------
// setProtocol()
//   Set the protocol for RF transmission.
// ----------------------------------------------------------------------------
void RFController::setProtocol(int protocol) {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[RF][CONFIG] Setting RF protocol to: " + String(protocol));
    #endif
    #endif
    _rfSwitch->setProtocol(protocol);
}

// ----------------------------------------------------------------------------
// setPulseLength()
//   Set the pulse length for RF transmission.
// ----------------------------------------------------------------------------
void RFController::setPulseLength(int pulse) {
    #ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[RF][CONFIG] Setting RF pulse length to: " + String(pulse) + "us");
    #endif
    #endif
    _rfSwitch->setPulseLength(pulse);
}

// ----------------------------------------------------------------------------
// enableReceive()
//   Enable the RF receiver.
// ----------------------------------------------------------------------------
void RFController::enableReceive() {
    _rfSwitch->enableReceive(rxPin);
}

// ----------------------------------------------------------------------------
// disableReceive()
//   Disable the RF receiver.
// ----------------------------------------------------------------------------
void RFController::disableReceive() {
    _rfSwitch->disableReceive();
}
