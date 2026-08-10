#include <IRremote.hpp>
#include "IRController.h"
#include <Debug.h>

// ============================================================================
//  IRController.cpp - implementation of the IR receive/echo logic.
//
//  The key idea: when we receive an IR frame we don't just keep the decoded
//  number - we also snapshot the raw waveform (a list of on/off timings).
//  Later, echo() replays that exact snapshot with sendRaw(), so what goes out
//  is a faithful copy of what came in. This is what makes the device behave
//  like a transparent IR "repeater".
// ============================================================================

// ----------------------------------------------------------------------------
// Constructor.
//   Stores the pins. The debounce window (300 ms) also conveniently stops a
//   remote that keeps repeating its frame (while a button is held) from
//   flooding our callback every few milliseconds.
// ----------------------------------------------------------------------------
IRController::IRController(int rxPin, int txPin)
    : rxPin(rxPin), txPin(txPin),
      _lastCodeValue(0), _lastCodeTime(0), _debounceMs(300)
{
}

// ----------------------------------------------------------------------------
// init()
//   Start both the IR receiver (on rxPin) and the IR sender (on txPin).
//   The receiver drives the on-board LED as a visual feedback on each IR pulse,
//   the sender LED feedback is disabled to keep pin usage minimal.
// ----------------------------------------------------------------------------
void IRController::init()
{
#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 1
        Debug::println("[IR][init] Initializing IR Controller with RX pin " + String(rxPin) + " and TX pin " + String(txPin));
    #endif
#endif
    IrReceiver.begin(rxPin, ENABLE_LED_FEEDBACK);
    IrSender.begin(txPin, DISABLE_LED_FEEDBACK);
}

// ----------------------------------------------------------------------------
// onCommand()
//   Store the user-supplied callback. update() will invoke it whenever a new,
//   valid IR frame has been received.
// ----------------------------------------------------------------------------
void IRController::onCommand(IRCallback callback)
{
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
        Debug::println("[IR][receive] Received code: 0x" + String(value, HEX) +
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
// echo()
//   Re-transmit a previously received IRCommand so the output waveform is
//   identical to the input.
//
//   Why stop() / start()?
//   ---------------------
//   The IR TX LED and RX module are physically close. If we transmit while the
//   receiver is live, we'll pick up our own burst and loop forever. So we
//   briefly disable the receiver, blast the raw waveform, then re-enable it.
//
//   Why sendRaw() and not sendNECMSB()?
//   -----------------------------------
//   sendNECMSB() re-encodes a *number* into a fresh NEC frame - the resulting
//   waveform may not match the original (different bit order etc.).
//   sendRaw() replays the captured on/off timings verbatim, guaranteeing an
//   exact copy of the received signal.
// ----------------------------------------------------------------------------
void IRController::echo(const IRCommand& cmd)
{
#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 2
        Debug::println("[IR][echo] Echoing code 0x" + String(cmd.code, HEX) +
                       " rawLen=" + String(cmd.rawCodeLength));
    #endif
#endif

    // Pause the receiver so it does not pick up our own transmission.
    IrReceiver.stop();

    if (cmd.hasRaw && cmd.rawCodeLength > 0) {
        // Replay the exact received waveform (assume 38 kHz carrier).
        IrSender.sendRaw(cmd.rawCode, cmd.rawCodeLength, 38);
    } else {
        // Fallback: if no raw snapshot was captured, re-send as NEC MSB-first.
        IrSender.sendNECMSB(cmd.code, (cmd.bits > 0) ? cmd.bits : 32);
    }

    // Re-enable the receiver so we can capture the next incoming frame.
    IrReceiver.start();
}
