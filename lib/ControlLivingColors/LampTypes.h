#ifndef LAMP_TYPES_H
#define LAMP_TYPES_H

#include <Arduino.h>

struct LampState {
    uint8_t h;
    uint8_t s;
    uint8_t v;
};

#endif