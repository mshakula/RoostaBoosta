/// \file audio_player.hpp
/// \date 2023-04-22
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief header fuctions to control the speaker

#ifndef AUDIO_PLAYER_HPP
#define AUDIO_PLAYER_HPP

#include "weather_data.hpp"

// ======================= Public Interface ==========================

/// \brief reads the weather data on the speaker
///
/// \param data The weather data to print.
void
play_audio(weather_data* data);

/// \brief plays the alarm sound
void
play_alarm();

// ===================== Detail Implementation =======================

#endif // AUDIO_PLAYER_HPP
