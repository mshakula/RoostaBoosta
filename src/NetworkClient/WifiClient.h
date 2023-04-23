/* WifiClient.h
 * Raymond Barrett for RoostaBoosta 2023
 * rbarrett38@gatech.edu
 *
 * @section DESCRIPTION
 *
 * ESP8266 serial wifi module
 *
 * Datasheet:
 *
 * http://www.electrodragon.com/w/Wi07c
 */

#ifndef WIFICLIENT_H
#define WIFICLIENT_H

#include "mbed.h"

//struct describing serial port and filestream pointing to it
struct Handle {
        FILE* file;
        mbed::BufferedSerial* serial;
};

/**
 * The ESP8266 class
 */
class WifiClient
{
public:
    /**
    * Constructor
    *
    * @param tx mbed pin to use for tx line of Serial interface
    * @param rx mbed pin to use for rx line of Serial interface
    * @param reset reset pin of the wifi module ()
    * @param baud the baudrate of the serial connection
    * @param timeout the timeout of the serial connection
    */
    WifiClient(PinName tx, PinName rx, PinName reset, int baud = 9600, std::chrono::microseconds timeout = 5s);
    
    /**
    * Initialize the wifi hardware
    * 
    * @return true if successful
    */
    bool init();

    /**
    * Connect the wifi module to the specified ssid.
    *
    * @param ssid ssid of the network
    * @param phrase WEP, WPA or WPA2 key
    * @return true if successful
    */
    bool connect(const char *ssid, const char *phrase);
    
    /**
    * Check connection to the access point
    * @return true if successful
    */
    bool is_connected();

    /**
    * Copy current IP address to buffer
    * @param ip buffer to write IP to
    */
    void get_ip(char* ip);

    /**
    * Copy list of AP's to buffer
    * @param ap_list buffer to write AP's to
    */      
    //void get_aps(char* ap_list);  TODO: WRITE THIS FUNCTION

    /**
    * Disconnect the ESP8266 module from the access point
    *
    * @return true if successful
    */
    bool disconnect();
    /**
    * Reset the wifi module
    */
    bool reset();

    /**
    * Obtains the current instance of the ESP8266
    */
    static WifiClient *getInstance() {
        return _inst;
    };

    
private:

    /**
    * Discards echoed characters
    *
    * @return true if successful
    */
    bool discardEcho();

    /**
    * Sends formatted string over serial port
    * @param handle struct with buffer to check writeable and filestream to write to
    * @param timeout timeout to wait writeable
    * @param fmt string to print, if this string inclues format specifiers the additional arguments following fmt 
    *              are formatted and inserted in the resulting string replacing their respective specifiers.
    */
    int printCMD(Handle* handle, std::chrono::microseconds timeout, const char* fmt, ...);

    /**
    * Flushes BufferedSerial Buffer
    */
    void flushBuffer();

    /**
    * Gets reply from ESP8662
    * @param resp optional buffer to store response
    * @return 1 if successful
    */
    int getreply(char* resp = 0);

    

protected:
    BufferedSerial _serial;
    DigitalOut _reset_pin;

    Handle _handle;
    
    static WifiClient * _inst;
    // TODO WISHLIST: ipv6?
    // this requires nodemcu support
    char _ip[16];

    char _recv[1024];
    
    int _baud;
    std::chrono::microseconds _timeout;
};

#endif