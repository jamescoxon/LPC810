#include "LPC8xx.h"

void gps_ubx_checksum(uint8_t* data, uint8_t len, uint8_t* cka,
                      uint8_t* ckb);
uint8_t _gps_verify_checksum(uint8_t* data, uint8_t len);
void sendUBX(uint8_t *MSG, uint8_t len);
void setupGPS();
uint8_t gps_check_nav(void);
void gps_get_data();
void gps_check_lock();
void gps_get_position();

uint8_t gps_buf[80]; //GPS receive buffer
uint8_t GPSerror;

extern int32_t lat;
extern int32_t lon;
extern int32_t alt;
extern uint8_t lock;
extern uint8_t sats;