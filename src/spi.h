/**************************************************************************/
/*!
    @file     spi.h
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
/**************************************************************************/
#ifndef _SPI_H_
#define _SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "LPC8xx.h"

#define SPI_CFG_ENABLE (0x1)
#define SPI_CFG_MASTER (0x4)
#define SPI_STAT_RXRDY (0x1)
#define SPI_STAT_TXRDY (0x2)
#define SPI_STAT_SSD (0x20)
#define SPI_STAT_MSTIDLE (0x100)
#define SPI_TXDATCTL_SSEL_N(s) ((s) << 16) 
#define SPI_TXCTL_DEASSERT_SSEL ((uint32_t) (1 << 16))
#define SPI_TXDATCTL_EOT (1 << 20) 
#define SPI_TXDATCTL_EOF (1 << 21) 
#define SPI_TXDATCTL_RXIGNORE (1 << 22) 
#define SPI_TXDATCTL_FLEN(l) ((l) << 24)
    
#define RFM69_SPI_WRITE_MASK 0x80
    
#define SPI_CR_MSB_FIRST_EN   ((uint32_t) 0) /*Data will be transmitted and received in standard order (MSB first).*/

void    spiInit     ( LPC_SPI_TypeDef *SPIx, uint32_t div, uint32_t delay );
    uint8_t spiTransmit(LPC_SPI_TypeDef *SPIx, uint8_t val, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif
