#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// RF433 Pins
#define PIN_RF433_RX    26
#define PIN_RF433_TX    33

// IR Zone Pins
#define PIN_IR_RX       14
#define PIN_IR_TX       4

#define PIN_IR_BOTTOM   25
#define PIN_IR_MID      27
#define PIN_IR_UPPER    14

// CC2500 SPI Pins
#define PIN_CC2500_SCK  18
#define PIN_CC2500_MISO 19
#define PIN_CC2500_MOSI 23
#define PIN_CC2500_CS   5

// System LEDs (Inverted logic: LOW = ON)
#define PIN_STATUS_LED  2
#define PIN_RUNNING_LED 32

#endif