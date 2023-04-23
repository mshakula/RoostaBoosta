//WifiClient.cpp
//Raymond Barrett for roostaBoosta 2023
//rbarrett38@gatech.edu

#include "mbed.h"
#include "WifiClient.h"
//#include "Endpoint.h"
#include <string>
#include <algorithm>

//Debug is disabled by default
//NOTE - MOST OF THESE FUNCTIONS DON'T WORK. WILL UPDATE LATER
#if 0
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

//BufferedSerial pc(USBTX, USBRX); //DEBUG ONLY BAD PRACTICE

WifiClient *WifiClient::_inst;

WifiClient::WifiClient(PinName tx, PinName rx, PinName reset, int baud, std::chrono::microseconds timeout) :
        _serial(tx, rx), _reset_pin(reset) {
    INFO("Initializing WifiClient");
    
    _baud = baud;
    _timeout = timeout;
    
    _serial.set_baud(_baud);

    _handle.serial = &_serial;
    FILE* serialstream = fdopen(&_serial, "w");
    _handle.file = serialstream;

    _inst = this;
}

bool WifiClient::reset() {
    _reset_pin = 0;
    wait_us(20);
    _reset_pin = 1;
    
    // Send reboot command in case reset is not connected
    printCMD(&_handle, 1s, "node.restart()\r\n");
    flushBuffer();
    return true;
}

bool WifiClient::init() {
    // Reset device to clear state
    return reset();
}

bool WifiClient::connect(const char *ssid, const char *phrase) {
    // Configure as station with passed ssid and passphrase
    printCMD(&_handle, 1s, "wifi.setmode(wifi.STATION)\r\n");
    ThisThread::sleep_for(500ms);
    printCMD(&_handle, 1s, "wifi.sta.config(\"%s\",\"%s\")\r\n", ssid, phrase);
    flushBuffer();
    
    Timer timer;
    timer.start();
    //keep checking for valid ip
    flushBuffer();
    while (timer.elapsed_time() < _timeout) {
        memset(_recv, '\0', sizeof(_recv));
        printCMD(&_handle, 1s, "print(wifi.sta.getip())\r\n");
        getreply(_recv);
        //printf("%s\n", _recv); //DEBUG ONLY
        if(strcmp(_recv, "nil") != 0){
            memcpy(_ip, _recv, 16);
            printf("%s\n", _ip);
            timer.stop();
            timer.reset();
            return 1;
        }
    }
    timer.stop();
    timer.reset();
    return 0;
}

bool WifiClient::disconnect() {
    printCMD(&_handle, 1s, "wifi.sta.disconnect()\r\n");
    flushBuffer();
    Timer timer;
    timer.start();
    //make sure that wifi station has ip nil
    while (timer.elapsed_time() < _timeout) {
        memset(_recv, '\0', sizeof(_recv));
        printCMD(&_handle, 1s, "print(wifi.sta.getip())\r\n");
        getreply(_recv);
        //printf("%s\n", _recv); //DEBUG ONLY
        if(strcmp(_recv, "nil") == 0){
            memcpy(_ip, _recv, 3);
            timer.stop();
            timer.reset();
            return 1;
        }
    }
    timer.stop();
    timer.reset();
    return 0;
}

bool WifiClient::is_connected() {
    return (strcmp(_ip, "nil") != 0);
}

void WifiClient::get_ip(char* ip){
    printf("test: %s\n", _ip);
    //ip = _ip;
    memcpy(ip, _ip, 16);
    printf("test2: %s\n", ip);
}

int WifiClient::printCMD(Handle* handle, std::chrono::microseconds timeout, const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    Timer t;
    t.start();
    while(t.elapsed_time() < timeout){
        if (handle->serial->writable()){
            vfprintf(handle->file, fmt, args);
            va_end(args);
            return 1;
        }
    }
    va_end(args);
    return 0;
}

bool WifiClient::discardEcho(){
    char c;
    int num;
    while(true){
        num = _serial.read(&c, 1);
        //pc.write(&c, num); //debug
        if (c < 0)
            return false;
        else if (c == '\r' || c == '>'){
            _serial.read(&c, 1);
            return true;
        }
    }
}

void WifiClient::flushBuffer(){
    Timer t;
    t.start();
    int num;
    char c;
    while(t.elapsed_time() < 3s){
        if(_serial.readable()){
            num = _serial.read(&c, 1);
            //pc.write(_recv, num); //DEBUG ONLY
        }
    }
}



//todo: clean up
int WifiClient::getreply(char* resp){
    if(!discardEcho()){
        return false;
    }
    Timer t;
    t.start();
    char c;
    int num = 0;
    while(t.elapsed_time() < 1s){
        if(_serial.readable()){
            num = _serial.read(&c, 1);
            if(c == '\r')
                break;
            std::string resp1;
            if(resp){strncat(resp, &c, 1);}
            //pc.write(&c, num); //DEBUG ONLY
        }
    }
    flushBuffer();
    return 1;
}