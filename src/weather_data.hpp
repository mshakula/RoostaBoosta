/// \file weather_data.hpp
/// \date 2023-04-21
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief Structure for weather API data.

#ifndef WEATHER_DATA_HPP
#define WEATHER_DATA_HPP

#ifndef __cplusplus
#error "weather_data.hpp is a c++-only header"
#endif // __cplusplus

#include <string>

// ======================= Public Interface ==========================

/// \brief Structure for weather API data.
struct weather_data
{
  int         humidity;
  int         precipitation_chance;
  int         temperature;
  int         wind_speed;
  std::string weather;
};

// ===================== Detail Implementation =======================

#endif // WEATHER_DATA_HPP
