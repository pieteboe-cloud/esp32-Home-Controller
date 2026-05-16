#include "RF433.h"

RF433::RF433(int rxPin, int txPin) : rxPin(rxPin), txPin(txPin) {}

// Known codes:
// 4276245 / 24bit / protocol: 1 = main power group 1.04276244
// 5974596 / 24bit / Protocol: 5 = main power group 1.1
//    5397 / 24bit / Protocol: 1 = dimmer function



void RF433::init() {
    mySwitch.enableReceive(digitalPinToInterrupt(rxPin));
    mySwitch.enableTransmit(txPin);
    mySwitch.setProtocol(1); // Standard protocol for TX
    mySwitch.setReceiveTolerance(90);
    Debug::println("[INFO][RF433] RF433 module initialized on RX pin " + String(rxPin) + " and TX pin " + String(txPin));
}

void RF433::send(unsigned long code, int length) {
    mySwitch.send(code, length);
     if (Debug::isVerbose()) {
        Debug::println("[INFO][RF433] Sent code: " + String(code) + " / " + String(length) + " bits");
    }   
}

unsigned long RF433::receive() {
    if (mySwitch.available()) {
        unsigned long value = mySwitch.getReceivedValue();
         if (Debug::isVerbose()) {
            Debug::println("[INFO][RF433] Received code: " + String(value) + " / " + String(mySwitch.getReceivedBitlength()) + " bits / Protocol: " + String(mySwitch.getReceivedProtocol()));
        }   
        mySwitch.resetAvailable();
        return value;
    }
    return 0;
}

int RF433::getReceivedBitlength() {
     if (Debug::isVerbose()) {
        Debug::println("[INFO][RF433] Received bit length: " + String(mySwitch.getReceivedBitlength()));
    }   
    return mySwitch.getReceivedBitlength();
}

int RF433::getReceivedProtocol() {
     if (Debug::isVerbose()) {
        Debug::println("[INFO][RF433] Received protocol: " + String(mySwitch.getReceivedProtocol()));
    }           
    return mySwitch.getReceivedProtocol();
}