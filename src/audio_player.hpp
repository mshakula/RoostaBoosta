/// \file audio_player.hpp
/// \date 2023-04-22
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief header fuctions to control the speaker

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include "weather_data.h"

// ======================= Public Interface ==========================

/// \brief reads the weather data on the speaker
///
/// \param data The weather data to print.
extern "C" void
play_audio(weather_data* data);

// ===================== Detail Implementation =======================

#endif // AUDIO_PLAYER_H
