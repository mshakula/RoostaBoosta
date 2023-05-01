/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief The main entrypoint for the program.

#include <array>
#include <string>
#include <string_view>

#include "WifiClient.hpp"
#include <mbed.h>
#include <mbed_error.h>

#include <FATFileSystem.h>
#include <SDBlockDevice.h>
#include <hal/spi_api.h>

#include "HTTPClient.hpp"
#include "LCD_Control.hpp"
#include "MusicPlayer.h"
#include "audio_player.hpp"
#include "pinout.hpp"
#include "weather_data.hpp"

using namespace rb;

// ======================= Local Definitions =========================

namespace {

constexpr std::size_t kBufSize = 1024;
char                  buf_[kBufSize];

mbed::DigitalOut led(LED2);
mbed::DigitalOut badLed(LED4);

} // namespace

// ====================== Global Definitions =========================

int
main()
{
  debug("\r\n[main] Starting up...");

  SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
    SCB_SHCSR_MEMFAULTENA_Msk;

  constexpr HTTPRequest req = []() constexpr {
    HTTPRequest req;
    req.method = "GET"sv;
    req.uri =
      "http://api.weatherapi.com/v1/"
      "forecast.json?key=a9e3fb6a760c49699d625304232504&q=Atlanta&aqi=no";
    req.general_header.connection = "close";
    req.request_header.user_agent = "mbed";
    req.request_header.host       = "api.weatherapi.com";
    req.request_header.accept     = "application/xml";
    req.message_body              = "{name: \"RoostaBoosta\"}";
    return req;
  }();
  led = 1;

  {
    mbed::Span buf{buf_, kBufSize};

    auto s_handle = req.get_serialization_handle();
    while (s_handle.serialize(buf)) {
      led = !led;
      buf = mbed::Span{
        buf.data() + s_handle.gcount(), buf.size() - s_handle.gcount()};
      rtos::ThisThread::sleep_for(100ms);
    }
    buf[s_handle.gcount()] = '\0';
    debug("\r\n[main] what the fuck... %d", s_handle.gcount());
    if (s_handle.fail()) {
      led = !led;
      RB_ERROR(s_handle.fail());
    }
    debug("\r\nRequest data:{\r\n%s}", buf_);
    debug("\r\nGoing to run dtor now...");
  }

  do {
    led = !led;
    ThisThread::sleep_for(100ms);
  } while (true);
}
