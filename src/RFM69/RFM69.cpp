// RFM69.cpp
//
// Ported to Arduino 2014 James Coxon
//
// Copyright (C) 2014 Phil Crump
//
// Based on RF22 Copyright (C) 2011 Mike McCauley ported to mbed by Karl Zweimueller
// Based on RFM69 LowPowerLabs (https://github.com/LowPowerLab/RFM69/)


#include "RFM69.h"
#include "RFM69Config.h"

RFM69::RFM69()
{
    _idleMode = RFM69_MODE_SLEEP; // Default idle state is SLEEP, our lowest power mode
    _mode = RFM69_MODE_RX; // We start up in RX mode
    _rxGood = 0;
    _rxBad = 0;
    _txGood = 0;
    _afterTxMode = RFM69_MODE_RX;
}

boolean RFM69::init()
{
    delay(12);
    _slaveSelectPin = 10;
    pinMode(_slaveSelectPin, OUTPUT); // Init nSS

    delay(100);

    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.begin();
    
    // Set up device
    for (uint8_t i = 0; CONFIG[i][0] != 255; i++)
        spiWrite(CONFIG[i][0], CONFIG[i][1]);
    
    setMode(_mode);

    // We should check for a device here, maybe set and check mode?
    
    //_interrupt.rise(this, &RFM69::isr0);
    //attachInterrupt(0, RFM69::handleInterrupt, RISING);

    clearTxBuf();
    clearRxBuf();

    return true;
}

void RFM69::handleInterrupt()
{
    // RX
    if(_mode == RFM69_MODE_RX) {
        //_lastRssi = rssiRead();
        // PAYLOADREADY (incoming packet)
        if(spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
            _bufLen = spiRead(RFM69_REG_00_FIFO)+1;
            spiBurstRead(RFM69_REG_00_FIFO, _buf, RFM69_FIFO_SIZE); // Read out full fifo
            _rxGood++;
            _rxBufValid = true;
        }
    // TX
    } else if(_mode == RFM69_MODE_TX) {
    
        // PacketSent
        if(spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PACKETSENT) {
            _txGood++;
            spiWrite(RFM69_REG_25_DIO_MAPPING1, RF_DIOMAPPING1_DIO0_01);
            setMode(_afterTxMode);
            _txPacketSent = true;
        }
    }
}

void RFM69::isr0()
{
    handleInterrupt ();
}

uint8_t RFM69::spiRead(uint8_t reg)
{
    noInterrupts();    // Disable Interrupts
    digitalWrite(_slaveSelectPin, LOW);
    
    SPI.transfer(reg & ~RFM69_SPI_WRITE_MASK); // Send the address with the write mask off
    uint8_t val = SPI.transfer(0); // The written value is ignored, reg value is read
    
    digitalWrite(_slaveSelectPin, HIGH);
    interrupts();     // Enable Interrupts
    return val;
}

void RFM69::spiWrite(uint8_t reg, uint8_t val)
{
    noInterrupts();    // Disable Interrupts
    digitalWrite(_slaveSelectPin, LOW);
    
    SPI.transfer(reg | RFM69_SPI_WRITE_MASK); // Send the address with the write mask on
    SPI.transfer(val); // New value follows

    digitalWrite(_slaveSelectPin, HIGH);
    interrupts();     // Enable Interrupts
}

void RFM69::spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len)
{
    digitalWrite(_slaveSelectPin, LOW);
    
    SPI.transfer(reg & ~RFM69_SPI_WRITE_MASK); // Send the start address with the write mask off
    while (len--)
        *dest++ = SPI.transfer(0);

    digitalWrite(_slaveSelectPin, HIGH);
}

void RFM69::spiBurstWrite(uint8_t reg, const uint8_t* src, uint8_t len)
{
    digitalWrite(_slaveSelectPin, LOW);
    
    SPI.transfer(reg | RFM69_SPI_WRITE_MASK); // Send the start address with the write mask on
    while (len--)
        SPI.transfer(*src++);
        
    digitalWrite(_slaveSelectPin, HIGH);
}

int RFM69::rssiRead()
{
    int rssi = 0;
    //RSSI trigger not needed if DAGC is in continuous mode
    rssi = -spiRead(RFM69_REG_24_RSSI_VALUE);
    rssi >>= 1;
    return rssi;
}

void RFM69::setMode(uint8_t newMode)
{
    spiWrite(RFM69_REG_01_OPMODE, (spiRead(RFM69_REG_01_OPMODE) & 0xE3) | newMode);
	_mode = newMode;
}
void RFM69::setModeSleep()
{
    setMode(RFM69_MODE_SLEEP);
}
void RFM69::setModeRx()
{
    setMode(RFM69_MODE_RX);
}
void RFM69::setModeTx()
{
    setMode(RFM69_MODE_TX);
}
uint8_t  RFM69::mode()
{
    return _mode;
}

void RFM69::clearRxBuf()
{
    noInterrupts();   // Disable Interrupts
    _bufLen = 0;
    _rxBufValid = false;
    interrupts();     // Enable Interrupts
}

boolean RFM69::available()
{
    return _rxBufValid;
}

boolean RFM69::recv(uint8_t* buf, uint8_t* len)
{
    if (!available())
        return false;
    noInterrupts();   // Disable Interrupts
    if (*len > _bufLen)
        *len = _bufLen;
    memcpy(buf, _buf, *len);
    clearRxBuf();
    interrupts();     // Enable Interrupts
    return true;
}

void RFM69::clearTxBuf()
{
    noInterrupts();   // Disable Interrupts
    _bufLen = 0;
    _txBufSentIndex = 0;
    _txPacketSent = false;
    interrupts();     // Enable Interrupts
}

void RFM69::startTransmit()
{
    //sendNextFragment(); // Actually the first fragment
    setModeTx(); // Start the transmitter, turns off the receiver
    //Serial.println("Tx Mode Enabled");
    delay(10);
    sendTxBuf();
}

boolean RFM69::send(const uint8_t* data, uint8_t len)
{
    clearTxBuf();
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (!fillTxBuf(data, len))
            return false;
        startTransmit();
    }
    spiWrite(RFM69_REG_25_DIO_MAPPING1, RF_DIOMAPPING1_DIO0_00);
    return true;
}

boolean RFM69::fillTxBuf(const uint8_t* data, uint8_t len)
{
    if (((uint16_t)_bufLen + len) > RFM69_MAX_MESSAGE_LEN)
        return false;
    noInterrupts();   // Disable Interrupts
    memcpy(_buf + _bufLen, data, len);
    _bufLen += len;
    interrupts();     // Enable Interrupts
    return true;
}

void RFM69::sendTxBuf() {
    if(_bufLen<RFM69_FIFO_SIZE) {
        uint8_t* src = _buf;
        uint8_t len = _bufLen;
        digitalWrite(_slaveSelectPin, LOW);
	    SPI.transfer(RFM69_REG_00_FIFO | RFM69_SPI_WRITE_MASK); // Send the start address with the write mask on
	    SPI.transfer(len);
    	while (len--)
        	SPI.transfer(*src++);
	    digitalWrite(_slaveSelectPin, HIGH);
    }
}

void RFM69::readRxBuf()
{
    spiBurstRead(RFM69_REG_00_FIFO, _buf, RFM69_FIFO_SIZE);
    _bufLen += RFM69_FIFO_SIZE;
}

int RFM69::lastRssi()
{
    return _lastRssi;
}
