#include "ControlLivingColors.h"

// Packet structure constants
const uint8_t PACKET_LENGTH = 0x0E; // 14 payload bytes after the length byte
const uint8_t PACKET_SIZE = 15;     // 1 length + 14 data
const uint8_t POS_LENGTH = 0;
const uint8_t POS_DEST_START = 1;
const uint8_t POS_DEST_END = 4;
const uint8_t POS_SRC_START = 5;
const uint8_t POS_SRC_END = 8;
const uint8_t POS_PORT = 9;
const uint8_t POS_COMMAND = 10;
const uint8_t POS_SEQUENCE = 11;
const uint8_t POS_PAYLOAD_START = 12;

// Commands
const uint8_t CMD_SET_COLOR = 0x03; // Use the actual LivingColors HSV command for color updates
const uint8_t CMD_TURN_ON = 0x05;
const uint8_t CMD_TURN_OFF = 0x07;
const uint8_t CMD_COLOR_CHANGING = 0x0C;

ControlLivingColors::ControlLivingColors(CC2500 &r)
    : radio(r)
{
}

void ControlLivingColors::setColor(const uint8_t *lampAddr,
                                   const uint8_t *remoteAddr,
                                   uint8_t h, uint8_t s, uint8_t v)
{
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CONTROL] setColor H=" + String(h) +
                          " S=" + String(s) +
                          " V=" + String(v));
    }

    // Use CMD_TURN_ON to ensure the lamp wakes up if it was off
    sendPacket(lampAddr, CMD_TURN_ON, h, s, v);
}

void ControlLivingColors::turnOnWhite(const uint8_t *lampAddr)
{
    if (Debug::isVerbose())
        Debug::println("[INFO][CONTROL] turnOnWhite");

    sendPacket(lampAddr, CMD_TURN_ON, 0, 0, 255);
}

void ControlLivingColors::turnOnWithColor(const uint8_t *lampAddr, uint8_t h, uint8_t s, uint8_t v)
{
    if (Debug::isVerbose()) {
        Debug::println("[INFO][CONTROL] turnOnWithColor H=" + String(h) +
                          " S=" + String(s) +
                          " V=" + String(v));
    }
    // Use CMD_TURN_ON to ensure the lamp wakes up
    sendPacket(lampAddr, CMD_TURN_ON, h, s, v);
}

void ControlLivingColors::turnOff(const uint8_t *lampAddr)
{
    if (Debug::isVerbose())
        Debug::println("[INFO][CONTROL] turnOff");

    sendPacket(lampAddr, CMD_TURN_OFF, 0, 0, 0);
}

void ControlLivingColors::turnOn(const uint8_t *lampAddr)
{
    if (Debug::isVerbose())
        Debug::println("[INFO][CONTROL] turnOn");

    sendPacket(lampAddr, CMD_TURN_ON, 0, 0, 255); 
}

void ControlLivingColors::sendPacket(const uint8_t *lampAddr,
                                     uint8_t command,
                                     uint8_t h, uint8_t s, uint8_t v)
{
    uint8_t data[PACKET_SIZE];

    // Packet length (16 data bytes)
    data[POS_LENGTH] = PACKET_LENGTH;

    // 9-byte lamp address (destination 4, source 4, port 1)
    for (int i = 0; i < 9; i++)
        data[POS_DEST_START + i] = lampAddr[i];

    // Command
    data[POS_COMMAND] = command;
    
    // Sequence number
    data[POS_SEQUENCE] = sequence++;
    
    // HSV payload
    data[POS_PAYLOAD_START] = h;
    data[POS_PAYLOAD_START + 1] = s;
    data[POS_PAYLOAD_START + 2] = v;

    if (Debug::isVerbose()) {
        Debug::println("[INFO][CONTROL] TX " + Debug::hex(data, PACKET_SIZE));
    }

    radio.sendPacket(data, PACKET_SIZE);
}

