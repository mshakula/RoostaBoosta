/// \file LCD_Control.hpp
/// \date 2023-04-22
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief Header for LCD control.

#ifndef LCD_CONTROL_HPP
#define LCD_CONTROL_HPP

#ifndef __cplusplus
#error "LCD_Control.hpp is a cxx-only header."
#endif // __cplusplus

#include "weather_data.hpp"

// ======================= Public Interface ==========================

/// \brief Prints the weather data to the LCD.
///
/// \param data The weather data to print.
void
Display_Weather(weather_data* data);

// ===================== Detail Implementation =======================

#endif // LCD_CONTROL_HPP
