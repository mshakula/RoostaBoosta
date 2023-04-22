/// \file MusicPlayer.h
/// \date 2023-04-22
/// \author mshakula (mshakula3)
///
/// \brief The music playing function

#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

// ======================= Public Interface ==========================

/// \brief Play the music file at the given speed.
///
/// The function is non-reentrant, as such a mutex is used to ensure unique
/// access. Calling thread will block until the music is done playing.
///
/// \note As the sampling frequency of the file increases, the time drift of the
/// music player is delayed. It will play notes at their correct frequencies and
/// everything (calibrated to do so), however it will have significant slowdown
/// due to "crunchiness".
///
/// \param file_name The name of the file to play.
/// \param initial_speed The initial speed of the music player.
extern "C" void
playMusic(const char* file_name, double initial_speed);

// ===================== Detail Implementation =======================

#endif // MUSIC_PLAYER_H
