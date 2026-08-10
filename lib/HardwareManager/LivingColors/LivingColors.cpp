#include "LivingColors.h"
#include "../Debug/Debug.h"

LivingColors::LivingColors(uint8_t csPin, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin, uint8_t gdo2Pin)
    : radio(csPin, sckPin, misoPin, mosiPin, gdo2Pin)
{
}

void LivingColors::begin(bool wakeAllBlack) {
    radio.begin();

    if (!radio.verify()) {
        Debug::println("[LC] ERROR: CC2500 verify failed");
        return;
    }

    radio.initForLivingColors();

   // if (wakeAllBlack) {
        setBlackAll();
        setColorRGBAll(255, 0, 0);

     // }


    Debug::println("[LC] Initialized");
}

void LivingColors::setColor(uint8_t lampIndex,
                            uint8_t h, uint8_t s, uint8_t v) {
#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= 3
        Debug::println("[LC] [setColor] lamp=" + String(lampIndex) +
                       " h=" + String(h) + " s=" + String(s) + " v=" + String(v));
    #endif
#endif
    if (lampIndex >= LAMP_COUNT) {
#ifdef DEBUG_LEVEL
        #if DEBUG_LEVEL >= 3
            Debug::println("[LC] [setColor] lamp out of range (>= " + String(LAMP_COUNT) + "), skipping");
        #endif
#endif
        return;
    }
    sendPacket(lampIndex, 0x03, h, s, v);
}

void LivingColors::setColorRGB(uint8_t lampIndex,
                               uint8_t r, uint8_t g, uint8_t b) {
    uint8_t h,s,v;
    rgbToHsv(r,g,b,h,s,v);
    setColor(lampIndex, h,s,v);
}

void LivingColors::setBlack(uint8_t lampIndex) {
    setColor(lampIndex, 0, 0, 1);
}

void LivingColors::setColorAll(uint8_t h, uint8_t s, uint8_t v) {
    for (uint8_t i = 0; i < LAMP_COUNT; i++) {
        setColor(i, h,s,v);
    }
}

void LivingColors::setColorRGBAll(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t h,s,v;
    rgbToHsv(r,g,b,h,s,v);
    setColorAll(h,s,v);
}

void LivingColors::setBlackAll() {
    for (uint8_t i = 0; i < LAMP_COUNT; i++) {
        setBlack(i);
    }
}

void LivingColors::powerOffHard(uint8_t lampIndex) {
    if (lampIndex >= LAMP_COUNT) return;
    sendPacket(lampIndex, 0x07, 0,0,0);
}

void LivingColors::powerOffHardAll() {
    for (uint8_t i = 0; i < LAMP_COUNT; i++) {
        powerOffHard(i);
    }
}

void LivingColors::setColorBatch(const uint8_t* indices, uint8_t count,
                                 const uint8_t* h, const uint8_t* s, const uint8_t* v) {
    for (uint8_t i = 0; i < count; i++) {
        setColor(indices[i], h[i], s[i], v[i]);
    }
}

void LivingColors::setBlackBatch(const uint8_t* indices, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        setBlack(indices[i]);
    }
}

void LivingColors::powerOffHardBatch(const uint8_t* indices, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        powerOffHard(indices[i]);
    }
}

void LivingColors::sendPacket(uint8_t lampIndex,
                              uint8_t command,
                              uint8_t h, uint8_t s, uint8_t v) {

    const uint8_t* addr = LAMP_ADDRESSES[lampIndex];

    uint8_t data[15];
    data[0] = 0x0E;

    for (uint8_t i = 0; i < 9; i++) {
        data[1 + i] = addr[i];
    }

    data[10] = command;
    data[11] = sequence++;
    data[12] = h;
    data[13] = s;
    data[14] = v;

    radio.waitForGDO2Low(100);
    radio.sendRaw(data, sizeof(data));
    radio.waitForGDO2Low(100);


    /*. The classic LivingColors/CC2500 implementations 
    use a gap of roughly 5–20 ms between frames. 
    Since sending 12 lamps × one frame each, 
    a per-frame delay of ~10 ms gives a total burst of ~120 ms, 
    which is well within acceptable responce time
    If you still see occasional missed lamps, increase to 15–20 ms.
    If you want snappier response and it's reliable, try 5 ms first.
    */
    delayMicroseconds(10000); 
}

void LivingColors::rgbToHsv(uint8_t r, uint8_t g, uint8_t b,
                            uint8_t &h, uint8_t &s, uint8_t &v) {
    uint8_t rgb_min = min(r, min(g, b));
    uint8_t rgb_max = max(r, max(g, b));

    v = rgb_max;

    if (v == 0) {
        h = s = 0;
        return;
    }

    s = 255 * (uint32_t)(rgb_max - rgb_min) / v;

    if (s == 0) {
        h = 0;
        return;
    }

    if (rgb_max == r) {
        h = 0 + 43 * (g - b) / (rgb_max - rgb_min);
    } else if (rgb_max == g) {
        h = 85 + 43 * (b - r) / (rgb_max - rgb_min);
    } else {
        h = 171 + 43 * (r - g) / (rgb_max - rgb_min);
    }
}

void LivingColors::hsvToRgb(uint8_t h, uint8_t s, uint8_t v,
                            uint8_t &r, uint8_t &g, uint8_t &b) {
    if (s == 0) {
        r = g = b = v;
        return;
    }

    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}
