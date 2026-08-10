#pragma once

// CC2500 register addresses
#define CC2500_REG_IOCFG2      0x00
#define CC2500_REG_IOCFG0      0x02
#define CC2500_REG_FIFOTHR     0x03
#define CC2500_REG_SYNC1       0x04
#define CC2500_REG_SYNC0       0x05
#define CC2500_REG_PKTLEN      0x06
#define CC2500_REG_PKTCTRL1    0x07
#define CC2500_REG_PKTCTRL0    0x08
#define CC2500_REG_ADDR        0x09
#define CC2500_REG_CHANNR      0x0A
#define CC2500_REG_FSCTRL1     0x0B
#define CC2500_REG_FSCTRL0     0x0C
#define CC2500_REG_FREQ2       0x0D
#define CC2500_REG_FREQ1       0x0E
#define CC2500_REG_FREQ0       0x0F
#define CC2500_REG_MDMCFG4     0x10
#define CC2500_REG_MDMCFG3     0x11
#define CC2500_REG_MDMCFG2     0x12
#define CC2500_REG_MDMCFG1     0x13
#define CC2500_REG_MDMCFG0     0x14
#define CC2500_REG_DEVIATN     0x15
#define CC2500_REG_MCSM0       0x18
#define CC2500_REG_FOCCFG      0x19
#define CC2500_REG_BSCFG       0x1A
#define CC2500_REG_AGCCTRL2    0x1B
#define CC2500_REG_AGCCTRL1    0x1C
#define CC2500_REG_AGCCTRL0    0x1D
#define CC2500_REG_FSCAL3      0x1E
#define CC2500_REG_FSCAL2      0x1F
#define CC2500_REG_FSCAL1      0x20
#define CC2500_REG_FSCAL0      0x21
#define CC2500_REG_TEST2       0x2C
#define CC2500_REG_TEST1       0x2D
#define CC2500_REG_TEST0       0x2E

// Status registers
#define CC2500_REG_PARTNUM     0x30
#define CC2500_REG_VERSION     0x31

// Burst flags
#define CC2500_OFF_WRITE_BURST 0x40
#define CC2500_OFF_READ_BURST  0xC0

// Strobes
#define CC2500_CMD_SRES        0x30
#define CC2500_CMD_SFSTXON     0x31
#define CC2500_CMD_SXOFF       0x32
#define CC2500_CMD_SCAL        0x33
#define CC2500_CMD_SRX         0x34
#define CC2500_CMD_STX         0x35
#define CC2500_CMD_SIDLE       0x36
#define CC2500_CMD_SAFC        0x37
#define CC2500_CMD_SWOR        0x38
#define CC2500_CMD_SPWD        0x39
#define CC2500_CMD_SFRX        0x3A
#define CC2500_CMD_SFTX        0x3B
#define CC2500_CMD_SWORRST     0x3C
#define CC2500_CMD_SNOP        0x3D

// FIFO addresses
#define CC2500_REG_TXFIFO      0x3F
#define CC2500_REG_RXFIFO      0x3F

#define CC2500_REG_FREND1      0x21
#define CC2500_REG_FREND0      0x22
#define CC2500_REG_PATABLE     0x3E
