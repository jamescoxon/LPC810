// RFM69.cpp
//
// Ported to LPC810 2014 James Coxon
// Ported to Arduino 2014 James Coxon
//
// Copyright (C) 2014 Phil Crump
//
// Based on RF22 Copyright (C) 2011 Mike McCauley ported to mbed by Karl Zweimueller
// Based on RFM69 LowPowerLabs (https://github.com/LowPowerLab/RFM69/)

#include "rfm69.h"
#include "RFM69Config.h"
#include "spi.h"

uint8_t RFM69init()
{
    _mode = RFM69_MODE_RX; // We start up in RX mode
    
    mrtDelay(12);
    
    /* Configure SPI */
    spiInit(LPC_SPI0,12,0);
    
    //printf("SPI Configured\n\r");
    
    mrtDelay(1000);
    
    // Set up device
    uint8_t i;
    for (i = 0; CONFIG[i][0] != 255; i++) {
        spiWrite(CONFIG[i][0], CONFIG[i][1]);
    }
    
    setMode(_mode);
    
    // We should check for a device here, maybe set and check mode?
    
    //_interrupt.rise(this, &RFM69::isr0);
    //attachInterrupt(0, RFM69::handleInterrupt, RISING);
    
    //clearTxBuf();
    //clearRxBuf();
    
    mrtDelay(3000);
    
    //printf("Radio configured\n\r");
    
    return 1;
}

void setMode(uint8_t newMode)
{
    spiWrite(RFM69_REG_01_OPMODE, (spiRead(RFM69_REG_01_OPMODE) & 0xE3) | newMode);
}
void setModeSleep()
{
    setMode(RFM69_MODE_SLEEP);
}
void setModeRx()
{
    setMode(RFM69_MODE_RX);
}
void setModeTx()
{
    setMode(RFM69_MODE_TX);
}
uint8_t  mode()
{
    return _mode;
}

uint8_t send(uint8_t len)
{
    setModeTx(); // Start the transmitter, turns off the receiver
    mrtDelay(10);

    spiBurstWrite(RFM69_REG_00_FIFO | RFM69_SPI_WRITE_MASK, len);
    
    mrtDelay(1000);
    
    setModeRx(); // Start the transmitter, turns off the receiver
    return 1;
}