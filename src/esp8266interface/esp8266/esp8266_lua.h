/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * ESP8266 serial wifi module
 *
 * Datasheet:
 *
 * http://www.electrodragon.com/w/Wi07c
 */

#ifndef ESP8266_LUA_H
#define ESP8266_LUA_H

#include "mbed.h"
#include "CBuffer.h"
#include "BufferedSerial.h"

#define ESP_TCP_TYPE 1
#define ESP_UDP_TYPE 0 
#define ESP_MAX_LINE 62

/**
 * The ESP8266 class
 */
class ESP8266_Lua
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
    ESP8266_Lua(PinName tx, PinName rx, PinName reset, int baud = 9600, int timeout = 3000);
    
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
    * Disconnect the ESP8266 module from the access point
    *
    * @return true if successful
    */
    //bool disconnect();
    
    /*
    * Start up a UDP or TCP Connection
    *
    * @param type 0 for UDP, 1 for TCP
    * @param ip A string that contains the IP, no quotes
    * @param port Numerical port number to connect to
    * @param id number between 0-4, if defined it denotes ID to use in multimode (Default to Single connection mode with -1)
    * @return true if sucessful, 0 if fail
    */
    //bool open(bool type, char* ip, int port, int id = -1);
    
    /**
    * Close a connection
    *
    * @return true if successful
    */
    //bool close();
    
    /**
    * Read a character or block
    *
    * @return the character read or -1 on error
    */
    //int getc();
    
    /**
    * Write a character
    *
    * @param the character which will be written
    * @return -1 on error
    */
    //int putc(char c);
    
    /**
    * Write a string
    *
    * @param buffer the buffer that will be written
    * @param len the length of the buffer
    * @return true on success
    */
    //bool send(const char *buffer, int len);

    /**
    * Read a string without blocking
    *
    * @param buffer the buffer that will be written
    * @param len the length of the buffer, is replaced by read length
    * @return true on success
    */
    //bool recv(char *buffer, int *len);
    
    /**
    * Check if wifi is writable
    *
    * @return 1 if wifi is writable
    */
    int writeable();
    
    /**
    * Check if wifi is readable
    *
    * @return number of characters available
    */
    int readable();
    
    /**
    * Return the IP address 
    * @return IP address as a string
    */
    //const char *getIPAddress();

    /**
    * Return the IP address from host name
    * @return true on success, false on failure
    */    
    bool getHostByName(const char *host, char *ip);

    /**
    * Reset the wifi module
    */
    bool reset();

    /**
    * Obtains the current instance of the ESP8266
    */
    static ESP8266_Lua *getInstance() {
        return _inst;
    };
    
private:
    /**
    * Read a character with timeout
    *
    * @return the character read or -1 on timeout
    */
    int serialgetc();
    
    /**
    * Write a character
    *
    * @param the character which will be written
    * @return -1 on timeout
    */
    int serialputc(char c);

    /**
    * Write buffer
    *
    *
    */

    /**
    * Discards echoed characters
    *
    * @return true if successful
    */
    bool discardEcho();
    
    /**
    * Flushes to next prompt
    *
    * @return true if successful
    */
    bool flush();
    
    /**
    * Send part of a command to the wifi module.
    *
    * @param cmd string to be sent
    * @param len optional length of cmd
    * @param sanitize flag indicating if cmd is actually payload and needs to be escaped
    * @return true if successful
    */
    bool command(const char *cmd);
    
    //TODO: WRITE BRIEF
    bool sendCMD(const char *cmd, int len);

    int getreply();

    /**
    * Execute the command sent by command
    *
    * @param resp_buf pointer to buffer to store response from the wifi module
    * @param resp_len len of buffer to store response from the wifi module, is replaced by read length
    * @return true if successful
    */
    bool execute(char *resp_buffer = 0, int *resp_len = 0);

protected:
    BufferedSerial _serial;
    DigitalOut _reset_pin;

    static ESP8266_Lua * _inst;
    
    // TODO WISHLIST: ipv6?
    // this requires nodemcu support
    char _ip[16];
    
    int _baud;
    int _timeout;
};

#endif