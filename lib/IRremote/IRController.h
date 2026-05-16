#ifndef IR_CONTROLLER_H
#define IR_CONTROLLER_H

#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>

// NEC Address controller
// ADDRESS = 0xEF00;

class IRController
{
public:
    IRController(int rxPin, int txPin);
    void init();
    void send(unsigned long code, int bits = 32);
    void sendNECRaw(unsigned long code, int bits = 32, int sendPin = -1);
    unsigned long receive();
    int getReceivedBitlength();
    int getReceivedProtocol();

private:
    int rxPin;
    int txPin;
};

#endif
