/// \file weather_data.hpp
/// \date 2023-04-21
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief Structure for weather API data.

#ifndef WEATHER_DATA_HPP
#define WEATHER_DATA_HPP

#ifndef __cplusplus
#error "weather_data.hpp is a cxx-only header."
#endif // __cplusplus

// ======================= Public Interface ==========================

/// \brief Structure for weather API data.
struct weather_data
{
  int         humidity;
  int         precipitation_chance;
  int         temperature;
  int         wind_speed;
  const char* weather; // static size arr to avoid extra pointers
};

// ===================== Detail Implementation =======================

#endif // WEATHER_DATA_HPP
