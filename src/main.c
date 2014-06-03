/**************************************************************************/
/*!
 @file     main.c
 
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
#include <stdio.h>
#include <string.h>
#include "LPC8xx.h"
#include "gpio.h"
#include "mrt.h"
#include "uart.h"
#include "gps.h"
#include "spi.h"
#include "rfm69.h"


#if defined(__CODE_RED)
#include <cr_section_macros.h>
#include <NXP/crp.h>
__CRP const unsigned int CRP_WORD = CRP_NO_CRP ;
#endif

void configurePins()
{
    /* Enable SWM clock */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7);
    
    /* Pin Assign 8 bit Configuration */
    /* U0_TXD */
    /* U0_RXD */
    LPC_SWM->PINASSIGN0 = 0xffff0001UL;
    /* SPI0_SCK */
    LPC_SWM->PINASSIGN3 = 0x05ffffffUL;
    /* SPI0_MOSI */
    /* SPI0_MISO */
    /* SPI0_SSEL */
    LPC_SWM->PINASSIGN4 = 0xff020403UL;
    
    /* Pin Assign 1 bit Configuration */
    LPC_SWM->PINENABLE0 = 0xffffffffUL;
}

int main(void)
{
    
    int navmode = 9, count = 0;
    uint8_t data;
    
    /* Initialise the GPIO block */
    gpioInit();
    
    /* Initialise the UART0 block for printf output */
    uart0Init(115200);
    
    /* Configure the multi-rate timer for 1ms ticks */
    mrtInit(__SYSTEM_CLOCK/1000);
    
    /* Configure the switch matrix (setup pins for UART0 and GPIO) */
    configurePins();

    /* Send some text (printf is redirected to UART0) */
    printf("Hello, LPC810!\n\r");
    
    RFM69_init();

    //setMode(RFM69_MODE_TX);
    
    //setupGPS();
    
    while(1)
    {
        /*
        mrtDelay(5000);
        navmode = gps_check_nav();
        mrtDelay(500);
        gps_get_position();
        mrtDelay(500);
        gps_check_lock();
        mrtDelay(500);
        
        printf("Data: %d,%d,%d,%d,%d,%d\n\r", lat, lon, alt, navmode, lock, sats);
        printf("Errors: %d,%d\n\r", GPSerror, serialBuffer_write);
         */
        data = spiRead(RFM69_REG_10_VERSION);
        printf("Rx: 0x%02x\n\r", data);
        
        data = spiRead(RFM69_REG_01_OPMODE);
        printf("Mode: 0x%02x ", data);
        
        if(count == 1){
            RFM69_setMode(RFM69_MODE_TX);
            count = 0;
        }
        else{
            RFM69_setMode(RFM69_MODE_RX);
            count = 1;
        }
        
        spiBurstRead(RFM69_REG_01_OPMODE, _buf, 10);
        
        printf("Read: ");
        uint8_t i;
        for (i = 0; i < 10; i++){
            printf("0x%02x ", _buf[i]);
        }
        printf("\n\r");
        
        
        //Send data
        uint8_t num_repeats = '5';
        char id[] = "AF";
        
        //uint8_t n=sprintf(data_temp, "%c[%s]", num_repeats, id);
        
        //uint8_t i;
        uint8_t n = 7;
        
        for (i = 0; i < n; i++){
            data_temp[i] = i;
        }
        printf("Transmit: ");
        //uint8_t i;
        for (i = 0; i < n; i++){
            printf("%d ", data_temp[i]);
        }
        printf("\n\r");
        
        RFM69_send("TEST", 4, 10);
        
        mrtDelay(1000);
    }
}