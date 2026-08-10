#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "CC2500_Registers.h"
#include "../../Debug/Debug.h"


class CC2500 {
public:
    CC2500(uint8_t csPin,
           uint8_t sckPin,
           uint8_t misoPin,
           uint8_t mosiPin,
           uint8_t gdo2Pin);

    void begin();
    bool verify();
    void initForLivingColors();

    void setChannel(uint8_t channel);
    void manualCalibrate();

    void sendRaw(const uint8_t *data, uint8_t len);
    void readRaw(uint8_t *buf, uint8_t &len);

    bool isGDO2High() const;
    void waitForGDO2Low(uint32_t timeoutMs);

private:
    uint8_t _csPin;
    uint8_t _sckPin;
    uint8_t _misoPin;
    uint8_t _mosiPin;
    uint8_t _gdo2Pin;

    uint8_t readStatusReg(uint8_t addr);
    void writeReg(uint8_t addr, uint8_t val);
    void sendStrobe(uint8_t strobe);
};
