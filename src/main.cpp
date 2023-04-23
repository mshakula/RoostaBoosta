/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
///
/// \brief The main entrypoint for the program.

#include <mbed.h>
#include "WifiClient.h"
// ======================= Local Definitions =========================

namespace {

} // namespace

// ====================== Global Definitions =========================
WifiClient wifi(p28, p27, p26);
char ssid[32] = "test";     // enter WiFi router ssid inside the quotes
char pwd [32] = "test1234"; // enter WiFi router password inside the quotes

int
main()
{
    printf("Starting demo...\n");
    wifi.init();
    printf("Disconnecting from all AP's...\n");
    wifi.disconnect();
    printf("connecting...\n");
    if(wifi.connect(ssid,pwd)){
        printf("Connected!\n");
        //char ip[16];
        //wifi.get_ip(ip);
        //doesn't work for some reason. need bugfix
        //printf("%s", ip);
    }
    printf("Demo completed.");
}
