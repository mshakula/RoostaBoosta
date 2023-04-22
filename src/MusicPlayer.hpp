/// \file MusicPlayer.hpp
/// \date 2023-04-22
/// \author mshakula (mshakula3)
///
/// \brief THe music playing function

#ifndef MUSIC_PLAYER_HPP
#define MUSIC_PLAYER_HPP

#ifndef __cplusplus
#error "MusicThread.hpp is a cxx-only header."
#endif // __cplusplus

// ======================= Public Interface ==========================

/// \brief Play the music file at the given speed.
///
/// \note As the sampling frequency of the file increases, the time drift of the
/// music player is delayed. It will play notes at their correct frequencies and
/// everything (calibrated to do so), however it will have significant slowdown
/// due to "crunchiness".
///
/// \param file_name The name of the file to play.
/// \param initial_speed The initial speed of the music player.
void
playMusic(const char* file_name, double initial_speed);

// ===================== Detail Implementation =======================

#endif // MUSIC_PLAYER_HPP
