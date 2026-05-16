#include "CC2500.h"

// DO NOT MODIFY !! 
// These are the register addresses and command strobes for the CC2500 chip
// livingcolors uses these specific registers and commands to control the lamps, so changing them will break functionality


CC2500::CC2500(uint8_t csPin, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin) {
    _csPin   = csPin;
    _sckPin  = sckPin;
    _misoPin = misoPin;
    _mosiPin = mosiPin;
}

void CC2500::begin() {
    Debug::println("[INFO][CC2500] Begin...");
//    Debug::setVerbose( true); // Enable verbose logging for CC2500 initialization

    pinMode(_csPin, OUTPUT); 
    digitalWrite(_csPin, HIGH);
    SPI.begin(_sckPin, _misoPin, _mosiPin, _csPin);
    
    Debug::println("[INFO][CC2500] SPI pins  SCK=" + String(_sckPin) + " MISO=" + String(_misoPin) + " MOSI=" + String(_mosiPin) + " CS=" + String(_csPin));   

    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    
    Debug::println("[INFO][CC2500] SPI 4000000, MSBFIRST, SPI_MODE0 " );

    verify();
    init();
}

bool CC2500::verify() {
    Debug::println("[INFO][CC2500] Verifying chip...");

    byte partnum = readStatusReg(CC2500_REG_PARTNUM);
    byte version = readStatusReg(CC2500_REG_VERSION);

    Debug::println("[INFO][CC2500] PARTNUM: 0x"+ String(partnum, HEX));
    Debug::println("[INFO][CC2500] VERSION: 0x"+ String(version, HEX));


    if (partnum == 0x80 && version == 0x03) {
        Debug::println("[INFO][CC2500] Chip detected and correct version");
        return true;
    }

    Debug::println("[ERROR][CC2500] ✖ Chip NOT detected or wrong version!");
    Debug::println("[ERROR][CC2500] Check wiring: CS, SCK, MISO, MOSI");
    Debug::println("[ERROR][CC2500] Check power: 3.3V only (NO 5V!)");
    Debug::println("[ERROR][CC2500] Check GND connection");

    return false;
}



void CC2500::setRXMode() { //
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] Setting RX mode");
    }
    sendStrobe(0x36);   // SIDLE
    sendStrobe(0x33);   // SCAL
    sendStrobe(0x34);   // SRX
}

void CC2500::setChannel(byte channel) {
    writeReg(CC2500_REG_CHANNR, channel);  // CHANNR 
    sendStrobe(CC2500_CMD_SIDLE);         // Exit current state to apply channel change
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] Channel set to " + String(channel));
    }       
}

bool CC2500::dataReady() {
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] Checking for data...");
    }
    byte rxbytes = readStatusReg(0x3B);  // RXBYTES register has the number of bytes in RX FIFO,
                                         // and the high bit is set if there was an overflow 
                                         // (more bytes received than FIFO can hold)
    return rxbytes > 0;
}

