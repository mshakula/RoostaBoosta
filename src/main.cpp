/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
///
/// \brief The main entrypoint for the program.

#include <mbed.h>
#include "esp8266interface_lua.h"
// ======================= Local Definitions =========================

namespace {

} // namespace

// ====================== Global Definitions =========================

ESP8266Interface_Lua wifi(p28, p27, p26);
char ssid[32] = "test";     // enter WiFi router ssid inside the quotes
char pwd [32] = "test12345"; // enter WiFi router password inside the quotes

int
main()
{
  wifi.reset();
  printf("connecting...\n");
  wifi.connect(ssid,pwd);
  printf("%d", status);
  printf("\n\nfinished\n\n");
}
