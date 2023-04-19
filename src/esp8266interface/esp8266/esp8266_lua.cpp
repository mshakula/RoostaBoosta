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

#include "mbed.h"
#include "esp8266_lua.h"
//#include "Endpoint.h"
#include <string>
#include <algorithm>

//Debug is disabled by default
#if 1
#define DBG(x, ...)  printf("[ESP8266 : DBG]"x" \t[%s,%d]\r\n", ##__VA_ARGS__,__FILE__,__LINE__); 
#define WARN(x, ...) printf("[ESP8266 : WARN]"x" \t[%s,%d]\r\n", ##__VA_ARGS__,__FILE__,__LINE__); 
#define ERR(x, ...)  printf("[ESP8266 : ERR]"x" \t[%s,%d]\r\n", ##__VA_ARGS__,__FILE__,__LINE__); 
#define INFO(x, ...) printf("[ESP8266 : INFO]"x" \t[%s,%d]\r\n", ##__VA_ARGS__,__FILE__,__LINE__); 
#ifndef ESP8266_ECHO
#define ESP8266_ECHO 1
#endif
#else
#define DBG(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#define INFO(x, ...)
#endif

BufferedSerial pc(USBTX, USBRX);

ESP8266_Lua *ESP8266_Lua::_inst;

ESP8266_Lua::ESP8266_Lua(PinName tx, PinName rx, PinName reset, int baud, int timeout) :
        _serial(tx, rx), _reset_pin(reset) {
    INFO("Initializing ESP8266 object");
    
    _baud = baud;
    _timeout = timeout;
    
    _serial.set_baud(_baud);
    
    _inst = this;
}

bool ESP8266_Lua::reset() {
    _reset_pin = 0;
    wait_us(20);
    _reset_pin = 1;
    
    // Send reboot command in case reset is not connected
    char snd[1024];
    strcpy(snd, "node.restart()\r\n");
    sendCMD(snd,sizeof(snd));
    return getreply();
}

bool ESP8266_Lua::init() {
    // Reset device to clear state
    return reset();
}

bool ESP8266_Lua::connect(const char *ssid, const char *phrase) {
    // Configure as station with passed ssid and passphrase
    char snd[1024];
    memset(snd, '\0', sizeof(snd));
    strcpy(snd, "wifi.setmode(wifi.STATION)\r\n");
    sendCMD(snd, sizeof(snd));
    getreply();
    memset(snd, '\0', sizeof(snd));
    ThisThread::sleep_for(1s);
    strcpy(snd, "wifi.sta.config(\"");
    strcat(snd, ssid);
    strcat(snd, "\",\"");
    strcat(snd, phrase);
    strcat(snd, "\")\r\n");
    sendCMD(snd, sizeof(snd));
    getreply();
    memset(snd, '\0', sizeof(snd));
    // Wait for an IP address
    // TODO WISHLIST make this a seperate check so it can run asynch?
    Timer timer;
    timer.start();
    
    while (true) {
        strcpy(snd, "print(wifi.sta.getip())\r\n");
        sendCMD(snd, sizeof(snd));
        getreply();
        
    }
}

/*bool ESP8266_Lua::disconnect() {
    int ip_len = 15;
    
    if (!(command("wifi.sta.disconnect();") &&
          command("ip=wifi.sta.getip();") &&
          command("print(ip)") &&
          execute(_ip, &ip_len)))
        return false;
        
    _ip[ip_len] = 0;
    
    return (strcmp(_ip, "nil") == 0);
}*/

bool ESP8266_Lua::is_connected() {
    return (strcmp(_ip, "nil") != 0);
}


int ESP8266_Lua::writeable() {
    // Always succeeds since message can be temporarily stored on the esp
    return _serial.writable();
}

int ESP8266_Lua::readable() {
    return _serial.readable();
}   


//todo: clean up
bool ESP8266_Lua::sendCMD(const char *cmd, int len){
    Timer t;
    t.start();
    while(t.elapsed_time() < 1s){
        if(writeable()){
            _serial.write(cmd, len);
        }
    }
    t.stop();
    t.reset();
    return true;
}

//todo: clean up
int ESP8266_Lua::getreply(){
    Timer t;
    char write_buf[1024];
    memset(write_buf, '\0', sizeof(write_buf));
    t.start();
    while(t.elapsed_time() < 1s){
        int num = 0;
        if(readable()){
            num = _serial.read(&write_buf, sizeof(write_buf));
            pc.write(&write_buf, num);
        }
    }
    t.stop();
    t.reset();
    return 1;
}