// 
byte CC2500::readMarcState() {
    byte marcState = readStatusReg(CC2500_REG_MARCSTATE) & 0x1F;
    
    // Only log state changes to avoid flooding the serial buffer 
    static byte lastState = 0xFF;
    if (Debug::isVerbose() && marcState != lastState) {
       lastState = marcState;
       switch (marcState) {
    case 0x00: Debug::println("[INFO][CC2500] MARCSTATE: SLEEP"); break;
    case 0x01: Debug::println("[INFO][CC2500] MARCSTATE: IDLE"); break;
    case 0x02: Debug::println("[INFO][CC2500] MARCSTATE: XOFF"); break;
    case 0x03: Debug::println("[INFO][CC2500] MARCSTATE: VCOON_MC"); break;
    case 0x04: Debug::println("[INFO][CC2500] MARCSTATE: REGON_MC"); break;
    case 0x05: Debug::println("[INFO][CC2500] MARCSTATE: MANCAL"); break;
    case 0x06: Debug::println("[INFO][CC2500] MARCSTATE: VCOON"); break;
    case 0x07: Debug::println("[INFO][CC2500] MARCSTATE: REGON"); break;
    case 0x08: Debug::println("[INFO][CC2500] MARCSTATE: STARTCAL"); break;
    case 0x09: Debug::println("[INFO][CC2500] MARCSTATE: BWBOOST"); break;
    case 0x0A: Debug::println("[INFO][CC2500] MARCSTATE: FS_LOCK"); break;
    case 0x0B: Debug::println("[INFO][CC2500] MARCSTATE: IFADCON"); break;
    case 0x0C: Debug::println("[INFO][CC2500] MARCSTATE: ENDCAL"); break;
    case 0x0D: Debug::println("[INFO][CC2500] MARCSTATE: RX"); break;
    case 0x0E: Debug::println("[INFO][CC2500] MARCSTATE: RX_END"); break;
    case 0x0F: Debug::println("[INFO][CC2500] MARCSTATE: RX_RST"); break;
    case 0x10: Debug::println("[INFO][CC2500] MARCSTATE: TXRX_SWITCH"); break;
    case 0x11: Debug::println("[INFO][CC2500] MARCSTATE: RXFIFO_OVERFLOW"); break;
    case 0x12: Debug::println("[INFO][CC2500] MARCSTATE: FSTXON"); break;
    case 0x13: Debug::println("[INFO][CC2500] MARCSTATE: TX"); break;
    case 0x14: Debug::println("[INFO][CC2500] MARCSTATE: TX_END"); break;
    case 0x15: Debug::println("[INFO][CC2500] MARCSTATE: RXTX_SWITCH"); break;
    case 0x16: Debug::println("[INFO][CC2500] MARCSTATE: TXFIFO_UNDERFLOW"); break;
    default: Debug::println("[INFO][CC2500] MARCSTATE: UNKNOWN"); break;
    }
    }
    return marcState;
}

int CC2500::readRSSI() {
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] Reading RSSI...");
    }
    byte rssi = readStatusReg( 0x34 );  // read RSSI from PKTSTATUS register
    delay(10);

    if (rssi >= 128) {
        return (rssi - 256) / 2 - 74;
    } else {
        return rssi / 2 - 74;
    }
}

// private methods
byte CC2500::readStatusReg(byte addr) {
    byte val;
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr | 0xC0);
    val = SPI.transfer(0x00);
    digitalWrite(_csPin, HIGH);
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] Read Reg 0x" + String(addr, HEX) + " = 0x" + String(val, HEX));
    }
    return val;
}

void CC2500::writeReg(byte addr, byte val) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr);
    SPI.transfer(val);
    digitalWrite(_csPin, HIGH);
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] Write Reg 0x" + String(addr, HEX) + " = 0x" + String(val, HEX));
    }
}

void CC2500::sendStrobe(byte strobe) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(strobe);
    digitalWrite(_csPin, HIGH);
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] Sent Strobe 0x" + String(strobe, HEX));
    }
}

bool CC2500::waitForIdle(unsigned long timeoutUs) {
    unsigned long start = micros();
    while ((readMarcState() & 0x1F) != 0x01) {
        if (micros() - start >= timeoutUs) {
            Debug::println("[WARN][CC2500] waitForIdle timeout");
            return false;
        }
        yield(); // Allow background tasks to run while waiting
    }
    return true;
}

byte CC2500::bytesAvailable() {
    Debug::println("[INFO][CC2500] Checking bytes available in RX FIFO...");
    return readStatusReg(0x3B) & 0x7F;
}


