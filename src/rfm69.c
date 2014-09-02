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
#include <string.h>
#include <stdio.h>

void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;
    
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

uint8_t RFM69_init()
{
    mrtDelay(12);
    
    //Configure SPI
    spiInit(LPC_SPI0,24,0);
    
    mrtDelay(100);
    
    // Set up device
    uint8_t i;
    for (i = 0; CONFIG[i][0] != 255; i++)
        spiWrite(CONFIG[i][0], CONFIG[i][1]);
    
    RFM69_setMode(_mode);
    
    // Clear TX/RX Buffer
    _bufLen = 0;
    
    return 1;
}

void RFM69_spiFifoWrite(const uint8_t* src, int len)
{
    
    spiTransmit(LPC_SPI0, (RFM69_REG_00_FIFO | RFM69_SPI_WRITE_MASK), 9);
    spiReceive(LPC_SPI0);
    
    spiTransmit(LPC_SPI0, len, 9);
    spiReceive(LPC_SPI0);
    
    len--;
    
    uint8_t i = 0;
    while (len >= 0){
        spiTransmit(LPC_SPI0, src[i], len);
        spiReceive(LPC_SPI0);
        
        len--;
        i++;
    }
}
void RFM69_setMode(uint8_t newMode)
{
    spiWrite(RFM69_REG_01_OPMODE, (spiRead(RFM69_REG_01_OPMODE) & 0xE3) | newMode);
    _mode = newMode;
}

uint8_t  RFM69_mode()
{
    return _mode;
}

uint8_t RFM69_checkRx()
{
    // Check IRQ register for payloadready flag (indicates RXed packet waiting in FIFO)
    if(spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
        // Get packet length from first byte of FIFO
        _bufLen = spiRead(RFM69_REG_00_FIFO)+1;
        // Read FIFO into our Buffer
        spiBurstRead(RFM69_REG_00_FIFO, _buf, RFM69_FIFO_SIZE);
        // Read RSSI register (should be of the packet? - TEST THIS)
        _lastRssi = -(spiRead(RFM69_REG_24_RSSI_VALUE)/2);
        // Clear the radio FIFO (found in HopeRF demo code)
        RFM69_clearFifo();
        return 1;
    }
    
    return 0;
}

void RFM69_recv(uint8_t* buf, uint8_t* len)
{
    // Copy RX Buffer to byref Buffer
    memcpy(buf, _buf, _bufLen);
    *len = _bufLen;
    // Clear RX Buffer
    _bufLen = 0;
}

void RFM69_send(const uint8_t* data, uint8_t len, uint8_t power)
{
    // power is TX Power in dBmW (valid values are 2dBmW-20dBmW)
    if(power<2 || power >20) {
        // Could be dangerous, so let's check this
        return;
    }
    uint8_t oldMode = _mode;
    // Copy data into TX Buffer
    memcpy(_buf, data, len);
    // Update TX Buffer Size
    _bufLen = len;
    
    RFM69_clearFifo();
    
    // Start Transmitter
    RFM69_setMode(RFM69_MODE_TX);
    // Set up PA
    if(power<=17) {
        // Set PA Level
        uint8_t paLevel = power + 14;
        spiWrite(RFM69_REG_11_PA_LEVEL, RF_PALEVEL_PA0_OFF | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | paLevel);
    } else {
        // Disable Over Current Protection
        spiWrite(RFM69_REG_13_OCP, RF_OCP_OFF);
        // Enable High Power Registers
        spiWrite(RFM69_REG_5A_TEST_PA1, 0x5D);
        spiWrite(RFM69_REG_5C_TEST_PA2, 0x7C);
        // Set PA Level
        uint8_t paLevel = power + 11;
        spiWrite(RFM69_REG_11_PA_LEVEL, RF_PALEVEL_PA0_OFF | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | paLevel);
    }
    // Wait for PA ramp-up
    while(!(spiRead(RFM69_REG_27_IRQ_FLAGS1) & RF_IRQFLAGS1_TXREADY)) { };
    // Throw Buffer into FIFO, packet transmission will start automatically
    RFM69_spiFifoWrite(_buf, _bufLen);
    // Clear MCU TX Buffer
    _bufLen = 0;
    // Wait for packet to be sent
    while(!(spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PACKETSENT)) {};
    // Return Transceiver to original mode
    RFM69_setMode(RFM69_MODE_RX);
    // If we were in high power, switch off High Power Registers
    if(power>17) {
        // Disable High Power Registers
        spiWrite(RFM69_REG_5A_TEST_PA1, 0x55);
        spiWrite(RFM69_REG_5C_TEST_PA2, 0x70);
        // Enable Over Current Protection
        spiWrite(RFM69_REG_13_OCP, RF_OCP_ON | RF_OCP_TRIM_95);
    }
}

void RFM69_SetLnaMode(uint8_t lnaMode) {
    // RF_TESTLNA_NORMAL (default)
    // RF_TESTLNA_SENSITIVE
    spiWrite(RFM69_REG_58_TEST_LNA, lnaMode);
}

void RFM69_clearFifo() {
    // Must only be called in RX Mode
    // Apparently this works... found in HopeRF demo code
    RFM69_setMode(RFM69_MODE_STDBY);
    RFM69_setMode(RFM69_MODE_RX);
}


int RFM69_readTemp()
{
    // Store current transceiver mode
    uint8_t oldMode = _mode;
    // Set mode into Standby (required for temperature measurement)
    RFM69_setMode(RFM69_MODE_STDBY);
	
    // Trigger Temperature Measurement
    spiWrite(RFM69_REG_4E_TEMP1, RF_TEMP1_MEAS_START);
    // Check Temperature Measurement has started
    if(!(RF_TEMP1_MEAS_RUNNING && spiRead(RFM69_REG_4E_TEMP1))){
        return 255;
    }
    // Wait for Measurement to complete
    while(RF_TEMP1_MEAS_RUNNING && spiRead(RFM69_REG_4E_TEMP1)) { };
    // Read raw ADC value
    uint8_t rawTemp = spiRead(RFM69_REG_4F_TEMP2);
	
    // Set transceiver back to original mode
    RFM69_setMode(oldMode);
    // Return processed temperature value
    int temperature = 168 - rawTemp;
    return temperature;
}

int16_t RFM69_lastRssi() {
    return _lastRssi;
}

int16_t RFM69_sampleRssi() {
    // Must only be called in RX mode
    if(_mode!=RFM69_MODE_RX) {
        // Not sure what happens otherwise, so check this
        return 0;
    }
    // Trigger RSSI Measurement
    spiWrite(RFM69_REG_23_RSSI_CONFIG, RF_RSSI_START);
    // Wait for Measurement to complete
    while(!(RF_RSSI_DONE && spiRead(RFM69_REG_23_RSSI_CONFIG))) { };
    // Read, store in _lastRssi and return RSSI Value
    _lastRssi = -(spiRead(RFM69_REG_24_RSSI_VALUE)/2);
    return _lastRssi;
}