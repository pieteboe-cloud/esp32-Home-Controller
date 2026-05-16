#pragma once
#include <Arduino.h>
#include "CC2500.h"
#include "Debug.h"



/* LAMPS POSITIONS
________________________________________________________________________
|                    3                 4                5                |
|                                                                        |
|  2                                                                  6  |
|                                                                        |
|                                                                        |
|                                                                        |
\                     LIVING                                          7  |
  \                                                                      |
   \                                                                     |
|                      1                                                 |
_________________________                                                |
| = = = = = = = = = = = |              [X] -COUCH (I sit here)           |
| = = OTHER ROOM= = = = |                                                |
| = = = = = = = = = = = | 11        10            9                      |
| = = = = = = = = = = = |____________________________                    |   
|= = = = = = = = = = =  = = = = = = = = = = = = = = |  8                 |                  
|=  OTHER ROOM   = = =  = OTHER ROOM= = = = = = = = |                    |                                                   |                  |
|= = = = = = = = = = =  = = = = = = = = = = = = = = |                    |                                                   |                  |
|___________________________________________________|____________________|
*/


/* 
Message setup

Position   Length   Information
===================================================
0000-0000  1        Length of the message (always 0x10 for 16 data bytes)
0001-0004  4        Destination - to address
0005-0008  4        Source - from address
0009-0009  1        port ? (always 0x11)
000A-000A  1        Command (see below table)
000B-000B  1        tractid: Sequence number byte
000C-000E  3        Payload  (HSV) each 1 byte
000F-0010  2        CRC-16

Commands	Description	Comment
0x01	?? Ping / Identify / search	Is seen when no Ack from lamp is received
0x03	Set HSV color	payload is H,S,V bytes
0x04	Set HSV color Ack	payload is H,S,V bytes
0x05	Switch Lamp On	payload is H,S,V bytes
0x06	Switch Lamp On Ack	payload is H,S,V bytes
0x07	Switch Lamp Off	payload is 0,0,0
0x08	Switch Lamp Off Ack	payload is 0,0,0
0x0C	Color Changing/rotating mode. (Seen when holding ON)	payload is G,S,V

After each command the sequence byte is increased with 1 digit 
Lamp will acknoledge with the command bit +1 (0x04 is ack of command 0x03). 

    Destination address FF FF FF FF is broadcast 
    // rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b,
    // unsigned char *h, unsigned char *s, unsigned char *v);

    // rgb_color {
    // unsigned char r, g, b;    /* Channel intensities between 0 and 255 

    
    // hsv_color {
    hue;        /* Hue degree between 0 and 255 
    sat;        /* Saturation between 0 (gray) and 255 
    val;        /* Value between 0 (black) and 255 


*/

// ---------------- Full lamp code ----------------
// 11 lamps, 9 bytes each
// First 4 bytes is remote address, 
// next 4 bytes is lamp address, 
// then 1 byte is always 0x11 (unknown purpose)
// ---------------------------------------------

class ControlLivingColors {
public:
    ControlLivingColors(CC2500 &radio);

    // Main API
    void setColor(const uint8_t *lampAddr,
                  const uint8_t *remoteAddr,
                  uint8_t h, uint8_t s, uint8_t v);

    void turnOnWhite(const uint8_t *lampAddr);
    void turnOff(const uint8_t *lampAddr);
    void turnOn(const uint8_t *lampAddr);
    // Turn on lamp with specific color to avoid white flash
    void turnOnWithColor(const uint8_t *lampAddr, uint8_t h, uint8_t s, uint8_t v);
    void sniffForLamp();

    // Batch: send to multiple lamps in one burst
    void turnOnWithColorBatch(const uint8_t *lampAddrs[], uint8_t count, const uint8_t h[], const uint8_t s[], const uint8_t v[]);
    void turnOffBatch(const uint8_t *lampAddrs[], uint8_t count);
    
private:
    CC2500 &radio;
    uint8_t sequence = 0;

    void sendPacket(const uint8_t *lampAddr,
                    uint8_t command,
                    uint8_t h, uint8_t s, uint8_t v);
};
