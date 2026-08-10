#include "KakuDecoder.h"

// ==========================================
// Constructor
// ==========================================
KakuDecoder::KakuDecoder(uint32_t debounceMs)
  : _debounceMs(debounceMs), _lastCodeTime(0), _lastCodeValue(0) {
}

// ==========================================
// Set callback
// ==========================================
void KakuDecoder::onCommand(KakuCallback callback) {
  _callback = callback;
}

// ==========================================
// Classic KaKu House Code Decoder (n1+n2)
// ==========================================
// Mapping verified from a real A-P sweep with the remote.
char KakuDecoder::decodeHouseCode(byte n1, byte n2) {
  byte addr = (n1 << 4) | n2;
  switch (addr) {
    case 0x00: return 'A'; case 0x40: return 'B'; case 0x10: return 'C';
    case 0x04: return 'D'; case 0x44: return 'E'; case 0x14: return 'F';
    case 0x54: return 'G'; case 0x01: return 'H'; case 0x41: return 'I';
    case 0x11: return 'J'; case 0x51: return 'K'; case 0x05: return 'L';
    case 0x45: return 'M'; case 0x15: return 'N'; case 0x55: return 'O';
    case 0x50: return 'P';
    default:   return '?';
  }
}

// ==========================================
// Validate KaKu nibble
// ==========================================
bool KakuDecoder::isValidKakuNibble(byte n) {
  return (n == 0x00 || n == 0x01 || n == 0x04 || n == 0x05);
}

// ==========================================
// Validate full 24-bit KaKu frame
// ==========================================
bool KakuDecoder::isValidKaku24(unsigned long value) {
  byte n1 = (value >> 20) & 0x0F;
  byte n2 = (value >> 16) & 0x0F;
  byte n3 = (value >> 12) & 0x0F;
  byte n4 = (value >> 8)  & 0x0F;
  byte n5 = (value >> 4)  & 0x0F;
  byte n6 =  value        & 0x0F;

  // CLASSIC KAKU HOUSE VALIDATION (critical!)
  if (decodeHouseCode(n1, n2) == '?') return false;

  if (!isValidKakuNibble(n3)) return false;
  if (!isValidKakuNibble(n4)) return false;
  if (n5 != 0x01) return false;
  if (n6 != 0x04 && n6 != 0x05) return false;

  return true;
}

// ==========================================
// Map nibble to 1–4
// ==========================================
byte KakuDecoder::nibbleToPos(byte n) {
  if      (n == 0x00) return 1;
  else if (n == 0x04) return 2;
  else if (n == 0x01) return 3;
  else if (n == 0x05) return 4;
  else return 0;
}

// ==========================================
// Decode Classic KaKu (A–P + 1–32)
// ==========================================
void KakuDecoder::decodeClassicKaku(unsigned long value) {
  byte n1 = (value >> 20) & 0x0F;
  byte n2 = (value >> 16) & 0x0F;
  byte n3 = (value >> 12) & 0x0F;
  byte n4 = (value >> 8)  & 0x0F;
  byte n6 =  value        & 0x0F;

  char house = decodeHouseCode(n1, n2);
  byte rowPos = nibbleToPos(n3);
  byte sliderPos = nibbleToPos(n4);
  bool isOn = (n6 == 0x05);

  int button = (sliderPos - 1) * 8 + (rowPos - 1) * 2 + (isOn ? 1 : 2);

  Debug::println("Classic KaKu: House " + String(house) +
                 " Button " + String(button));
}

// ==========================================
// Process and forward to callback
// ==========================================
void KakuDecoder::processValue(unsigned long value) {
  byte n1 = (value >> 20) & 0x0F;
  byte n2 = (value >> 16) & 0x0F;
  byte n3 = (value >> 12) & 0x0F;
  byte n4 = (value >> 8)  & 0x0F;
  byte n6 =  value        & 0x0F;

  RFCommand cmd;
  cmd.house = decodeHouseCode(n1, n2);

  byte rowPos = nibbleToPos(n3);
  byte sliderPos = nibbleToPos(n4);
  bool isOn = (n6 == 0x05);

  cmd.button = (sliderPos - 1) * 8 + (rowPos - 1) * 2 + (isOn ? 1 : 2);
  cmd.timestamp = millis();

  if (_callback) _callback(cmd);
}

// ==========================================
// Raw data entry point
// ==========================================
void KakuDecoder::onRawData(unsigned long value, int bits, int protocol, int pulse) {
  unsigned long now = millis();

  if (value == 0) return;

  if (value == _lastCodeValue && (now - _lastCodeTime) < _debounceMs) {
    return;
  }

  _lastCodeValue = value;
  _lastCodeTime = now;

  bool valid24 = (bits == 24 && isValidKaku24(value));

  if (valid24) {
    decodeClassicKaku(value);
    processValue(value);
  }
}
