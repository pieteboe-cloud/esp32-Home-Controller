// ============================================================================
//  LivingColors.cpp - (LivingColors) lighting controller.
//
//  Controls wireless RGB lamps via CC2500 radio module.
//  Supports HSV/RGB color modes, power control, and synchronized scenes.
// ============================================================================

#include "LivingColors.h"
#include "../Debug/Debug.h"

LivingColors::LivingColors(uint8_t csPin, uint8_t sckPin, uint8_t misoPin,
                           uint8_t mosiPin, uint8_t gdo2Pin)
    : radio(csPin, sckPin, misoPin, mosiPin, gdo2Pin) {}

void LivingColors::begin(bool wakeAllBlack) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 2) {
    Debug::println(2,
                   "[LIVINGCOLORS][INIT] Starting LivingColors initialization");
  }
#endif
  // Initialize the radio module
  radio.begin();

  // Verify radio module is working.
  if (!radio.verify()) {
    Debug::println(1, "[LIVINGCOLORS][ERROR] CC2500 radio verification failed");
    return;
  }

#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 2) {
    Debug::println(3,
                   "[LIVINGCOLORS][INFO] Radio module verified successfully");
  }
#endif

  // Configure radio for LivingColors communication
  radio.initForLivingColors();

  // Wake up all lamps by setting them to black and then red
  setBlackAll(); // Ensure all lamps are 'on' before setting colors
  for (uint8_t i = 0; i < LAMP_COUNT; i++) {
    setColorRGB(i, 255, 0, 0);
    delay(50); // small delay for pretty effect
  }
  for (uint8_t i = 0; i < LAMP_COUNT; i++) {
    setColorRGB(i, 0, 255, 0);
    delay(50);
  }
  for (uint8_t i = 0; i < LAMP_COUNT; i++) {
    setColorRGB(i, 0, 0, 255);
    delay(50);
  }

#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 2) {
    Debug::println(2, "[LIVINGCOLORS][INFO] Initialization complete");
  }
#endif
}

/**
 * @brief Set lamp color using HSV values
 * Validates lamp index and sends color command to the specified lamp
 */
void LivingColors::setColor(uint8_t lampIndex, uint8_t h, uint8_t s,
                            uint8_t v) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][setColor] lamp=" + String(lampIndex) +
                          " h=" + String(h) + " s=" + String(s) +
                          " v=" + String(v));
  }
#endif
  // Check if lamp index is valid
  if (lampIndex >= LAMP_COUNT) {
#ifdef DEBUG_LEVEL
    if (Debug::getDebugLevel() >= 4) {
      Debug::println(1, "[LIVINGCOLORS][setColor] lamp out of range (>= " +
                            String(LAMP_COUNT) + "), skipping");
    }
#endif
    return;
  }
  // Send color command to the specified lamp
  sendPacket(lampIndex, 0x03, h, s, v);
}

/**
 * @brief Set lamp color using RGB values
 * Converts RGB to HSV and then sets the color
 */
void LivingColors::setColorRGB(uint8_t lampIndex, uint8_t r, uint8_t g,
                               uint8_t b) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][setColorRGB] lamp=" + String(lampIndex) +
                          " r=" + String(r) + " g=" + String(g) +
                          " b=" + String(b));
  }
#endif
  uint8_t h, s, v;
  // Convert RGB to HSV color space
  rgbToHsv(r, g, b, h, s, v);
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][setColorRGB] converted to h=" +
                          String(h) + " s=" + String(s) + " v=" + String(v));
  }
#endif
  // Set color using HSV values
  setColor(lampIndex, h, s, v);
}

/**
 * @brief Turn lamp off (black)
 * Sets color to black (hue=0, saturation=0, value=1)
 */
void LivingColors::setBlack(uint8_t lampIndex) {
  sendPacket(lampIndex, 0x05, 0, 0, 1);
}

/**
 * @brief Set all lamps to the same HSV color
 * Iterates through all lamps and sets each one to the specified color
 */
void LivingColors::setColorAll(uint8_t h, uint8_t s, uint8_t v) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][setColorAll] h=" + String(h) +
                          " s=" + String(s) + " v=" + String(v));
  }
#endif
  // Set each lamp to the specified HSV color
  for (uint8_t i = 0; i < LAMP_COUNT; i++) {
    setColor(i, h, s, v);
  }
}

