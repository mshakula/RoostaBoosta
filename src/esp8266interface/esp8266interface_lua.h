/* ESP8266Interface.h */
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
 */
 
#ifndef ESP8266INTERFACE_H_
#define ESP8266INTERFACE_H_

#include "esp8266_lua.h"
//#include "Endpoint.h"

 /**
 * Interface using ESP8266 to connect to an IP-based network
 */
class ESP8266Interface_Lua: public ESP8266_Lua {
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
    ESP8266Interface_Lua(PinName tx, PinName rx, PinName reset, int baud = 9600, int timeout = 3000);
    
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
    * Disconnect the ESP8266 module from the access point
    *
    * @return true if successful
    */
    //int disconnect();
  
    /** Get IP address
    *
    * @return Either a pointer to the internally stored IP address or null if not connected
    */
    //const char *getIPAddress();
};

#include "UDPSocket.h"

#endif /* ESP8266INTERFACE_H_ */