/**************************************************************************/
/*!
    @file     spi.c
    @author   K. Townsend

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, K. Townsend (microBuilder.eu)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*******************************SPIx*******************************************/
#include "spi.h"

/* Configure SPI as Master in Mode 0 (CPHA and CPOL = 0) */
void spiInit(LPC_SPI_TypeDef *SPIx, uint32_t div, uint32_t delay)
{
  NVIC_DisableIRQ( SPI0_IRQn);
    
  /* Enable SPI clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);

  /* Bring SPI out of reset */
  LPC_SYSCON->PRESETCTRL &= ~(0x1<<0);
  LPC_SYSCON->PRESETCTRL |= (0x1<<0);

  /* Enable SPI0 interrupt */
  //NVIC_EnableIRQ(SPI0_IRQn);
  
  /* Set clock speed and mode */
  SPIx->CFG = SPI_CR_MSB_FIRST_EN;
  SPIx->DIV = div;
  SPIx->DLY = delay;
  SPIx->CFG = (SPI_CFG_MASTER & ~SPI_CFG_ENABLE);
  SPIx->CFG |= SPI_CFG_ENABLE;
}

/* Send/receive data via the SPI bus (assumes 8bit data) */
uint8_t spiTransmit(LPC_SPI_TypeDef *SPIx, uint8_t val, uint8_t len)
{
    if(len == 0){
        while(~SPIx->STAT & SPI_STAT_TXRDY);
        SPIx->TXDATCTL = SPI_TXDATCTL_FLEN(7) | SPI_TXDATCTL_EOT | SPI_TXDATCTL_SSEL_N(0xe)| val;
    }
    else {
        while(~SPIx->STAT & SPI_STAT_TXRDY);
        SPIx->TXDATCTL = SPI_TXDATCTL_FLEN(7) | SPI_TXDATCTL_SSEL_N(0xe)| val;
    }
}
                                           
/* Receive data via the SPI bus (assumes 8bit data) */
uint8_t spiReceive(LPC_SPI_TypeDef *SPIx)
{
    
    while(~SPIx->STAT & SPI_STAT_RXRDY);
    return SPIx->RXDAT;
    
}

uint8_t spiRead(uint8_t reg){
    
    uint16_t data;
    spiTransmit(LPC_SPI0, reg & ~RFM69_SPI_WRITE_MASK, 1);
    spiReceive(LPC_SPI0);
    
    spiTransmit(LPC_SPI0, 0x00, 0);
    
    data = spiReceive(LPC_SPI0);
    return data;
}

uint8_t spiWrite(uint8_t reg, uint8_t val){
    
    spiTransmit(LPC_SPI0, (reg | RFM69_SPI_WRITE_MASK), 1); // Send the address with the write mask on
    spiReceive(LPC_SPI0);
    spiTransmit(LPC_SPI0, val, 0);
    spiReceive(LPC_SPI0);

}

void spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len)
{
    spiTransmit(LPC_SPI0, (reg & ~RFM69_SPI_WRITE_MASK), len);
    spiReceive(LPC_SPI0);
    len--;
    
    while (len--){
        spiTransmit(LPC_SPI0, 0x00, len);
        *dest++ = spiReceive(LPC_SPI0);
    }
}