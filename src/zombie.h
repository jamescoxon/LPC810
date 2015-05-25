// zombie.h

void init_sleep();
void sleepMicro(uint32_t sleep_time);
void WKT_IRQHandler();
void sleepRadio();
void acmpVccSetup();
int acmpVccEstimate();