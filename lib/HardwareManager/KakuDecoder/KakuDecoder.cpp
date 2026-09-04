// ============================================================================
//  KakuDecoder.cpp - Kaku protocol (GE Enbrighten) RF decoder.
//
//  Extracts house codes (A-P) and button positions (1-32) from 24-bit RF data.
//  Validates frame structure before firing callback.
// ============================================================================

#include "KakuDecoder.h"

KakuDecoder::KakuDecoder(uint32_t debounceMs)
  : _debounceMs(debounceMs), _lastCodeTime(0), _lastCodeValue(0) {
}

void KakuDecoder::onCommand(KakuCallback callback) {
  _callback = callback;
}

char KakuDecoder::decodeHouseCode(byte n1, byte n2) {
  // Combine two nibbles to get the 8-bit address and map it to a letter (A-P)
  byte addr = (n1 << 4) | n2;
  switch (addr) {
    case 0x00: return 'A'; case 0x40: return 'B'; case 0x10: return 'C';
    case 0x04: return 'D'; case 0x44: return 'E'; case 0x14: return 'F';
    case 0x54: return 'G'; case 0x01: return 'H'; case 0x41: return 'I';
    case 0x11: return 'J'; case 0x51: return 'K'; case 0x05: return 'L';
    case 0x45: return 'M'; case 0x15: return 'N'; case 0x55: return 'O';
    case 0x50: return 'P';
    default:   return '?'; // Unknown house code
  }
}

bool KakuDecoder::isValidKakuNibble(byte n) {
  // Kaku uses specific nibbles (0, 1, 4, 5) for valid data
  return (n == 0x00 || n == 0x01 || n == 0x04 || n == 0x05);
}

bool KakuDecoder::isValidKaku24(unsigned long value) {
  // Validate the entire 24-bit frame structure
  byte n1 = (value >> 20) & 0x0F; // House high
  byte n2 = (value >> 16) & 0x0F; // House low
  byte n3 = (value >> 12) & 0x0F; // Row
  byte n4 = (value >> 8)  & 0x0F; // Slider
  byte n5 = (value >> 4)  & 0x0F; // Protocol marker (must be 1)
  byte n6 =  value        & 0x0F; // On/Off (4 or 5)

  if (decodeHouseCode(n1, n2) == '?') return false; // Invalid house
  if (!isValidKakuNibble(n3)) return false;
  if (!isValidKakuNibble(n4)) return false;
  if (n5 != 0x01) return false;
  if (n6 != 0x04 && n6 != 0x05) return false;

  return true; // All checks passed
}

byte KakuDecoder::nibbleToPos(byte n) {
  // Convert Kaku nibbles (0,4,1,5) to human positions (1,2,3,4)
  if      (n == 0x00) return 1;
  else if (n == 0x04) return 2;
  else if (n == 0x01) return 3;
  else if (n == 0x05) return 4;
  else return 0; // Invalid
}

void KakuDecoder::decodeClassicKaku(unsigned long value) {
  // Debug helper: prints the decoded House and Button to serial
  byte n1 = (value >> 20) & 0x0F;
  byte n2 = (value >> 16) & 0x0F;
  byte n3 = (value >> 12) & 0x0F;
  byte n4 = (value >> 8)  & 0x0F;
  byte n6 =  value        & 0x0F;

  char house = decodeHouseCode(n1, n2);
  byte rowPos = nibbleToPos(n3);
  byte sliderPos = nibbleToPos(n4);
  bool isOn = (n6 == 0x05);

  // Calculate button number (1-32) based on slider row and on/off state
  int button = (sliderPos - 1) * 8 + (rowPos - 1) * 2 + (isOn ? 1 : 2);

#if DEBUG_LEVEL >= 3
  Debug::println(3, "[KAKU][DECODE] House " + String(house) + " Button " + String(button));
#endif
}

// ==========================================
// Process the decoded value and notify the system
// ==========================================
void KakuDecoder::processValue(unsigned long value) {
  byte n1 = (value >> 20) & 0x0F;
  byte n2 = (value >> 16) & 0x0F;
  byte n3 = (value >> 12) & 0x0F;
  byte n4 = (value >> 8)  & 0x0F;
  byte n6 =  value        & 0x0F;

  RFCommand cmd;
  char houseChar = decodeHouseCode(n1, n2);
  cmd.house = houseChar;

  byte rowPos = nibbleToPos(n3);
  byte sliderPos = nibbleToPos(n4);
  bool isOn = (n6 == 0x05);

  // Calculate final button ID (1-32)
  cmd.button = (sliderPos - 1) * 8 + (rowPos - 1) * 2 + (isOn ? 1 : 2);
  cmd.timestamp = millis();

  SystemEvent evt;
  evt.source = "KAKU";
  // Create a readable ID like "KAKU_C_2"
  evt.identifier = "KAKU_" + String(houseChar) + "_" + String(cmd.button);
  evt.rawData = String(value, HEX);
  
  EventBus::getInstance().publish(evt);
  #if DEBUG_LEVEL >= 2
    Debug::println("[KAKU][processValue] Published Event: " + evt.identifier + " Raw: " + evt.rawData);
  #endif
  // ------------------------------

  // Trigger the old-style callback if anyone is still listening
  if (_callback) _callback(cmd);
}

void KakuDecoder::onRawData(unsigned long value, int bits, int protocol, int pulse) {
  unsigned long now = millis();

  if (value == 0) return; // Ignore empty signals

  // Debounce: Ignore if same code received recently
  if (value == _lastCodeValue && (now - _lastCodeTime) < _debounceMs) {
    return;
  }

  _lastCodeValue = value;
  _lastCodeTime = now;

  // Check if it's a valid 24-bit Kaku frame
  bool valid24 = (bits == 24 && isValidKaku24(value));

  if (valid24) {
    decodeClassicKaku(value); 
    processValue(value);      
  }
}   