/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief The main entrypoint for the program.

#include <mbed.h>

#include <FATFileSystem.h>
#include <SDBlockDevice.h>

#include "LCD_Control.hpp"
#include "MusicPlayer.hpp"
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

} // namespace

// ====================== Global Definitions =========================

int
main()
{
  debug("[main] Starting up.\r\n");

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

  debug("[main] Playing with file...");
  playMusic(MUSIC_DEMO_FILE, 1.0);
  debug(" done.\r\n");

end0:
  do {
    printf("AM DEAD\r\n");
    ThisThread::sleep_for(1s);
  } while (true);
}
