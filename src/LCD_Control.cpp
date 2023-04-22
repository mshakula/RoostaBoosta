/// \file LCD_Control.cpp
/// \date 2023-04-22
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief Functions to control the LCD.

#include "LCD_Control.hpp"

#include <4DGL-uLCD-144-MbedOS6/uLCD_4DGL.hpp>
#include <mbed.h>

#include "weather_data.hpp"

// ======================= Local Definitions =========================

namespace {

uLCD_4DGL uLCD(p28, p27, p30); // serial tx, serial rx, reset pin;
char      line[30];

} // namespace

// ====================== Global Definitions =========================

void
Display_Weather(weather_data* data)
{
  uLCD.cls();
  ThisThread::sleep_for(100ms);
  // header
  sprintf(line, "Weather Outside:");
  uLCD.text_string(line, 1, 1, FONT_7X8, GREEN);
  ThisThread::sleep_for(100ms);
  // format temp
  sprintf(line, "Temp: %d °F", data->temperature);
  uLCD.text_string(line, 1, 2, FONT_7X8, GREEN);
  ThisThread::sleep_for(100ms);
  // format precipitation_chance
  sprintf(line, "Rain Chance: %d%%", data->precipitation_chance);
  uLCD.text_string(line, 1, 3, FONT_7X8, GREEN);
  ThisThread::sleep_for(100ms);
  // format wind_speed
  sprintf(line, "Wind: %d mph", data->wind_speed);
  uLCD.text_string(line, 1, 4, FONT_7X8, GREEN);
  ThisThread::sleep_for(100ms);
  // format humidity
  sprintf(line, "Humidity: %d%%", data->humidity);
  uLCD.text_string(line, 1, 5, FONT_7X8, GREEN);
  ThisThread::sleep_for(100ms);
  // format weather part 1
  sprintf(line, "%.15s", data->weather);
  uLCD.text_string(line, 1, 7, FONT_7X8, GREEN);
  ThisThread::sleep_for(100ms);
  // format weather part 2
  sprintf(line, "%.15s", data->weather + 15);
  uLCD.text_string(line, 1, 8, FONT_7X8, GREEN);
  ThisThread::sleep_for(100ms);
}