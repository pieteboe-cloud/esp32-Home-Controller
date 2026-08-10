#pragma once
#include <Arduino.h>
#include "../Config.h"
#include "CC2500.h"

class LivingColors {
public:
    LivingColors(uint8_t csPin, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin, uint8_t gdo2Pin);

    void begin(bool wakeAllBlack = true);

    void setColor(uint8_t lampIndex, uint8_t h, uint8_t s, uint8_t v);
    void setColorRGB(uint8_t lampIndex, uint8_t r, uint8_t g, uint8_t b);
    void setBlack(uint8_t lampIndex);

    void setColorAll(uint8_t h, uint8_t s, uint8_t v);
    void setColorRGBAll(uint8_t r, uint8_t g, uint8_t b);
    void setBlackAll();

    void setColorBatch(const uint8_t* indices, uint8_t count,
                       const uint8_t* h, const uint8_t* s, const uint8_t* v);

    void setBlackBatch(const uint8_t* indices, uint8_t count);

    void powerOffHard(uint8_t lampIndex);
    void powerOffHardAll();
    void powerOffHardBatch(const uint8_t* indices, uint8_t count);

    static void rgbToHsv(uint8_t r, uint8_t g, uint8_t b,
                         uint8_t &h, uint8_t &s, uint8_t &v);

    static void hsvToRgb(uint8_t h, uint8_t s, uint8_t v,
                         uint8_t &r, uint8_t &g, uint8_t &b);

private:
    CC2500 radio;
    uint8_t sequence = 0;

    void sendPacket(uint8_t lampIndex,
                    uint8_t command,
                    uint8_t h, uint8_t s, uint8_t v);
};
