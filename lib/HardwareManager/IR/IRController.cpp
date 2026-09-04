#include <IRremote.hpp>
#include "IRController.h"

// ============================================================================
//  IRController.cpp - IR receiver/transmitter implementation.
//
//  Snapshots both decoded value and raw waveform for bit-exact replay.
//  Debouncing (300ms) prevents button-hold from flooding events.
// ============================================================================

IRController::IRController(int rxPin, int txPin)
    : rxPin(rxPin), txPin(txPin),
      _lastCodeValue(0), _lastCodeTime(0), _debounceMs(300)
{
}

void IRController::init() {
    // Initialize IR receiver (with LED feedback) and sender (without).
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[IR][INIT] RX pin " + String(rxPin) + " TX pin " + String(txPin));
#endif
    IrReceiver.begin(rxPin, ENABLE_LED_FEEDBACK);
    IrSender.begin(txPin, DISABLE_LED_FEEDBACK);
}

void IRController::onCommand(IRCallback callback) {
    _callback = callback;
}

// ----------------------------------------------------------------------------
// update()
//   Poll the IR receiver. Must be called regularly from loop().
//
//   Flow:
//     1. If no frame is pending, do nothing.
//     2. Drop empty/invalid decodes (the library sometimes reports a decode
//        when there is no real data - we filter those out here).
//     3. Debounce: if it's the same code again within _debounceMs, ignore it.
//     4. Build an IRCommand that carries BOTH the decoded summary AND the raw
//        waveform, then fire the callback.
//     5. resume() the receiver so it can capture the next frame.
// ----------------------------------------------------------------------------
void IRController::update()
{
   // Debug::println("[IR][DEBUG] Calling IRController::update()");
    if (IrReceiver.decode())
    {
        unsigned long value = IrReceiver.decodedIRData.decodedRawData;
        unsigned long now   = millis();

        // Ignore empty/invalid decodes (e.g. the library's hash decoder
        // returns true even for garbage/empty data).
        if (value == 0 || IrReceiver.decodedIRData.numberOfBits == 0) {
            IrReceiver.resume();
            return;
        }

        // Debounce: ignore the same code repeating within the debounce window.
        if (value == _lastCodeValue && (now - _lastCodeTime) < _debounceMs) {
            IrReceiver.resume();
            return;
        }

        // Remember this code/time so we can detect repeats on the next decode.
        _lastCodeValue = value;
        _lastCodeTime  = now;

#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println(2, "[IR][receive] Received code: 0x" + String(value, HEX) +
                       " / " + String(IrReceiver.decodedIRData.numberOfBits) + " bits");
    #endif
#endif

        if (_callback) {
            IRCommand cmd;
            cmd.code      = value;
            cmd.bits      = IrReceiver.decodedIRData.numberOfBits;
            cmd.protocol  = (uint16_t)IrReceiver.decodedIRData.protocol;
            cmd.timestamp = now;

            // ----------------------------------------------------------------
            // Snapshot the raw waveform for bit-exact replay.
            //
            // IrReceiver.rawlen   = number of captured on/off transitions.
            // rawlen - 1          = skip the leading "silence" entry.
            // compensateAndStoreIRResultInArray() writes the (compensated)
            // durations into cmd.rawCode[] - the exact array format that
            // IrSender.sendRaw() expects later.
            // ----------------------------------------------------------------
            uint16_t rawLen = IrReceiver.irparams.rawlen - 1; // skip leading space
            if (rawLen > IR_CONTROLLER_RAW_LEN) {
                rawLen = IR_CONTROLLER_RAW_LEN;              // clamp to our buffer
            }
            IrReceiver.compensateAndStoreIRResultInArray(cmd.rawCode);
            cmd.rawCodeLength = rawLen;
            cmd.hasRaw        = (rawLen > 0);

            // Hand the whole packet (decoded + raw) to the user handler.
            _callback(cmd);
        }

        // Ready for the next frame.
        IrReceiver.resume();
    }
}

// ----------------------------------------------------------------------------
// send()
//   Transmit an IR signal with the given command.
// ----------------------------------------------------------------------------
void IRController::send(const IRCommand& cmd)
{
    String sendHex = String(cmd.code, HEX);
    sendHex.toLowerCase();
    while (sendHex.length() < 8) sendHex = "0" + sendHex;
    if (sendHex.length() > 8) sendHex = sendHex.substring(sendHex.length() - 8);

#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[IR][send] Sending code 0x" + sendHex +
                       " rawLen=" + String(cmd.rawCodeLength) +
                       " bits=" + String(cmd.bits));
    #endif
#endif

    Debug::println("[IR][DEBUG] Disabling IR receiver for transmission");
    // Pause the receiver so it does not pick up our own transmission.
    disableReceive();

    if (cmd.hasRaw && cmd.rawCodeLength > 0) {
        Debug::println("[IR][send] mode=RAW");
        // Replay the exact received waveform (assume 38 kHz carrier).
        IrSender.sendRaw(cmd.rawCode, cmd.rawCodeLength, 38);
    } else if (cmd.bits == 32 || cmd.code > 0) {
        Debug::println("[IR][send] mode=NECRaw code=0x" + sendHex);
        // The RGB database stores the raw NEC payload itself (for example 0xfb04ef00),
        // not an address/command pair. Sending it through the deprecated MSB-first
        // path rewrites the bit order and produces a different device command.
        // Use the raw NEC encoder so the exact remote bit sequence is preserved.
        IrSender.sendNECRaw(cmd.code, 0);
    } else {
        Debug::println("[IR][send] mode=NECMSB code=0x" + sendHex);
        IrSender.sendNECMSB(cmd.code, (cmd.bits > 0) ? cmd.bits : 32);
    }

    // Re-enable the receiver so we can capture the next incoming frame.
    enableReceive();
}

// ----------------------------------------------------------------------------
// enableReceive()
//   Enable the IR receiver.
// ----------------------------------------------------------------------------
void IRController::enableReceive()
{
    Debug::println("[IR][DEBUG] Enabling IR receiver");
    IrReceiver.start();
}

// ----------------------------------------------------------------------------
// disableReceive()
//   Disable the IR receiver.
// ----------------------------------------------------------------------------
void IRController::disableReceive()
{
    Debug::println("[IR][DEBUG] Disabling IR receiver");
    IrReceiver.stop();
}


