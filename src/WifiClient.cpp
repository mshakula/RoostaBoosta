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
    if(_reset_pin.is_connected()){
        _reset_pin = 0;
        wait_us(20);
        _reset_pin = 1;
    }else{    
    // Send reboot command in case reset is not connected
    printCMD(&_handle, 1s, "node.restart()\r\n");
    flushBuffer();
    }
    return true;
}

bool WifiClient::init() {
    // Reset device to clear state
    return reset();
}

bool WifiClient::is_connected() {
    return (strcmp(_ip, "nil") != 0);
}

void WifiClient::get_ip(char* ip){
    //printf("test: %s\n", _ip);
    //ip = _ip;
    memcpy(ip, _ip, 16);
    //printf("test2: %s\n", ip);
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
        printCMD(&_handle, 1s, "print(wifi.sta.getip())\r\n");
        getreply(_ip, 16);
        //printf("%s\n", _ip); //DEBUG ONLY
        if(strcmp(_ip, "nil") != 0){
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
        memset(_ip, '\0', sizeof(_ip));
        printCMD(&_handle, 1s, "print(wifi.sta.getip())\r\n");
        getreply(_ip,3);
        //printf("%s\n", _ip); //DEBUG ONLY
        if(strcmp(_ip, "nil") == 0){
            timer.stop();
            timer.reset();
            return 1;
        }
    }
    printCMD(&_handle, 1s, "print(wifi.sta.getip())\r\n");
    getreply(_ip,16);
    timer.stop();
    timer.reset();
    return 0;
}

int WifiClient::scan(char* aplist, int size){
    printCMD(&_handle, 1s, "function listap(t)\r\n");
    flushBuffer();
    printCMD(&_handle, 1s, "for k,v in pairs(t) do\r\n");
    flushBuffer();
    printCMD(&_handle, 1s, "print(k)\r\n");
    flushBuffer();
    printCMD(&_handle, 1s, "end\r\n");
    flushBuffer();
    printCMD(&_handle, 1s, "end\r\n");
    flushBuffer();
    printCMD(&_handle, 1s, "wifi.sta.getap(listap)\r\n");
    getreply(aplist, size);
    return 1;
}

int WifiClient::http_get_request(const char* address, const char* payload, const char* header, char* respBuffer, size_t respBufferSize){
    printCMD(&_handle, 1s, "sk=net.createConnection(net.TCP, 0)\r\n");
    flushBuffer();
    printCMD(&_handle, 1s, "sk:on(\"receive\", function(sck, c) print(c) end )\r\n");
    flushBuffer();
    printCMD(&_handle, 1s, "sk:connect(80,\"%s\")\r\n", address);
    flushBuffer();
    printCMD(&_handle, 1s, "sk:send(\"GET %s HTTP/1.1\\r\\nHost: %s\\r\\n%s\\r\\n\\r\\n\")\r\n", payload, address, header);
    getreply_json(respBuffer, respBufferSize);
    return 1;
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
        else if (c == '>' || c =='\r'){
            _serial.read(&c, 1);
            return true;
        }
    }
}

void WifiClient::flushBuffer(int len){
    Timer t;
    t.start();
    int n = 0;
    char c;
    while(t.elapsed_time() < 1s && n!=len){
        if(_serial.readable()){
            _serial.read(&c, 1);
            n++;
            //pc.write(&c, 1); //DEBUG ONLY
        }
    }
}



//todo: clean up
int WifiClient::getreply(char* resp, int size){
    if(!discardEcho()){
        return false;
    }
    Timer t;
    t.start();
    char c;
    int cnt = 0;
    while(t.elapsed_time() < 3s){
        if(_serial.readable()){
            _serial.read(&c, 1);
            //special case to filter
            if(c == '>'){
                flushBuffer(2);
                continue;
            }

            if(resp){
                strncat(resp, &c, 1);
                cnt++;
            }
            if(cnt == size)break;
            //pc.write(&c, num); //DEBUG ONLY
        }
    }
    flushBuffer();
    return 1;
}

int WifiClient::getreply_json(char* resp, int size){
    if(!discardEcho()){
        return false;
    }
    Timer t;
    t.start();
    char c;
    int cnt = 0;
    int fwd_brackets = 0;
    while(t.elapsed_time() < 10s){
        if(_serial.readable()){
            _serial.read(&c, 1);
            //special case to filter
            if(c == '{'){
                fwd_brackets++;
            }else if(c=='}'){
                fwd_brackets--;
            }

            if(resp && fwd_brackets>0){
                strncat(resp, &c, 1);
                cnt++;
            }
            if(cnt == size)break;
            //pc.write(&c, num); //DEBUG ONLY
        }
    }
    flushBuffer();
    return 1;
}

int WifiClient::getreply_xml(char* resp, int size){
    if(!discardEcho()){
        return false;
    }
    Timer t;
    t.start();
    char c;
    int cnt = 0;
    bool in_xml = false;
    while(t.elapsed_time() < 10s){
        if(_serial.readable()){
            _serial.read(&c, 1);
            //special case to filter
            if(c == '<'){
                in_xml=true;
            }else if(c=='\r'){
                in_xml=false;
            }
            
            if(resp && in_xml){
                strncat(resp, &c, 1);
                cnt++;
            }
            if(cnt == size)break;
            //pc.write(&c, num); //DEBUG ONLY
        }
    }
    flushBuffer();
    return 1;
}