/**
 * @brief Set all lamps to the same RGB color
 * Converts RGB to HSV and then sets all lamps to that color
 */
void LivingColors::setColorRGBAll(uint8_t r, uint8_t g, uint8_t b) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][setColorRGBAll] r=" + String(r) +
                          " g=" + String(g) + " b=" + String(b));
  }
#endif
  uint8_t h, s, v;
  // Convert RGB to HSV color space
  rgbToHsv(r, g, b, h, s, v);
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(5, "[LIVINGCOLORS][setColorRGBAll] converted to h=" +
                          String(h) + " s=" + String(s) + " v=" + String(v));
  }
#endif
  // Set all lamps to the converted HSV color
  setColorAll(h, s, v);
}

/**
 * @brief Turn all lamps off (black)
 * Iterates through all lamps and turns each one off
 */
void LivingColors::setBlackAll() {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][setBlackAll] Turning all lamps off");
  }
#endif
  // Turn each lamp off (black)
  for (uint8_t i = 0; i < LAMP_COUNT; i++) {
    setBlack(i);
  }
}

/**
 * @brief Power Off Lamp , needs 0x05 to turn on again
 * Iterates through all lamps and turns each one off
 */
void LivingColors::powerOffHard(uint8_t lampIndex) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][powerOffHard] lamp=" + String(lampIndex));
  }
#endif
  if (lampIndex >= LAMP_COUNT)
    return;
  sendPacket(lampIndex, 0x07, 0, 0, 0);
}

/**
 * @brief Power all lamps off
 * Iterates through all lamps and powers each one off
 * need 0x05 to turn on again
 */
void LivingColors::powerOffHardAll() {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][powerOffHardAll] Powering off all lamps");
  }
#endif
  for (uint8_t i = 0; i < LAMP_COUNT; i++) {
    powerOffHard(i);
  }
}

/**
 * @brief Set multiple lamps to different colors
 * Processes an array of lamp indices and corresponding HSV color values
 */
void LivingColors::setColorBatch(const uint8_t *indices, uint8_t count,
                                 const uint8_t *h, const uint8_t *s,
                                 const uint8_t *v) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 4) {
    Debug::println(4, "[LIVINGCOLORS][setColorBatch] count=" + String(count));
  }
#endif
  // Set each specified lamp to its corresponding color
  for (uint8_t i = 0; i < count; i++) {
#ifdef DEBUG_LEVEL
    if (Debug::getDebugLevel() >= 4) {
      Debug::println(4, "[LIVINGCOLORS][setColorBatch] lamp[" + String(i) +
                            "]=" + String(indices[i]) + " h=" + String(h[i]) +
                            " s=" + String(s[i]) + " v=" + String(v[i]));
    }
#endif
    setColor(indices[i], h[i], s[i], v[i]);
  }
}

/**
 * @brief Turn multiple lamps off (black)
 * Processes an array of lamp indices and turns each one off
 * example setBlackBatch({2,4,6} , 3) turn off lamp 2,4,6 , count = 3
 */
void LivingColors::setBlackBatch(const uint8_t *indices, uint8_t count) {
  // Turn each specified lamp off (black)
  for (uint8_t i = 0; i < count; i++) {
    setBlack(indices[i]);
  }
}

/**
 * @brief Hard power off multiple lamps
 * Processes an array of lamp indices and hard powers off each one
 * example powerOffHardBatch({2,4,6} , 3) turn off lamp 2,4,6 , count = 3
 */
void LivingColors::powerOffHardBatch(const uint8_t *indices, uint8_t count) {
  // Hard power off each specified lamp
  for (uint8_t i = 0; i < count; i++) {
    powerOffHard(indices[i]);
  }
}

/**
 * @brief Send a control packet to a lamp
 * Constructs and sends a radio packet with the specified command and color data
 */
