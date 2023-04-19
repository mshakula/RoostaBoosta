#include "esp8266interface_lua.h"

ESP8266Interface_Lua::ESP8266Interface_Lua(PinName tx, PinName rx, PinName reset, 
                                   int baud, int timeout) :
        ESP8266_Lua(tx, rx, reset, baud, timeout) {
}

bool ESP8266Interface_Lua::init() {
    return ESP8266_Lua::init();
}

bool ESP8266Interface_Lua::connect(const char * ssid, const char * phrase) {
    return ESP8266_Lua::connect(ssid, phrase);
}
/*
int ESP8266Interface_Lua::disconnect() {
    return ESP8266_Lua::disconnect();
}

const char *ESP8266Interface_Lua::getIPAddress() {
    return ESP8266_Lua::getIPAddress();
}*/