/// \file weather_data.h
/// \date 2023-04-21
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief Structure for weather API data.

#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

// ======================= Public Interface ==========================

/// \brief Structure for weather API data.
extern "C" struct weather_data
{
  int         humidity;
  int         precipitation_chance;
  int         temperature;
  int         wind_speed;
  const char* weather;
};

// ===================== Detail Implementation =======================

#endif // WEATHER_DATA_H
