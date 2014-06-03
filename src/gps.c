//Stuff

#include "gps.h"
#include "uart.h"

int32_t lat;
int32_t lon;
int32_t alt;
uint8_t lock;
uint8_t sats;

/**
 * Calculate a UBX checksum using 8-bit Fletcher (RFC1145)
 */
void gps_ubx_checksum(uint8_t* data, uint8_t len, uint8_t* cka,
                      uint8_t* ckb)
{
    *cka = 0;
    *ckb = 0;
    uint8_t i;
    for( i = 0; i < len; i++ )
    {
        *cka += *data;
        *ckb += *cka;
        data++;
    }
}

/**
 * Verify the checksum for the given data and length.
 */
uint8_t _gps_verify_checksum(uint8_t* data, uint8_t len)
{
    uint8_t a, b;
    gps_ubx_checksum(data, len, &a, &b);
    if( a != *(data + len) || b != *(data + len + 1))
        return 0;
    else
        return 1;
}

// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t *MSG, uint8_t len)
{
    int i;
    for(i=0; i<len; i++) {
        uart0SendByte(MSG[i]);
    }
}

void setupGPS(){
    mrtDelay(5000);
    //Turning off all GPS NMEA strings apart on the uBlox module
    // Taken from Project Swift (rather than the old way of sending ascii text)
    // Had problems with memcpy (or the lack of) - solution here: http://www.utasker.com/forum/index.php?topic=1750.0;wap2
    static uint8_t setNMEAoff[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA9};
    sendUBX(setNMEAoff, 28);
    
    mrtDelay(1000);
    
    // Check and set the navigation mode (Airborne, 1G)
    static uint8_t setNav[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC};
    sendUBX(setNav, 44);
}

/**
 * Verify that the uBlox 6 GPS receiver is set to the <1g airborne
 * navigaion mode.
 */
uint8_t gps_check_nav(void)
{
    GPSerror = 0;
    
    static uint8_t request[8] = {0xB5, 0x62, 0x06, 0x24, 0x00, 0x00,
        0x2A, 0x84};
    sendUBX(request, 8);
    
    // Get the message back from the GPS
    gps_get_data();
    
    // Verify sync and header bytes
    if( gps_buf[0] != 0xB5 || gps_buf[1] != 0x62 ){
        GPSerror = 41;
    }
    if( gps_buf[2] != 0x06 || gps_buf[3] != 0x24 ){
        GPSerror = 42;
    }
    // Check 40 bytes of message checksum
    if( !_gps_verify_checksum(&gps_buf[2], 40) ) {
        GPSerror = 43;
    }
    
    // Return the navigation mode and let the caller analyse it
    if (GPSerror == 0){
        return gps_buf[8];
    }
    else {
        return -1;
    }
}

/**
 * Poll the GPS for a position message then extract the useful
 * information from it - POSLLH.
 */
void gps_get_position()
{
    GPSerror = 0;
    // Request a NAV-POSLLH message from the GPS
    static uint8_t request[8] = {0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03,
        0x0A};
    sendUBX(request, 8);
    
    // Get the message back from the GPS
    gps_get_data();
    
    // Verify the sync and header bits
    if( gps_buf[0] != 0xB5 || gps_buf[1] != 0x62 )
        GPSerror = 21;
    if( gps_buf[2] != 0x01 || gps_buf[3] != 0x02 )
        GPSerror = 22;
    
    if( !_gps_verify_checksum(&gps_buf[2], 32) ) {
        GPSerror = 23;
    }
    
    if(GPSerror == 0) {
        // 4 bytes of longitude (1e-7)
        lon = (int32_t)gps_buf[10] | (int32_t)gps_buf[11] << 8 |
        (int32_t)gps_buf[12] << 16 | (int32_t)gps_buf[13] << 24;
        lon /= 1000;
        
        // 4 bytes of latitude (1e-7)
        lat = (int32_t)gps_buf[14] | (int32_t)gps_buf[15] << 8 |
        (int32_t)gps_buf[16] << 16 | (int32_t)gps_buf[17] << 24;
        lat /= 1000;
        
        // 4 bytes of altitude above MSL (mm)
        alt = (int32_t)gps_buf[22] | (int32_t)gps_buf[23] << 8 |
        (int32_t)gps_buf[24] << 16 | (int32_t)gps_buf[25] << 24;
        alt /= 1000;
    }
    
}

/**
 * Check the navigation status to determine the quality of the
 * fix currently held by the receiver with a NAV-STATUS message.
 */
void gps_check_lock()
{
    GPSerror = 0;
    // Construct the request to the GPS
    static uint8_t request[8] = {0xB5, 0x62, 0x01, 0x06, 0x00, 0x00,
        0x07, 0x16};
    sendUBX(request, 8);
    
    // Get the message back from the GPS
    gps_get_data();
    // Verify the sync and header bits
    if( gps_buf[0] != 0xB5 || gps_buf[1] != 0x62 ) {
        GPSerror = 11;
    }
    if( gps_buf[2] != 0x01 || gps_buf[3] != 0x06 ) {
        GPSerror = 12;
    }
    
    // Check 60 bytes minus SYNC and CHECKSUM (4 bytes)
    if( !_gps_verify_checksum(&gps_buf[2], 56) ) {
        GPSerror = 13;
    }
    
    if(GPSerror == 0){
        // Return the value if GPSfixOK is set in 'flags'
        if( gps_buf[17] & 0x01 )
            lock = gps_buf[16];
        else
            lock = 0;
        
        sats = gps_buf[53];
    }
    else {
        lock = 0;
    }
}

void gps_get_data(){
    uint8_t i;
    mrtDelay(1000);
    if(UART0_available() > 0){
        for(i=0; i<serialBuffer_write; i++){
            gps_buf[i] = serialBuffer[i];
        }
        serialBuffer_write = 0;
    }
}