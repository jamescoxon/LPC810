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


char data_rx[64];
uint8_t data_count = 96; // 'a'
uint8_t num_repeats = '5';
char id[] = "AF";
char location_string[] = "51.3580,1.0208";

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
    uint8_t data, rx_len;
    
    /* Initialise the GPIO block */
    gpioInit();
    
    /* Initialise the UART0 block for printf output */
    uart0Init(115200);
    
    /* Configure the multi-rate timer for 1ms ticks */
    mrtInit(__SYSTEM_CLOCK/1000);
    
    /* Configure the switch matrix (setup pins for UART0 and GPIO) */
    configurePins();

    /* Send some text (printf is redirected to UART0) */
    //printf("Hello, LPC810!\n\r");
    
    RFM69_init();

    printf("Done\n\r");

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

        //Send data
        
        //**** Packet Tx Count ******
        //using a byte to keep track of transmit count
        // its meant to roll over
        data_count++;
        //'a' packet is only sent on the first transmission so we need to move it along
        // when we roll over.
        // 98 = 'b' up to 122 = 'z'
        if(data_count > 122){
            data_count = 98; //'b'
        }
        
        //uint8_t n=sprintf(data_temp, "%c[%s]", num_repeats, id);
        uint8_t n=sprintf(data_temp, "%c%cL%s[%s]", num_repeats, data_count, location_string, id);
        
        RFM69_setMode(RFM69_MODE_TX);
        
        mrtDelay(100);
        
        RFM69_send(data_temp, n + 1, 10);

        printf("Tx\n\r");
        
        RFM69_setMode(RFM69_MODE_RX);

        mrtDelay(100);
        
        int countdown = 1000;
        
        while (countdown >0){
            if(RFM69_checkRx() == 1){
                //printf("\r\nData rx'd\r\n");
                RFM69_recv(data_rx,  &rx_len);
                printf("%s\n\r",data_rx);
            }
            countdown--;
            mrtDelay(100);
            //printf("%d ", countdown);
        }
        
        //mrtDelay(1000);
        
        //int radio_rssi = RFM69_sampleRssi();
        //printf("RSSI: %d\r\n", radio_rssi);
        
    }
}