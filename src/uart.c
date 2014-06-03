/**************************************************************************/
/*!
    @file     uart.c
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
#include <string.h>

#include "uart.h"

void uart0Init(uint32_t baudRate)
{
  uint32_t clk;
  const uint32_t UARTCLKDIV=1;

  /* Setup the clock and reset UART0 */
  LPC_SYSCON->UARTCLKDIV = UARTCLKDIV;
  NVIC_DisableIRQ(UART0_IRQn);
  LPC_SYSCON->SYSAHBCLKCTRL |=  (1 << 14);
  LPC_SYSCON->PRESETCTRL    &= ~(1 << 3);
  LPC_SYSCON->PRESETCTRL    |=  (1 << 3);

  /* Configure UART0 */
  clk = __MAIN_CLOCK/UARTCLKDIV;
  LPC_USART0->CFG = UART_DATA_LENGTH_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
  LPC_USART0->BRG = clk / 16 / baudRate - 1;
  LPC_SYSCON->UARTFRGDIV = 0xFF;
  LPC_SYSCON->UARTFRGMULT = (((clk / 16) * (LPC_SYSCON->UARTFRGDIV + 1)) /
    (baudRate * (LPC_USART0->BRG + 1))) - (LPC_SYSCON->UARTFRGDIV + 1);

  /* Clear the status bits */
  LPC_USART0->STAT = UART_STATUS_CTSDEL | UART_STATUS_RXBRKDEL;

  /* enable rx interrupts (http://jaromir.xf.cz/phone/phone1.html) */
  LPC_USART0->INTENSET = (1 << 0);
    
  /* Enable UART0 interrupt */
  NVIC_EnableIRQ(UART0_IRQn);

  /* Enable UART0 */
  LPC_USART0->CFG |= UART_ENABLE;
    
  serialBuffer_read = 0;
  serialBuffer_write = 0;
}

void uart0SendChar(char buffer)
{
  /* Wait until we're ready to send */
  while (!(LPC_USART0->STAT & UART_STATUS_TXRDY));
  LPC_USART0->TXDATA = buffer;
}

void uart0SendByte(uint8_t buffer)
{
    /* Wait until we're ready to send */
    while (!(LPC_USART0->STAT & UART_STATUS_TXRDY));
    LPC_USART0->TXDATA = buffer;
}

void uart0Send(char *buffer, uint32_t length)
{
  while (length != 0)
  {
    uart0SendChar(*buffer);
    buffer++;
    length--;
  }
}

//Lifted directly from https://github.com/devmapal/LPC810_Pong
// Ideally this should be interrupt driven
char uart0ReceiveChar()
{
	if (LPC_USART0->STAT & UART_STATUS_RXRDY)
		return LPC_USART0->RXDATA;
    
	return 0;
}

// (http://jaromir.xf.cz/phone/phone1.html)
void UART0_IRQHandler(void)
{
	char temp;
	int intstat = LPC_USART0->INTSTAT;
	if(intstat & (1 << 0))
    {
        serialBuffer[serialBuffer_write] = LPC_USART0->RXDATA;
        
		if(serialBuffer_write < 64){
            serialBuffer_write++;
        }
        else{
            serialBuffer_write = 0;
        }
        //uart0SendChar(temp); //Echo characters to UART0
    }
}

uint8_t UART0_available(){
    return serialBuffer_write;
}

void UART0_printBuffer(){
    uint8_t i;
    
    for(i=0; i<serialBuffer_write; i++){
        uart0SendChar(serialBuffer[i]);
    }
    serialBuffer_write = 0;
}