void LivingColors::sendPacket(uint8_t lampIndex, uint8_t command, uint8_t h,
                              uint8_t s, uint8_t v) {
  // Get the address of the target lamp
  const uint8_t *addr = LAMP_ADDRESSES[lampIndex];

  // Construct the data packet
  uint8_t data[15]; // set first byte 0x0E = 14 bytes (to come next)
  data[0] = 0x0E;   // first byte + Packet length (0x0E)  = 15

  // Copy lamp address to packet
  for (uint8_t i = 0; i < 9; i++) {
    data[1 + i] = addr[i];
  }

  // Add command, sequence, and color data
  data[10] = command;    // Command type (0x03 = Set color, 0x05 = power on with
                         // color, 0x07 = power off)
  data[11] = sequence++; // Sequence counter
  data[12] = h;          // Hue
  data[13] = s;          // Saturation
  data[14] = v;          // Value/Brightness

#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 5) {
    Debug::println(4, String("[LIVINGCOLORS][sendPacket] ") +
                          String(sizeof(data)));
  }
#endif

  // Send the packet with proper timing
  radio.waitForGDO2Low(100);         // Wait for GDO2 to go low
  radio.sendRaw(data, sizeof(data)); // Send the packet
  radio.waitForGDO2Low(100);         // Wait for GDO2 to go low again

  /*
   * The classic LivingColors/CC2500 implementations
   * use a gap of roughly 5–20 ms between frames.
   * Since sending 12 lamps × one frame each,
   * a per-frame delay of ~10 ms gives a total burst of ~120 ms,
   * which is well within acceptable response time
   * If you still see occasional missed lamps, increase to 15–20 ms.
   * If you want snappier response and it's reliable, try 5 ms first.
   */
  delayMicroseconds(7000); // 7ms delay between packets
}

/**
 * @brief Convert RGB color to HSV
 * Converts RGB values to HSV color space
 */
void LivingColors::rgbToHsv(uint8_t r, uint8_t g, uint8_t b, uint8_t &h,
                            uint8_t &s, uint8_t &v) {
  // Find minimum and maximum RGB values
  uint8_t rgb_min = min(r, min(g, b));
  uint8_t rgb_max = max(r, max(g, b));

  // Value is the maximum RGB value (brightness)
  v = rgb_max;

  // If value is 0, color is black - hue and saturation are undefined (set to 0)
  if (v == 0) {
    h = s = 0;
    return;
  }

  // Calculate saturation as a percentage of the difference between max and min
  s = 255 * (uint32_t)(rgb_max - rgb_min) / v;

  // If saturation is 0, color is grayscale - hue is undefined (set to 0)
  if (s == 0) {
    h = 0;
    return;
  }

  // Calculate hue based on which RGB component is maximum
  // Hue is divided into 6 regions (0-255), each spanning 43 units
  if (rgb_max == r) {
    // Red is max - hue depends on relationship between green and blue
    h = 0 + 43 * (g - b) / (rgb_max - rgb_min);
  } else if (rgb_max == g) {
    // Green is max - hue is offset by 85 (43*2)
    h = 85 + 43 * (b - r) / (rgb_max - rgb_min);
  } else {
    // Blue is max - hue is offset by 171 (43*4)
    h = 171 + 43 * (r - g) / (rgb_max - rgb_min);
  }
}

/**
 * @brief Convert HSV color to RGB
 * Converts HSV values to RGB color space
 */
void LivingColors::hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t &r,
                            uint8_t &g, uint8_t &b) {
  // If saturation is 0, color is grayscale (all RGB components equal)
  if (s == 0) {
    r = g = b = v;
    return;
  }

  // Convert hue to region (0-5) and remainder (0-255)
  uint8_t region = h / 43; // Which of the 6 color regions we're in
  uint8_t remainder = (h - (region * 43)) * 6; // Position within the region

  // Calculate intermediate RGB values for interpolation
  // These represent the color at different points in the hue region
  uint8_t p = (v * (255 - s)) >> 8; // Minimum RGB value in this region
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8; // Intermediate value
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >>
              8; // Another intermediate value

  // Map the region to the appropriate RGB values
  switch (region) {
  case 0: // Red to yellow transition
    r = v;
    g = t;
    b = p;
    break;
  case 1: // Yellow to green transition
    r = q;
    g = v;
    b = p;
    break;
  case 2: // Green to cyan transition
    r = p;
    g = v;
    b = t;
    break;
  case 3: // Cyan to blue transition
    r = p;
    g = q;
    b = v;
    break;
  case 4: // Blue to magenta transition
    r = t;
    g = p;
    b = v;
    break;
  default: // Magenta to red transition
    r = v;
    g = p;
    b = q;
    break;
  }
}