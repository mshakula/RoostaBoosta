/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief The main entrypoint for the program.

#include <mbed.h>

#include "LCD_Control.hpp"
#include "weather_data.hpp"

// ======================= Local Definitions =========================

namespace {

} // namespace

// ====================== Global Definitions =========================

int
main()
{
  DigitalOut    led(LED1);
  weather_data* data         = (weather_data*)malloc(sizeof(weather_data));
  data->humidity             = 100;
  data->precipitation_chance = 100;
  data->temperature          = 78;
  data->wind_speed           = 15;
  data->weather              = "Chance Rain Showers";
  while (true) {
    led = !led;
    Display_Weather(data);
    ThisThread::sleep_for(10s);
  }
}
