#ifndef RF433_H
#define RF433_H

#include <RCSwitch.h>
#include <Arduino.h>
#include "Debug.h"


class RF433 {
public:
    RF433(int rxPin, int txPin);
    void init();
    void send(unsigned long code, int length = 24);
    unsigned long receive();
    int getReceivedBitlength();
    int getReceivedProtocol();

private:
    RCSwitch mySwitch;
    int rxPin;
    int txPin;
};

#endif