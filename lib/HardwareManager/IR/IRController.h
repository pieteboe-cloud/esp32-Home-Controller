#ifndef IR_CONTROLLER_H
#define IR_CONTROLLER_H

#include <Arduino.h>
#include "../../Debug/Debug.h"
#include <functional>

// ============================================================================
//  IRController - wraps the Arduino-IRremote library for this project.
//
//  PURPOSE
//  --------
//  Provide a clean interface to receive and transmit IR signals, with the
//  special ability to "echo" (replay) a received signal *bit-for-bit*.
//
//  WHY RAW TIMINGS?
//  ----------------
//  When we receive IR we get a decoded *value* (e.g. 0xFC03EF00 for NEC).
//  Re-encoding that value and sending it again (e.g. with sendNECMSB) does NOT
//  always reproduce the original waveform - the bit order / protocol details
//  can differ. Instead we capture the exact raw timing ticks of the received
//  burst and replay them with IRremote's IrSender.sendRaw(). That guarantees an
//  identical waveform on every transmission.
// ============================================================================

// Maximum number of raw timing entries we keep for replay.
// (100 is a safe default for NEC/Sony/RC5 type bursts.)
#ifndef IR_CONTROLLER_RAW_LEN
#define IR_CONTROLLER_RAW_LEN 100
#endif

// ----------------------------------------------------------------------------
// IRCommand
//   Everything about one received IR signal.
//   - 'code/bits/protocol'   : the decoded summary (handy for logging/debug).
//   - 'rawCode/rawCodeLength': the exact timing waveform, used for faithful echo.
// ----------------------------------------------------------------------------
struct IRCommand {
  unsigned long code;                    // decoded raw data (e.g. NEC value)
  uint16_t      bits;                    // number of data bits decoded
  uint16_t      protocol;                // decode_type_t from the IRremote lib
  uint32_t      timestamp;               // millis() when the signal arrived

  // Exact captured waveform (compensated ticks, as expected by sendRaw()).
  // rawCode[i] is a mark (LED on) when i is even, a space (LED off) when odd.
  uint8_t       rawCode[IR_CONTROLLER_RAW_LEN];
  uint16_t      rawCodeLength;           // how many entries of rawCode[] are valid
  bool          hasRaw;                  // true = rawCode[] holds a valid waveform
};

// Callback signature. Called on the main loop whenever a valid IR frame arrives.
typedef std::function<void(const IRCommand&)> IRCallback;

// ----------------------------------------------------------------------------
// IRController
// ----------------------------------------------------------------------------
class IRController
{
public:
    // rxPin: input from the IR receiver module (usually TSOP38xxx, 38 kHz).
    // txPin: output that drives the IR LED (through a transistor usually).
    IRController(int rxPin, int txPin);

    // Begin the IR receiver + sender. Call once from setup().
    void init();

    // Poll the receiver for new data and fire the callback if a code arrived.
    // Call this every loop() iteration.
    void update();

    // Replay a captured IRCommand exactly as it was received.
    // This is what makes the "remote echo / repeater" behaviour work.
    void echo(const IRCommand& cmd);

    // Register the function invoked by update() when a new code is received.
    void onCommand(IRCallback callback);

private:
    int rxPin;                            // IR receiver module data pin
    int txPin;                            // IR LED driver pin

    IRCallback _callback;                 // user-supplied handler

    // Simple debounce: ignore the same code arriving again within this window,
    // so a remote that repeats its frame while held down doesn't spam us.
    unsigned long _lastCodeValue;
    unsigned long _lastCodeTime;
    unsigned long _debounceMs;
};

#endif

