//zombie.c

#include <stdint.h>
#include "zombie.h"
#include "LPC8xx.h"
#include "rfm69.h"
#include "settings.h"

//Low Power Modes

void init_sleep(){
    //http://jeelabs.org/book/1446b/
    //https://github.com/Fadis/lpc810/blob/master/mbed/target/hal/TARGET_NXP/TARGET_LPC81X/sleep.c
    LPC_SYSCON->SYSAHBCLKCTRL |= 1<<9;  // SYSCTL_CLOCK_WKT
    LPC_PMU->DPDCTRL |= (1<<2)|(1<<3);  // LPOSCEN and LPOSCDPDEN
    LPC_WKT->CTRL = 1<<0;               // WKT_CTRL_CLKSEL
    
    NVIC_EnableIRQ(WKT_IRQn);
    
    LPC_SYSCON->STARTERP1 = 1<<15;      // wake up from alarm/wake timer
    SCB->SCR |= 1<<2;                   // enable SLEEPDEEP mode
    //SCB->SCR = 0;                     // enable SLEEP (clock remains running, system clock remain in active mode but processor not clocked
    //LPC_PMU->PCON = 3;
    
    //Deep sleep in PCON
    LPC_PMU->PCON &= ~0x03;
    LPC_PMU->PCON |= 0x02;
    
    //If brownout detection and WDT are enabled, keep them enabled during sleep
    LPC_SYSCON->PDSLEEPCFG = LPC_SYSCON->PDRUNCFG;
    
    //After wakeup same stuff as currently enabled:
    LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;
    
}

void sleepMicro(uint32_t sleep_time){
        
    LPC_WKT->COUNT = (sleep_time * 10);              // 10 KHz / 5000 -> wakeup in 500 ms
    __WFI();
}

void WKT_IRQHandler () {
    LPC_WKT->CTRL = LPC_WKT->CTRL;      // clear the alarm interrupt
}

void sleepRadio(){
    //printf("Sleeping");
    RFM69_setMode(RFM69_MODE_SLEEP);
    init_sleep();
    sleepMicro(TX_GAP * 100);
    //printf("Awake");
    
}

// estimate the bandgap voltage in terms of Vcc ladder steps, returns as mV
int acmpVccEstimate () {
    
    LPC_SYSCON->PDRUNCFG &= ~(1<<15);             // power up comparator
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<19);         // ACMP & IOCON clocks
    //LPC_SYSCON->PRESETCTRL &= ~(1<<12);           // reset comparator
    //LPC_SYSCON->PRESETCTRL |= (1<<12);            // release comparator
    
    // connect ladder to CMP+ and bandgap to CMP-
    LPC_CMP->CTRL = (6<<11); // careful: 6 on LPC81x, 5 on LPC82x !
    
    int i, n;
    for (i = 2; i < 32; ++i) {
        LPC_CMP->LAD = (i << 1) | 1;                // use ladder tap i
        for (n = 0; n < 100; ++n) __ASM("");    // brief settling delay
        if (LPC_CMP->CTRL & (1<<21))                // if COMPSTAT bit is set
            break;                                    // ... we're done
    }
    // the result is the number of Vcc/31 ladder taps, i.e.
    //    tap * (Vcc / 31) = 0.9
    // therefore:
    //    Vcc = (0.9 * 31) / tap
    // or, in millivolt:
    int tap = i - 1;
    return (900 * 31) / tap;
    
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<19);         // ACMP & IOCON clocks
    LPC_SYSCON->PDRUNCFG |= (1<<15);
}


