#include "CC2500.h"

CC2500::CC2500(uint8_t csPin, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin,
               uint8_t gdo2Pin)
    : _csPin(csPin), _sckPin(sckPin), _misoPin(misoPin), _mosiPin(mosiPin),
      _gdo2Pin(gdo2Pin) {}

void CC2500::begin() {
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);

  pinMode(_gdo2Pin, INPUT_PULLUP);

  SPI.begin(_sckPin, _misoPin, _mosiPin, _csPin);

#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 1) {
    Debug::println(
        1, "[CC2500] Begin SPI: CS=" + String(_csPin) +
               " SCK=" + String(_sckPin) + " MISO=" + String(_misoPin) +
               " MOSI=" + String(_mosiPin) + " GDO2=" + String(_gdo2Pin));
  }
#endif
}

bool CC2500::verify() {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 1) {
    Debug::println(1, "[CC2500] Verifying chip...");
  }
#endif

  digitalWrite(_csPin, LOW);
  SPI.transfer(CC2500_CMD_SRES);
  digitalWrite(_csPin, HIGH);
  delay(10);

  uint8_t partnum = readStatusReg(CC2500_REG_PARTNUM);
  uint8_t version = readStatusReg(CC2500_REG_VERSION);

#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 1) {
    Debug::println(1, "[CC2500] PARTNUM=0x" + String(partnum, HEX) +
                          " VERSION=0x" + String(version, HEX));
  }
#endif

  return (partnum == 0x80 && version == 0x03);
}

void CC2500::initForLivingColors() {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 3) {
    Debug::println(1, "[CC2500] Init for LivingColors...");
  }
#endif

  sendStrobe(CC2500_CMD_SRES);
  delay(100);
  sendStrobe(CC2500_CMD_SRES);
  delay(100);

  writeReg(CC2500_REG_FSCTRL1, 0x09);
  writeReg(CC2500_REG_FSCTRL0, 0x00);
  writeReg(CC2500_REG_FREQ2, 0x5D);
  writeReg(CC2500_REG_FREQ1, 0x93);
  writeReg(CC2500_REG_FREQ0, 0xB1);

  writeReg(CC2500_REG_MDMCFG4, 0x2D);
  writeReg(CC2500_REG_MDMCFG3, 0x3B);
  writeReg(CC2500_REG_MDMCFG2, 0x73);
  writeReg(CC2500_REG_MDMCFG1, 0x22);
  writeReg(CC2500_REG_MDMCFG0, 0xF8);

  writeReg(CC2500_REG_CHANNR, 0x03);
  writeReg(CC2500_REG_DEVIATN, 0x00);

  writeReg(CC2500_REG_FREND1, 0xB6);
  writeReg(CC2500_REG_FREND0, 0x10);

  writeReg(CC2500_REG_MCSM0, 0x18);

  writeReg(CC2500_REG_FOCCFG, 0x1D);
  writeReg(CC2500_REG_BSCFG, 0x1C);
  writeReg(CC2500_REG_AGCCTRL2, 0xC7);
  writeReg(CC2500_REG_AGCCTRL1, 0x00);
  writeReg(CC2500_REG_AGCCTRL0, 0xB2);

  writeReg(CC2500_REG_FSCAL2, 0x0A);
  writeReg(CC2500_REG_FSCAL1, 0x00);
  writeReg(CC2500_REG_FSCAL0, 0x11);

  writeReg(CC2500_REG_TEST2, 0x88);
  writeReg(CC2500_REG_TEST1, 0x31);
  writeReg(CC2500_REG_TEST0, 0x0B);

  writeReg(CC2500_REG_IOCFG2, 0x06);
  writeReg(CC2500_REG_IOCFG0, 0x01);

  writeReg(CC2500_REG_PKTCTRL1, 0x04);
  writeReg(CC2500_REG_PKTCTRL0, 0x45);
  writeReg(CC2500_REG_ADDR, 0x00);
  writeReg(CC2500_REG_PKTLEN, 0xFF);
  writeReg(CC2500_REG_FIFOTHR, 0x0D);

  writeReg(CC2500_REG_PATABLE, 0xFF);

  sendStrobe(CC2500_CMD_SIDLE);

#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 3) {
    Debug::println(1, "[CC2500] LivingColors protocol configured...");
  }
#endif
}

void CC2500::setChannel(uint8_t channel) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 3) {
    Debug::println(2, "[CC2500] Set channel " + String(channel));
  }
#endif
  writeReg(CC2500_REG_CHANNR, channel);
  sendStrobe(CC2500_CMD_SIDLE);
}

void CC2500::manualCalibrate() {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 3) {
    Debug::println(2, "[CC2500] Manual calibration...");
  }
#endif
  sendStrobe(CC2500_CMD_SCAL);
  delayMicroseconds(200);
}

void CC2500::sendRaw(const uint8_t *data, uint8_t len) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 5) {
    String dbg = "[CC2500] sendRaw len=" + String(len) + " bytes:";
    for (uint8_t i = 0; i < len; i++) {
      dbg += " 0x" + String(data[i], HEX);
    }
    Debug::println(4, dbg);
    Debug::println(5, "[CC2500] GDO2 before TX = " +
                          String(digitalRead(_gdo2Pin)));
  }
#endif

  sendStrobe(CC2500_CMD_SIDLE);
  sendStrobe(CC2500_CMD_SFTX);

  digitalWrite(_csPin, LOW);
  SPI.transfer(CC2500_REG_TXFIFO | CC2500_OFF_WRITE_BURST);
  for (uint8_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_csPin, HIGH);

  sendStrobe(CC2500_CMD_STX);

#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 5) {
    Debug::println(3, "[CC2500] STX sent, GDO2 after TX = " +
                          String(digitalRead(_gdo2Pin)));
  }
#endif
}

bool CC2500::isGDO2High() const { return digitalRead(_gdo2Pin) == HIGH; }

void CC2500::waitForGDO2Low(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (digitalRead(_gdo2Pin) == HIGH) {
    yield();
    if (millis() - start > timeoutMs) {
#ifdef DEBUG_LEVEL
      if (Debug::getDebugLevel() >= 3) {
        Debug::println(1, "[CC2500] GDO2 timeout");
      }
#endif
      break;
    }
  }
}

uint8_t CC2500::readStatusReg(uint8_t addr) {
  digitalWrite(_csPin, LOW);
  SPI.transfer(addr | CC2500_OFF_READ_BURST);
  uint8_t val = SPI.transfer(0x00);
  digitalWrite(_csPin, HIGH);
  return val;
}

void CC2500::writeReg(uint8_t addr, uint8_t val) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 5) {
    Debug::println(3, "[CC2500] [writeReg] Addr=0x" + String(addr, HEX) +
                          " Val=0x" + String(val, HEX));
  }
#endif
  digitalWrite(_csPin, LOW);
  SPI.transfer(addr);
  SPI.transfer(val);
  digitalWrite(_csPin, HIGH);
}

void CC2500::sendStrobe(uint8_t strobe) {
#ifdef DEBUG_LEVEL
  if (Debug::getDebugLevel() >= 5) {
    Debug::println(3, "[CC2500] [sendStrobe] Strobe=0x" + String(strobe, HEX) +
                          " GDO2=" + String(digitalRead(_gdo2Pin)));
  }
#endif
  digitalWrite(_csPin, LOW);
  SPI.transfer(strobe);
  digitalWrite(_csPin, HIGH);
  delayMicroseconds(10);
}
