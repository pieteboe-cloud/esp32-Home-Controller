#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>
#include "IRController.h"
#include <Debug.h>

IRController::IRController(int rxPin, int txPin)
    : rxPin(rxPin), txPin(txPin)
{
}

void IRController::init()
{
    Debug::println("[INFO][IRController] Initializing IR Controller with RX pin " + String(rxPin) + " and TX pin " + String(txPin));
    IrReceiver.begin(rxPin, ENABLE_LED_FEEDBACK);
    IrSender.begin(txPin, DISABLE_LED_FEEDBACK);
}

void IRController::send(unsigned long code, int bits)
{
    if (bits <= 0) {
        bits = 32;
    }
    if (Debug::isVerbose()) {
        Debug::println("[INFO][IRController] Sending code: " + String(code) + " / " + String(bits) + " bits");
    }
    IrSender.sendNECMSB(code, bits);
}

void IRController::sendNECRaw(unsigned long code, int bits, int sendPin)
{
    if (sendPin < 0) {
        sendPin = txPin;
    }
    if (bits <= 0) {
        bits = 32;
    }
    if (Debug::isVerbose()) {
        Debug::println("[INFO][IRController] Sending raw NEC code: 0x" + String(code, HEX) + " / " + String(bits) + " bits / pin " + String(sendPin));
    }
    IrSender.setSendPin(sendPin);
    IrSender.sendNECMSB(code, bits);
}

unsigned long IRController::receive()
{
    if (IrReceiver.decode())
    {
        unsigned long value = IrReceiver.decodedIRData.decodedRawData;
        if (Debug::isVerbose()) {
            Debug::println("[INFO][IRController] Received code: 0x" + String(value, HEX) + " / " + String(IrReceiver.decodedIRData.numberOfBits) + " bits");
        }
        IrReceiver.resume();
        return value;
    }
    return 0;
}

int IRController::getReceivedBitlength()
{
    return IrReceiver.decodedIRData.numberOfBits;
}

int IRController::getReceivedProtocol()
{
    return (int)IrReceiver.decodedIRData.protocol;
}
