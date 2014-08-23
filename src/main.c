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
// Comment out if you don't want printing to serial (if isolated node)
#define DEBUG

// Comment out if you don't want GPS (Ublox binary)
//#define GPS

#include <stdio.h>
#include "LPC8xx.h"
//#include "gpio.h"
#include "mrt.h"
#include "uart.h"
#include "spi.h"
#include "rfm69.h"

#ifdef GPS
    #include "gps.h"
#endif

//NODE SPECIFIC DETAILS - need to be changed
#define NUM_REPEATS                     5
#define ID                        "BALL1"
#define LOCATION_STRING         "52.316,13.62"

uint8_t power_output = 1; //in dbmW
int tx_gap = 500; // milliseconds between tx = tx_gap * 100, therefore 1000 = 100seconds

char data_temp[64];
uint8_t data_count = 96; // 'a' - 1 (as the first function will at 1 to make it 'a'
// attempt to overcome issue of problem with first packet
int rx_packets = 0;

void configurePins()
{
    /* Enable SWM clock */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7);
    
    /* Pin Assign 8 bit Configuration */
    /* U0_TXD */
    /* U0_RXD */
    LPC_SWM->PINASSIGN0 = 0xffff0001UL;
    /* SPI0_SCK */
    LPC_SWM->PINASSIGN3 = 0x02ffffffUL;
    /* SPI0_MOSI */
    /* SPI0_MISO */
    /* SPI0_SSEL */
    LPC_SWM->PINASSIGN4 = 0xff050304UL;
    
    /* Pin Assign 1 bit Configuration */
    LPC_SWM->PINENABLE0 = 0xffffffffUL;
    
}

void awaitData(int countdown){
    
    uint8_t rx_len;
    
    //Clear buffer
    data_temp[0] = '\0';
    
    RFM69_setMode(RFM69_MODE_RX);
    
    while (countdown >0){
        if(RFM69_checkRx() == 1){
            RFM69_recv(data_temp,  &rx_len);
            data_temp[rx_len - 1] = '\0';
            #ifdef DEBUG
                printf("rx: %s\n\r",data_temp);
            #endif
            processData(rx_len);
        }
        countdown--;
        mrtDelay(100);
    }
}

inline void processData(uint32_t len) {
    uint8_t i, packet_len;
    
    for(i=0; i<len; i++) {
        if(data_temp[i] != ']')
            continue;
        
        //Print the string
        data_temp[i+1] = '\0';
        
        if(data_temp[0] <= '0')
            break;
        
        //Reduce the repeat value
        data_temp[0] = data_temp[0] - 1;
        //Now add , and end line and let string functions do the rest
        data_temp[i] = ',';
        data_temp[i+1] = '\0';
        
        if(strstr(data_temp, ID) != 0)
            break;
        
        
        strcat(data_temp, ID); //add ID
        strcat(data_temp, "]"); //add ending
        
        packet_len = strlen(data_temp);
        //random delay to try and avoid packet collision
        mrtDelay(100);
        
        rx_packets++;
        //Send the data (need to include the length of the packet and power in dbmW)
        RFM69_send(data_temp, packet_len, 10);
#ifdef DEBUG
        printf("tx: %s\n\r",data_temp);
#endif
        
        //Ensure we are in RX mode
        RFM69_setMode(RFM69_MODE_RX);
        break;
    }
}

void transmitData(uint8_t i){
    
    #ifdef DEBUG
        printf(data_temp);
        printf(" %d\n\r", i);
    #endif
    //Send the data (need to include the length of the packet and power in dbmW)
    RFM69_send(data_temp, i, 20); //20dbmW
    
    //Ensure we are in RX mode
    RFM69_setMode(RFM69_MODE_RX);
}

int main(void)
{
    /* Initialise the GPIO block */
    gpioInit();
    
#ifdef GPS
    /* Initialise the UART0 block for printf output */
    uart0Init(9600);
#else
    /* Initialise the UART0 block for printf output */
    uart0Init(115200);
#endif
    
    /* Configure the multi-rate timer for 1ms ticks */
    mrtInit(__SYSTEM_CLOCK/1000);
    
    /* Configure the switch matrix (setup pins for UART0 and SPI) */
    configurePins();
    
    RFM69_init();

    #ifdef DEBUG
        printf("Done\n\r");
    #endif
    
    #ifdef GPS
        int navmode = 9;
        setupGPS();
    #endif
    
    while(1)
    {
        
        #ifdef GPS
            mrtDelay(5000);
            navmode = gps_check_nav();
            mrtDelay(500);
            gps_get_position();
            mrtDelay(500);
            gps_check_lock();
            mrtDelay(500);

        
        
            //printf("Data: %d,%d,%d,%d,%d,%d\n\r", lat, lon, alt, navmode, lock, sats);
            //printf("Errors: %d,%d\n\r", GPSerror, serialBuffer_write);
        #endif
        
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
        
        //Clear buffer
        data_temp[0] = '\0';
        uint8_t n;
        
        #ifdef GPS
            n=sprintf(data_temp, "%d%cL%d,%d,%d[%s]", NUM_REPEATS, data_count, lat, lon, alt, ID);
        #else
        //Read internal temperature
        int int_temp = RFM69_readTemp();
        //Create the packet
        
        if (data_count == 97){
            n=sprintf(data_temp, "%d%cL%s[%s]", NUM_REPEATS, data_count, LOCATION_STRING, ID);
        }
        else{
            n=sprintf(data_temp, "%d%cT%d[%s]", NUM_REPEATS, data_count, int_temp, ID);
        }
        
        //uint8_t n=sprintf(data_temp, "%c%cC%d[%s]", num_repeats, data_count, rx_packets, id);
        //uint8_t n=sprintf(data_temp, "%c%cL%d,%d,%d,%d,%d,%d[%s]", num_repeats, data_count, lat, lon, alt, navmode, lock, sats, id);
        #endif
        
        transmitData(n);
        
        awaitData(tx_gap);
        
    }
    
}

//