void CC2500::sendPacket(uint8_t* data, uint8_t len) {
    Debug::println("[INFO][CC2500] Sending single packet...");

    // Put radio into IDLE and flush the TX FIFO
    sendStrobe(CC2500_CMD_SIDLE);
    sendStrobe(CC2500_CMD_SFTX);
    waitForIdle(2000);

    // Log packet before transmission
    if (Debug::isVerbose()) {
        Debug::print("[INFO][CC2500] TX Packet:");
        for (uint8_t i = 0; i < len; i++) {
            Debug::print(" 0x" + String(data[i], HEX));
        }
        Debug::println("");
    }

    // Burst write the packet to the TX FIFO
    digitalWrite(_csPin, LOW);
    SPI.transfer(CC2500_REG_TXFIFO | CC2500_OFF_WRITE_BURST);
    for (uint8_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(_csPin, HIGH);

    // Strobe STX to start transmission
    // Note: FS_AUTOCAL is enabled in MCSM0, so calibration happens automatically
    sendStrobe(CC2500_CMD_STX);
    if (!waitForIdle(5000)) {
        delayMicroseconds(800);
    }
}


void CC2500::sendPackets(uint8_t** packets, uint8_t* lens, uint8_t count) {
    if (count == 0) return;

    //Debug::println("[INFO][CC2500] Sending batch of " + String(count) + " packets");

    for (uint8_t i = 0; i < count; i++) {
        // 1. Ensure radio is IDLE and FIFO is clear before each packet
        sendStrobe(CC2500_CMD_SIDLE);
        sendStrobe(CC2500_CMD_SFTX);
        if (!waitForIdle(1000)) continue; 

        // Write packet to TX FIFO
        digitalWrite(_csPin, LOW);
        SPI.transfer(CC2500_REG_TXFIFO | CC2500_OFF_WRITE_BURST);
        for (uint8_t j = 0; j < lens[i]; j++) {
            SPI.transfer(packets[i][j]);
        }
        digitalWrite(_csPin, HIGH);

        // Fire!
        sendStrobe(CC2500_CMD_STX);
        
        // 4. Wait for completion (increased timeout for auto-calibration)
        if (!waitForIdle(5000)) {
            Debug::println("[WARN][CC2500] Packet " + String(i) + " timeout");
        }

        // 5. Inter-packet gap (Crucial for LivingColors lamps)
        // This provides the delay that verbose logging was "accidentally" providing.
        delayMicroseconds(500); 
        yield();
    }
}

void CC2500::readPacket(uint8_t *buf, uint8_t &len) {
    byte rxbytes = readStatusReg(CC2500_REG_RXBYTES);
    
    if ((rxbytes & 0x80) || ((rxbytes & 0x7F) == 0)) {
        len = 0;
        if (rxbytes & 0x80) {
            sendStrobe(CC2500_CMD_SIDLE);
            sendStrobe(CC2500_CMD_SFRX);
            setRXMode();
        }
        return;
    }

    uint8_t available = rxbytes & 0x7F;
    uint8_t toRead = (available < len) ? available : len;

    digitalWrite(_csPin, LOW);
    SPI.transfer(CC2500_REG_RXFIFO | CC2500_OFF_READ_BURST);
    for (uint8_t i = 0; i < available; i++) {
        uint8_t val = SPI.transfer(0x00);
        if (i < toRead) buf[i] = val;
    }
    digitalWrite(_csPin, HIGH);
    len = toRead;

    sendStrobe(CC2500_CMD_SIDLE);
    sendStrobe(CC2500_CMD_SFRX);
    setRXMode();
}

void CC2500::readPacket() {
    byte rxbytes = readStatusReg(CC2500_REG_RXBYTES);
    
    // Hard reset if overflow or no bytes, to clear the FIFO and get back to a known state
    if ((rxbytes & 0x80) || rxbytes > 64 || rxbytes == 0) {
        sendStrobe(CC2500_CMD_SIDLE);
        sendStrobe(CC2500_CMD_SFRX);
        setRXMode();
        return;
    }

    digitalWrite(_csPin, LOW);
    SPI.transfer(CC2500_REG_RXFIFO | CC2500_OFF_READ_BURST);
    byte len = SPI.transfer(0x00);

    // LivingColors packets are always 15 bytes long (1 length byte + 14 data bytes)
    // Validate packet length (should be at least 1 and not exceed available bytes - 1 for the length byte)
    if (len == 0 || len > (rxbytes - 1) || len > 64) {
        digitalWrite(_csPin, HIGH);
        sendStrobe(CC2500_CMD_SIDLE);
        sendStrobe(CC2500_CMD_SFRX);
        setRXMode();
        return;
    }

    // Read the actual packet data based on the length byte
    uint8_t packet[len + 1];  // +1 for the length byte itself
    packet[0] = len;
    for (byte i = 1; i <= len; i++) {
        packet[i] = SPI.transfer(0x00);
    }
        
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CC2500] RX Packet: " + String  (packet, len + 1)) ;
    }

    digitalWrite(_csPin, HIGH);

    // After reading flash the RX FIFO to clear any remaining bytes and avoid overflow issues
    sendStrobe(CC2500_CMD_SIDLE);
    sendStrobe(CC2500_CMD_SFRX);
    setRXMode();

}





