//adc.c
// adapted from http://blog.nano-age.co.uk/2014_08_01_archive.html

#define halt_clk(c) LPC_MRT->Channel[3].INTVAL = (c-3)

#include "adc.h"

/* Use ACMP_2 as a simple 5 bit SAR adc */
int read_adc2() {
    int min = 0, max = 63, value = 31, count, compstat, laststat;
    LPC_CMP->CTRL = (0x2 << 8) | (0x3 << 25); /* positive input ACMP0_I2 , negative input voltage ladder , 20mV hysteresis */
    while (!((value == min) || (value == max))) {
        count = 0;
        LPC_CMP->LAD = 1 | (value); /* ladder Enable | Vref=Vdd | value */
        halt_clk(12 * 15); /* 15us worst case for ladder to settle */
        laststat = (LPC_CMP->CTRL >> 21) & 1;
        /* wait till reading is the same 3 times in row */
        do {
            halt_clk(6); /* wait 0.5us */
            compstat = (LPC_CMP->CTRL >> 21) & 1;
            if (compstat == laststat)
                count++;
            else
                count = 0;
            laststat = compstat;
        } while (count < 3);
        /* Binary divide */
        if (compstat) {
            min = value;
            value = value + ((max - value) >> 1);
        } else {
            max = value;
            value = value - ((value - min) >> 1);
        }
    }
    return value >> 1;
}


