/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief The main entrypoint for the program.

#include <array>
#include <string>
#include <string_view>

#include <mbed.h>

#include <FATFileSystem.h>
#include <SDBlockDevice.h>
#include <hal/spi_api.h>

#include "LCD_Control.hpp"
#include "MusicPlayer.h"
#include "audio_player.hpp"
#include "pinout.hpp"
#include "weather_data.h"

// ======================= Local Definitions =========================

namespace {

int
printdir()
{
  DIR* d = opendir(SCRATCH_DIR);
  if (!d)
    return 1;
  printf("\r\n[main] Dumping %s: {", SCRATCH_DIR);
  for (struct dirent* e = readdir(d); e; e = readdir(d))
    printf("\r\n  %s", e->d_name);
  printf("\r\n}");
  closedir(d);
  return 0;
}

} // namespace

// ====================== Global Definitions =========================

int
main()
{
  debug("\r\n[main] Starting up.");

  debug("\r\n[main] Seeding rand...");
  srand(time(NULL));
  debug(" rand() = %d... done.", rand() % 100);

  debug("\r\n[main] Initializing SD Block Device...");
  SDBlockDevice sd(
    rb::pinout::kSD_mosi,
    rb::pinout::kSD_miso,
    rb::pinout::kSD_sck,
    rb::pinout::kSD_cs);
  {
    spi_capabilities_t caps;
    spi_get_capabilities(rb::pinout::kSD_cs, true, &caps);
    debug(" maxumum speed: %d...", caps.maximum_frequency);
    sd.frequency(caps.maximum_frequency);
  }
  debug(" done.");

  debug("\r\n[main] Mounting SD card...");
  FATFileSystem fs(AUX_MOUNT_POINT, &sd);
  debug(" done.");

  debug("\r\n[main] Opening root file directory...");
  if (printdir()) {
    MBED_ERROR(
      MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno),
      "Could not open root file directory.");
  }
  debug(" done.");

  debug("\r\n[main] Running weather demo...");
  weather_data* data         = (weather_data*)malloc(sizeof(weather_data));
  data->humidity             = 35;
  data->precipitation_chance = 76;
  data->temperature          = 78;
  data->wind_speed           = 15;
  const char* w              = "it is partly cloudy";
  data->weather              = w;
  while (true) {
    Display_Weather(data);
    ThisThread::sleep_for(1s);
    play_audio(data);
    ThisThread::sleep_for(10s);
  }
}