void CC2500::readPacketTest() {
    Debug::println("[INFO][CC2500] TEST Reading packet...");

    byte rxbytes = readStatusReg(CC2500_REG_RXBYTES) & 0x7F; // Filter overflow bit
    
    // Overflow?
    if (rxbytes & 0x80) {
        Debug::println("[ERROR][CC2500] TEST RX FIFO Overflow detected! Flushing..."); 
        sendStrobe(CC2500_CMD_SIDLE);
        sendStrobe(CC2500_CMD_SFRX);
        setRXMode();
        return;
    }

    // No packet?
    if (rxbytes == 0) {
        Debug::println("[INFO][CC2500] TEST  No packets in FIFO"); 
        return;
    }               


    if (rxbytes > 0) {
        digitalWrite(_csPin, LOW);
        SPI.transfer(CC2500_REG_RXFIFO | CC2500_OFF_READ_BURST);
        
        Debug::println("[INFO][CC2500] TEST Packet received! Raw Data " +    String(rxbytes) + String(" bytes): "));

        for (byte i = 0; i < rxbytes; i++) {
            byte b = SPI.transfer(0x00);
            Debug::println("[INFO][CC2500] TEST Byte " + String(i) );//+ ": " + String(b, HEX) + " " );
        }
        digitalWrite(_csPin, HIGH);
        
        // always flush to avoid getting stuck in a loop if there is a bad packet that causes overflow
        sendStrobe(CC2500_CMD_SIDLE);
        sendStrobe(CC2500_CMD_SFRX);
        setRXMode();
}
}


