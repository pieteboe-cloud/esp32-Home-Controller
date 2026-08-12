
#include "RFController.h"

// ============================================================================
//  RFController.cpp - implementation of the RF receive logic.
//
//  The key idea: when we receive an RF frame we don't just keep the decoded
//  number - we also store all the raw parameters (value, bits, protocol, pulse).
//  The Core will handle echoing the signal.
// ============================================================================

// ----------------------------------------------------------------------------
// Constructor.
//   Stores the pins. The debounce window (300 ms) also conveniently stops a
//   remote that keeps repeating its frame (while a button is held) from
//   flooding our callback every few milliseconds.
// ----------------------------------------------------------------------------
RFController::RFController(int rxPin, int txPin)
    : rxPin(rxPin), txPin(txPin),
      _lastCodeValue(0), _lastCodeTime(0), _debounceMs(300)
{
}

// ----------------------------------------------------------------------------
// init()
//   Start both the RF receiver (on rxPin) and the RF sender (on txPin).
// ----------------------------------------------------------------------------
void RFController::init()
{
#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[RF][init] Initializing RF Controller with RX pin " + String(rxPin) + " and TX pin " + String(txPin));
    #endif
#endif
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    _rfSwitch = new RCSwitch();
    _rfSwitch->enableReceive(rxPin);
    _rfSwitch->enableTransmit(txPin);
}

// ----------------------------------------------------------------------------
// onCommand()
//   Store the user-supplied callback. update() will invoke it whenever a new,
//   valid RF frame has been received.
// ----------------------------------------------------------------------------
void RFController::onCommand(RFCallback callback)
{
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
            _rfSwitch->resetAvailable();
            return;
        }

        // Debounce: ignore the same code repeating within the debounce window.
        if (value == _lastCodeValue && (millis() - _lastCodeTime) < _debounceMs) {
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

            // Debug: Print received RF signal
            Debug::println("[RF][DEBUG] Received signal with value: 0x" + String(value, HEX) +
                          ", bits: " + String(bits) +
                          ", protocol: " + String(protocol) +
                          ", pulse: " + String(pulse) + "us");

            // Hand the packet to the user handler.
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
    _rfSwitch->send(value, bits);
}

// ----------------------------------------------------------------------------
// setProtocol()
//   Set the protocol for RF transmission.
// ----------------------------------------------------------------------------
void RFController::setProtocol(int protocol) {
    _rfSwitch->setProtocol(protocol);
}

// ----------------------------------------------------------------------------
// setPulseLength()
//   Set the pulse length for RF transmission.
// ----------------------------------------------------------------------------
void RFController::setPulseLength(int pulse) {
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
