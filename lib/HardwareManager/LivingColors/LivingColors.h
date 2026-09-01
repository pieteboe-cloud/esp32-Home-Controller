#pragma once
#include <Arduino.h>
#include "../Config.h"
#include "CC2500.h"

/**
 * @class LivingColors
 * @brief Controls Philips LivingColors lamps using CC2500 radio module
 * 
 * This class provides methods to control multiple LivingColors lamps
 * through wireless communication using the CC2500 radio module.
 * Supports HSV and RGB color modes, individual and batch operations,
 * and power management.
 */
class LivingColors {
public:
    /**
     * @brief Initialize LivingColors controller with CC2500 radio module
     * @param csPin Chip Select pin for CC2500
     * @param sckPin SPI Clock pin
     * @param misoPin SPI Master In Slave Out pin
     * @param mosiPin SPI Master Out Slave In pin
     * @param gdo2Pin GDO2 pin for CC2500
     */
    LivingColors(uint8_t csPin, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin, uint8_t gdo2Pin);

    /**
     * @brief Initialize the LivingColors system
     * @param wakeAllBlack If true, turns all lamps black on startup
     */
    void begin(bool wakeAllBlack = true);

    /**
     * @brief Set lamp color using HSV values
     * @param lampIndex Index of the lamp (0 to LAMP_COUNT-1)
     * @param h Hue (0-255)
     * @param s Saturation (0-255)
     * @param v Value/Brightness (0-255)
     */
    void setColor(uint8_t lampIndex, uint8_t h, uint8_t s, uint8_t v);
    
    /**
     * @brief Set lamp color using RGB values
     * @param lampIndex Index of the lamp (0 to LAMP_COUNT-1)
     * @param r Red (0-255)
     * @param g Green (0-255)
     * @param b Blue (0-255)
     */
    void setColorRGB(uint8_t lampIndex, uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * @brief Turn lamp off (black)
     * @param lampIndex Index of the lamp (0 to LAMP_COUNT-1)
     */
    void setBlack(uint8_t lampIndex);

    /**
     * @brief Set all lamps to the same HSV color
     * @param h Hue (0-255)
     * @param s Saturation (0-255)
     * @param v Value/Brightness (0-255)
     */
    void setColorAll(uint8_t h, uint8_t s, uint8_t v);
    
    /**
     * @brief Set all lamps to the same RGB color
     * @param r Red (0-255)
     * @param g Green (0-255)
     * @param b Blue (0-255)
     */
    void setColorRGBAll(uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * @brief Turn all lamps off (black)
     */
    void setBlackAll();

    /**
     * @brief Set multiple lamps to different colors
     * @param indices Array of lamp indices example {0, 2, 4}
     * @param count Number of lamps to set example 0,2,4 = 3
     * @param h Array of hue values (0-255)
     * @param s Array of saturation values (0-255)
     * @param v Array of value/brightness values (0-255)
     */
    void setColorBatch(const uint8_t* indices, uint8_t count,
                       const uint8_t* h, const uint8_t* s, const uint8_t* v);

    /**
     * @brief Turn multiple lamps off (black)
     * @param indices Array of lamp indices example {0, 2, 4}
     * @param count Number of lamps to turn off example 0,2,4 = 3
     */
    void setBlackBatch(const uint8_t* indices, uint8_t count);

    /**
     * @brief Hard power off a lamp
     * @param lampIndex Index of the lamp (0 to LAMP_COUNT-1)
     */
    void powerOffHard(uint8_t lampIndex);
    
    /**
     * @brief Hard power off all lamps
     */
    void powerOffHardAll();
    
    /**
     * @brief Hard power off multiple lamps
     * @param indices Array of lamp indices
     * @param count Number of lamps to turn off
     */
    void powerOffHardBatch(const uint8_t* indices, uint8_t count);

    /**
     * @brief Convert RGB color to HSV
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     * @param h Output hue (0-255)
     * @param s Output saturation (0-255)
     * @param v Output value/brightness (0-255)
     */
    static void rgbToHsv(uint8_t r, uint8_t g, uint8_t b,
                         uint8_t &h, uint8_t &s, uint8_t &v);

    /**
     * @brief Convert HSV color to RGB
     * @param h Hue (0-255)
     * @param s Saturation (0-255)
     * @param v Value/Brightness (0-255)
     * @param r Output red value (0-255)
     * @param g Output green value (0-255)
     * @param b Output blue value (0-255)
     */
    static void hsvToRgb(uint8_t h, uint8_t s, uint8_t v,
                         uint8_t &r, uint8_t &g, uint8_t &b);

private:
    CC2500 radio;              ///< CC2500 radio module for wireless communication
    uint8_t sequence = 0;      ///< Sequence counter for packet numbering

    /**
     * @brief Send a control packet to a lamp
     * @param lampIndex Index of the target lamp
     * @param command Command type (0x03 for color, 0x07 for power off)
     * @param h Hue (0-255)
     * @param s Saturation (0-255)
     * @param v Value/Brightness (0-255)
     */
    void sendPacket(uint8_t lampIndex,
                    uint8_t command,
                    uint8_t h, uint8_t s, uint8_t v);
};