void CC2500::init() {
    
     Debug::println("[INFO][CC2500] Setting up CC2500 for livingcolor protocol...");

    sendStrobe(CC2500_CMD_SRES);   // SRES reset
    delay(100);
    sendStrobe(CC2500_CMD_SRES);   // SRES reset
    delay(100);

    // write configuration register: FSCTRL1 – Frequency Synthesizer Control
    writeReg(CC2500_REG_FSCTRL1, 0x09); // fxtal = 26MHz 
    Debug::println("[INFO][CC2500] FSCTRL1 set to 0x09 for 26MHz crystal");

    // write configuration register: FSCTRL0 – Frequency Synthesizer Control
    writeReg(CC2500_REG_FSCTRL0, 0x00); // fxtal = 26MHz
    Debug::println("[INFO][CC2500] FSCTRL0 set to 0x00 for 26MHz crystal");

    // write configuration register: FREQ2 – Frequency Control Word, High Byte
    writeReg(CC2500_REG_FREQ2, 0x5D); 
    Debug::println("[INFO][CC2500] FREQ2 set to 0x5D");

    // write configuration register: FREQ1 – Frequency Control Word, Middle Byte
    writeReg(CC2500_REG_FREQ1, 0x93); 
    Debug::println("[INFO][CC2500] FREQ1 set to 0x93");
    
    // write configuration register: FREQ0 – Frequency Control Word, Low Byte
    writeReg(CC2500_REG_FREQ0, 0xB1); // 2433.00 MHz (LivingColors Kanaal 0)
    Debug::println("[INFO][CC2500] FREQ0 set to 0xB1 for 2433.00 MHz");

    // write configuration register: MDMCFG4 – Modem Configuration
    writeReg(CC2500_REG_MDMCFG4, 0x2D);
    Debug::println("[INFO][CC2500] MDMCFG4 set to 0x2D");

    // write configuration register: MDMCFG3 – Modem Configuration
    writeReg(CC2500_REG_MDMCFG3, 0x3B);
    Debug::println("[INFO][CC2500] MDMCFG3 set to 0x3B");

    // write configuration register: MDMCFG2 – Modem Configuration
    writeReg(CC2500_REG_MDMCFG2, 0x73); // GFSK, 30/32 sync word bits detect
    Debug::println("[INFO][CC2500] MDMCFG2 set to 0x73 for GFSK, 30/32 sync word bits detect ");
    
    // write configuration register: MDMCFG1 – Modem Configuration
    writeReg(CC2500_REG_MDMCFG1, 0x22); // 250kbps
    Debug::println("[INFO][CC2500] MDMCFG1 set to 0x22 for 250kbps");

    // write configuration register: MDMCFG0 – Modem Configuration
    writeReg(CC2500_REG_MDMCFG0, 0xF8); 
    Debug::println("[INFO][CC2500] MDMCFG0 set to 0xF8");
    
    // write configuration register: CHANNR – Channel Number
    writeReg(CC2500_REG_CHANNR, 0x03); // Kanaal 3 (2440.00 MHz) is ook veel gebruikt door LivingColors, maar kanaal 0 (2433.00 MHz) is het meest betrouwbaar in mijn tests
    Debug::println("[INFO][CC2500] CHANNR set to 0x03 for 2440.00 MHz");

    // write configuration register: DEVIATN – Modem Deviation Setting
    writeReg(CC2500_REG_DEVIATN, 0x00); // 5.1 kHz
    Debug::println("[INFO][CC2500] DEVIATN set to 0x00 for 5.1 kHz");

    // write configuration register: FREND1 – Front End RX Configuration
    writeReg(CC2500_REG_FREND1, 0xB6); // set to 0xB6 for normal operation instead of production testing
    Debug::println("[INFO][CC2500] FREND1 set to 0xB6 for normal operation instead of production testing");

    // write configuration register: FREND0 – Front End TX configuration
    writeReg(CC2500_REG_FREND0, 0x10); // set to 0x10 for normal operation instead of production testing
    Debug::println("[INFO][CC2500] FREND0 set to 0x10 for normal operation instead of production testing");

    // write configuration register: MCSM0 – Main Radio Control State Machine Configuration
    writeReg(CC2500_REG_MCSM0, 0x18); // set to 0x18 for normal operation instead of production testing
    Debug::println("[INFO][CC2500] MCSM0 set to 0x18 for normal operation instead of production testing");

    
    // write configuration register: FOCCFG – Frequency Offset Compensation Configuration
    writeReg(CC2500_REG_FOCCFG, 0x1D);  
    Debug::println("[INFO][CC2500] FOCCFG set to 0x1D");

    // write configuration register: BSCFG – Bit Synchronization Configuration
    writeReg(CC2500_REG_BSCFG, 0x1C); 
    Debug::println("[INFO][CC2500] BSCFG set to 0x1C");

    // write configuration register: AGCCTRL2 – AGC Control
    writeReg(CC2500_REG_AGCCTRL2, 0xC7); 
    Debug::println("[INFO][CC2500] AGCCTRL2 set to 0xC7");

    // write configuration register: AGCCTRL1 – AGC Control
    writeReg(CC2500_REG_AGCCTRL1, 0x00);
    Debug::println("[INFO][CC2500] AGCCTRL1 set to 0x00");

    // write configuration register: AGCCTRL0 – AGC Control 
    writeReg(CC2500_REG_AGCCTRL0, 0xB2); //   
    Debug::println("[INFO][CC2500] AGCCTRL0 set to 0xB2");

    // write configuration register: FSCAL2 – Frequency Synthesizer Calibration
    writeReg(CC2500_REG_FSCAL2, 0x0A); 
    Debug::println("[INFO][CC2500] FSCAL2 set to 0x0A");

    // write configuration register: FSCAL1 – Frequency Synthesizer Calibration
    writeReg(CC2500_REG_FSCAL1, 0x00); // set to 0x00 for normal operation instead of production testing    
    Debug::println("[INFO][CC2500] FSCAL1 set to 0x00");

    // write configuration register: FSCAL0 – Frequency Synthesizer Calibration
    writeReg(CC2500_REG_FSCAL0, 0x11); // set to 0x11 for normal operation instead of production testing
    Debug::println("[INFO][CC2500] FSCAL0 set to 0x11");

    // write configuration register: TEST2 – Various Test Settings
    writeReg(CC2500_REG_TEST2, 0x88); // set to 0x88 for normal operation instead of production testing 
    Debug::println("[INFO][CC2500] TEST2 set to 0x88");

        // write configuration register: TEST1 – Various Test Settings
    writeReg(CC2500_REG_TEST1, 0x31); // set to 0x31 for normal operation instead of production testing
    Debug::println("[INFO][CC2500] TEST1 set to 0x31");

    // write configuration register: TEST0 – Various Test Settings
    writeReg(CC2500_REG_TEST0, 0x0B); // set to 0x0B for normal operation instead of production testing
    Debug::println("[INFO][CC2500] TEST0 set to 0x0B");

    // write configuration register: IOCFG2 – GDO2 Output Pin Configuration
    writeReg(CC2500_REG_IOCFG2, 0x06); // GDO2 output pin is configured to assert when sync word is sent/received,
                                        // and de-assert when packet is finished
    Debug::println("[INFO][CC2500] IOCFG2 set to 0x06");

    // write configuration register: IOCFG0 – GDO0 Output Pin Configuration
    writeReg(CC2500_REG_IOCFG0, 0x01); // GDO0 output is not used, so set to default value which is "high impedance"
    Debug::println("[INFO][CC2500] IOCFG0 set to 0x01");

    // write configuration register: PKTCTRL1 – Packet Automation Control
    writeReg(CC2500_REG_PKTCTRL1, 0x04);  // enable CRC check, append status bytes to the end of the payload (RSSI and LQI)
    Debug::println("[INFO][CC2500] PKTCTRL1 set to 0x04");

    // write configuration register: PKTCTRL0 – Packet Automation Control
    writeReg(CC2500_REG_PKTCTRL0, 0x45);  // enable CRC, use variable length packets
    Debug::println("[INFO][CC2500] PKTCTRL0 set to 0x45");

    // write configuration register: ADDR – Device Address
    writeReg(CC2500_REG_ADDR, 0x00); // we will use the first byte of the payload to specify actual destination address, so set this to 0x00
    Debug::println("[INFO][CC2500] ADDR set to 0x00");

    // write configuration register: PKTLEN – Packet Length
    writeReg(CC2500_REG_PKTLEN, 0xFF); // max packet length, we will use the first byte of the payload to specify actual length of each packet
    Debug::println("[INFO][CC2500] PKTLEN set to 0xFF");

    // write configuration register: FIFOTHR – RX FIFO and TX FIFO Thresholds
    writeReg(CC2500_REG_FIFOTHR, 0x0D);
    Debug::println("[INFO][CC2500] FIFOTHR set to 0x0D");

    // write power setting to PATABLE memory using single access write. See table 31 on page 47
    // of datasheet original value is 0xA9    
    writeReg(CC2500_REG_PATABLE, 0xFF);
    Debug::println("[INFO][CC2500] PATABLE set to 0xFF");

   
    sendStrobe(CC2500_CMD_SIDLE); 
    Debug::println("[INFO][CC2500] CC2500 SIDLE");

    
    Debug::println("[INFO][CC2500] initialized livingcolor protocol");


}


void CC2500::setPromiscuousMode() {
    Debug::println("[INFO][CC2500] Setting Promiscuous Mode...");

    // PKTCTRL1 (0x08): 
    // - Schakel Address Check uit (bits 0-1 op 00)
    // - Laat Append Status aan staan (bit 2 op 1) voor RSSI info
    writeReg(CC2500_REG_PKTCTRL1, 0x04); 

    // PKTCTRL0 (0x07):
    // - Zet CRC_EN uit (bit 2 op 0) -> Ontvang ook corrupte pakketten
    // - Zet op Variable Packet Length (bits 0-1 op 01)
    writeReg(CC2500_REG_PKTCTRL0, 0x05); 
    
    Debug::println("[INFO][CC2500] Promiscuous Mode:  Address Check OFF, CRC Check OFF");

}