void ControlLivingColors::turnOnWithColorBatch(const uint8_t *lampAddrs[], uint8_t count, const uint8_t h[], const uint8_t s[], const uint8_t v[]) {
    // Build all packets first, then send in one burst
    uint8_t packets[11][PACKET_SIZE];
    uint8_t* packetPtrs[11];
    uint8_t lens[11];

    for (uint8_t i = 0; i < count; i++) {
        packets[i][POS_LENGTH] = PACKET_LENGTH;
        for (int j = 0; j < 9; j++)
            packets[i][POS_DEST_START + j] = lampAddrs[i][j];
        packets[i][POS_COMMAND] = CMD_SET_COLOR;
        packets[i][POS_SEQUENCE] = sequence++;
        packets[i][POS_PAYLOAD_START] = h[i];
        packets[i][POS_PAYLOAD_START + 1] = s[i];
        packets[i][POS_PAYLOAD_START + 2] = v[i];

        packetPtrs[i] = packets[i];
        lens[i] = PACKET_SIZE;
    }

    if (Debug::isVerbose()) {
        Debug::println("[INFO][CONTROL] Batch TX " + String(count) + " lamps");
    }

    radio.sendPackets(packetPtrs, lens, count);
}

void ControlLivingColors::turnOffBatch(const uint8_t *lampAddrs[], uint8_t count) {
    uint8_t packets[11][PACKET_SIZE];
    uint8_t* packetPtrs[11];
    uint8_t lens[11];

    for (uint8_t i = 0; i < count; i++) {
        packets[i][POS_LENGTH] = PACKET_LENGTH;
        for (int j = 0; j < 9; j++)
            packets[i][POS_DEST_START + j] = lampAddrs[i][j];
        packets[i][POS_COMMAND] = CMD_TURN_OFF;
        packets[i][POS_SEQUENCE] = sequence++;
        packets[i][POS_PAYLOAD_START] = 0;
        packets[i][POS_PAYLOAD_START + 1] = 0;
        packets[i][POS_PAYLOAD_START + 2] = 0;

        packetPtrs[i] = packets[i];
        lens[i] = PACKET_SIZE;
    }

    if (Debug::isVerbose()) {
        Debug::println("[INFO][CONTROL] Batch OFF " + String(count) + " lamps");
    }

    radio.sendPackets(packetPtrs, lens, count);
}

void ControlLivingColors::sniffForLamp() {
    uint8_t buf[64];
    uint8_t len = sizeof(buf);

    radio.readPacket(buf, len);

    if (len == 0)
        return;

    // LivingColors packets are typically 17 bytes (1 length + 16 data + CRC)
    if (len >= PACKET_SIZE) {
        Debug::println("SNIFF: " + Debug::hex(buf, len));

        // First byte is length (0x10)
        // Next 9 bytes = destination (4) + source (4) + port (1)
        // Then command, seq, H, S, V, CRC_L, CRC_H

        uint8_t *addr = &buf[POS_DEST_START];
        Debug::println("Sniffed Address: " + Debug::hex(addr, 9));
        
        uint8_t cmd = buf[POS_COMMAND];
        uint8_t seq = buf[POS_SEQUENCE];
        uint8_t h = buf[POS_PAYLOAD_START];
        uint8_t s = buf[POS_PAYLOAD_START + 1];
        uint8_t v = buf[POS_PAYLOAD_START + 2];
        
        Debug::println("Command: 0x" + String(cmd, HEX) + 
                      " Seq: " + String(seq) + 
                      " HSV: " + String(h) + "," + String(s) + "," + String(v));
    }
}

/* 
Message setup

Position   Length   Information
===================================================
0000-0000  1        Length of the message (always 0x0E)
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
    rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b,
    unsigned char *h, unsigned char *s, unsigned char *v);

    rgb_color {
    unsigned char r, g, b;    /* Channel intensities between 0 and 255 

    
    hsv_color {
    unsigned char hue;        /* Hue degree between 0 and 255 
    unsigned char sat;        /* Saturation between 0 (gray) and 255 
    unsigned char val;        /* Value between 0 (black) and 255 
};

*/
