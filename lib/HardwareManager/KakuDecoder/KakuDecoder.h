#pragma once
#include <Arduino.h>
#include "EventBus.h" 
#include "Debug.h"
#include <functional>

// Data structure passed to your callback
struct RFCommand {
  char house;        // 'A' - 'P'
  uint8_t button;    // 1 - 32
  uint32_t timestamp;
};

// Callback type definition
typedef std::function<void(const RFCommand&)> KakuCallback;

class KakuDecoder {
public:
    KakuDecoder(uint32_t debounceMs = 300);

    void onRawData(unsigned long value, int bits, int protocol, int pulse);
    void onCommand(KakuCallback callback);

private:
    uint32_t _debounceMs;
    unsigned long _lastCodeTime;
    unsigned long _lastCodeValue;
    KakuCallback _callback;

    static bool isValidKakuNibble(byte n);
    static bool isValidKaku24(unsigned long value);

    // CLASSIC KAKU HOUSE CODE (A–P)
    static char decodeHouseCode(byte n1, byte n2);

    static byte nibbleToPos(byte n);
    void processValue(unsigned long value);
    void decodeClassicKaku(unsigned long value);
};

