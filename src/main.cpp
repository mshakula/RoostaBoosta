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

#include "LCD_Control.hpp"
#include "MusicPlayer.h"
#include "pinout.hpp"
#include "weather_data.hpp"

// ======================= Local Definitions =========================

namespace {

int
printdir()
{
  DIR* d = opendir(SCRATCH_DIR);
  if (!d)
    return 1;
  printf("[main] Dumping %s: {\r\n", SCRATCH_DIR);
  for (struct dirent* e = readdir(d); e; e = readdir(d))
    printf("  %s\r\n", e->d_name);
  printf("}\r\n");
  closedir(d);
  return 0;
}

std::string
select_random_pcm()
{
  DIR* d = opendir(AUDIO_DIR);
  if (!d)
    MBED_ERROR(
      MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno),
      "Could not open root file directory.");

  std::array<std::uint16_t, 100> pcm_files_indices;
  int                            pcm_files_count = 0;
  {
    int i = 0;
    for (struct dirent* e = readdir(d);
         e && pcm_files_count < pcm_files_indices.size();
         e = readdir(d)) {
      std::string_view name = e->d_name;
      std::string_view ext  = name.substr(name.find_last_of('.'));
      if (ext == ".pcm") {
        pcm_files_indices[pcm_files_count++] = i;
      }
      ++i;
    }
  }
  if (errno) {
    MBED_ERROR(
      MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno),
      "Error in traversing directory.");
  }
  if (pcm_files_indices.empty()) {
    MBED_ERROR(
      MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, ENOENT),
      "No PCM files found in root file directory.");
  }

  rewinddir(d);
  {
    int            i           = 0;
    int            target_file = pcm_files_indices[rand() % pcm_files_count];
    struct dirent* e;

    do {
      e = readdir(d);
    } while (i++ != target_file);

    std::string ret = std::string(AUDIO_DIR) + std::string(e->d_name);
    closedir(d);
    return ret;
  }
}

} // namespace

// ====================== Global Definitions =========================

int
main()
{
  debug("[main] Starting up.\r\n");

  debug("[main] Seeding rand...");
  srand(time(NULL));
  debug(" rand() = %d... done.\r\n", rand() % 100);

  debug("[main] Initializing SD Block Device...");
  SDBlockDevice sd(
    rb::pinout::kSD_mosi,
    rb::pinout::kSD_miso,
    rb::pinout::kSD_sck,
    rb::pinout::kSD_cs);
  debug(" done.\r\n");

  debug("[main] Mounting SD card...");
  FATFileSystem fs(AUX_MOUNT_POINT, &sd);
  debug(" done.\r\n");

  // debug("[main] Reformatting SD card...");
  // if (fs.reformat(&sd)) {
  //   MBED_ERROR(
  //     MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno),
  //     "Could not reformat SD card.");
  // }
  // debug(" done.\r\n");

  debug("[main] Opening root file directory...");
  if (printdir()) {
    MBED_ERROR(
      MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno),
      "Could not open root file directory.");
  }
  debug(" done.\r\n");

  // debug("[main] Testing open/close file...");
  // FILE* f = fopen(SCRATCH_DIR "test.txt", "w");
  // if (!f) {
  //   MBED_ERROR(
  //     MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno), "Could not open
  //     file.");
  // }
  // if (fclose(f)) {
  //   MBED_ERROR(
  //     MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno), "Could not close
  //     file.");
  // }
  // debug(" done.\r\n");

  // debug("[main] Testing creating / destroy directory...");
  // if (mkdir(SCRATCH_DIR "mydir", 0777)) {
  //   MBED_ERROR(
  //     MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno),
  //     "Could not create directory.");
  // }
  // if (remove(SCRATCH_DIR "mydir")) {
  //   MBED_ERROR(
  //     MBED_MAKE_ERROR(MBED_MODULE_FILESYSTEM, errno),
  //     "Could not destroy directory.");
  // }
  // debug(" done.\r\n");

  while (true) {
    debug("[main] Selecting random PCM file...");
    auto f_name = select_random_pcm();
    debug(" done.\r\n");

    debug("[main] Playing file %s...", f_name.c_str());
    playMusic(f_name.c_str(), 1.0);
    debug(" done.\r\n");
  }
}
