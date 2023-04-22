/// \file MusicPlayer.cpp
/// \date 2023-03-04
/// \author mshakula (matvey@gatech.edu)
///
/// \brief The MusicPlayer implementation. In separate file because it is quite
/// complex.

#include "MusicPlayer.h"

#include <cstdint>
#include <cstring>

#include <mutex>

#include <mbed.h>
#include <rtos.h>

#include <MODDMA.h>

#include "pinout.hpp"

using namespace AjK; // for MODDMA.

// ======================= Local Definitions =========================

namespace {

/// \brief The number of banks of audio data.
constexpr int kBankCount = 2;

/// \brief Initialize the DMA controller.
MODDMA DMA;

/// \brief Onboard LEDs.
mbed::BusOut OnboardLEDs(LED4, LED3, LED2, LED1);

/// \brief The output pin.
mbed::AnalogOut audio_out(rb::pinout::kAudio_out);

/// \brief Configure the DAC clock to be in-phase with the CPU Clock.
///
/// \return Clock speed.
int
configDACClock_()
{
  LPC_SC->PCLKSEL0 |= 0x1 << 22; // PCLK_DAC = CCLK
  return CCK_SPEED;
}

// switching audio buffer.
std::uint32_t audio_buf[kBankCount][MUSIC_PLAYER_AUDIO_BUF_BANK_SIZE]
  __attribute__((section("AHBSRAM0")));

/// \brief The callback functor type for when the DMA encounters an error.
struct ErrorCallback_
{
  void operator()() { error("Error in DMA Callback"); }
};

/// \brief The callback functor type for when the music player DAC runs out of
/// samples.
struct DataCallback_
{
  osThreadId     tid;
  volatile int&  curr_bank;
  MODDMA_Config* bank_conf;

  DataCallback_(
    osThreadId     tid_,
    volatile int&  curr_bank_,
    MODDMA_Config* bank_conf_) :
      tid(tid_),
      curr_bank(curr_bank_),
      bank_conf(bank_conf_)
  {
  }

  void operator()()
  {
    curr_bank = (curr_bank + 1) % kBankCount;
    DMA.Disable((MODDMA::CHANNELS)DMA.getConfig()->channelNum());
    DMA.Prepare(&bank_conf[curr_bank]);
    if (DMA.irqType() == MODDMA::TcIrq)
      DMA.clearTcIrq();

    osSignalSet(tid, EVENT_FLAG_AUDIO_LOAD);
  }
};

// clang-format off
/// \brief Supported file types.
enum FileType_
{
  FileType_Undefined = 0
, FileType_u8pcm    = 1
};
// clang-format on

struct U8PCMFileInfo_
{
  FILE* file;

  void destroy() { std::fclose(file); }
};

/// \brief Structure about a file. Very heavy.
struct FileInfo_
{
  const char* name;
  FileType_   type;
  int         rate;
  union {
    U8PCMFileInfo_ u8pcm;
  };
};

/// \brief Queries the file, and fills out file info structure.
FileType_
initFile_(const char* fname, FileInfo_& info)
{
  // Get name.
  info.name = fname;

  { // Get type.
    const char* dot = std::strrchr(fname, '.');
    if (!dot || dot == fname)
      info.type = FileType_u8pcm;
    else
      info.type = FileType_u8pcm;
  }

  // Get rate and init decoding structs.
  switch (info.type) {

    case FileType_u8pcm: {
      info.u8pcm.file = std::fopen(info.name, "rb");
      if (!info.u8pcm.file)
        goto err;
      info.rate = 0;
    } break;

    default:
      break;
  }

  return info.type;

err:
  info.type = FileType_Undefined;
  return info.type;
}

void
deinitFile_(FileInfo_& info)
{
  switch (info.type) {
    case FileType_u8pcm:
      info.u8pcm.destroy();
      break;

    default:
      break;
  }
}

/// \brief Helper to read into audio buffer from file.
///
/// \return 0 on success, 1 on failure.
int
readBuffer_(FileInfo_& file_info, bool& more, std::uint32_t* buffer)
{
  switch (file_info.type) {
    case FileType_u8pcm: {
      FILE* const fp = file_info.u8pcm.file;
      std::size_t read_ct =
        std::fread(buffer, 1, MUSIC_PLAYER_AUDIO_BUF_BANK_SIZE, fp);
      if (read_ct < MUSIC_PLAYER_AUDIO_BUF_BANK_SIZE) {
        if (std::ferror(fp)) {
          return 1;
        } else if (std::feof(fp)) {
          std::memset(
            buffer + read_ct,
            0,
            (MUSIC_PLAYER_AUDIO_BUF_BANK_SIZE - read_ct) * sizeof(*buffer));
          more = false;
        }
      }
      for (int i = read_ct - 1; i >= 0; --i)
        buffer[i] = reinterpret_cast<std::uint8_t*>(buffer)[i] << 8;
    } break;

    default:
      return 1;
  }
  return 0;
}

} // namespace

// ====================== Global Definitions =========================

extern "C" void
playMusic(const char* file_name, double initial_speed)
{
  // Non-reentrant function due to need of static variables and contention on
  // DMA bus.
  static rtos::Mutex mutex;
  std::scoped_lock   lock(mutex);

  static const int kClockFreq = configDACClock_();

  // allocate large decoding structs in static memory.
  static FileInfo_ file_info;

  bool more = true;

  volatile int   curr_bank = 0;
  MODDMA_Config  bank_conf[kBankCount];
  DataCallback_  callback_d(osThreadGetId(), curr_bank, bank_conf);
  ErrorCallback_ callback_e;

  // Open file and get its data.
  if (!initFile_(file_name, file_info)) {
    error("[MusicPlayer] Cannot open file %s!", file_info.name);
    return;
  }

  // Fill initial two buffer banks.
  for (int i = 0; i < kBankCount; ++i) {
    if (readBuffer_(file_info, more, audio_buf[i])) {
      error("[MusicPlayer] Error reading file %s!", file_info.name);
      goto end;
    }
  }

  debug("\r\n[MusicPlayer] Loaded initial banks.");

  // Configure Banks
  for (int i = 0; i < kBankCount; ++i) {
    (&bank_conf[i])
      ->srcMemAddr(reinterpret_cast<std::uint32_t>(&audio_buf[i]))
      ->dstMemAddr(MODDMA::DAC)
      ->transferSize(sizeof(audio_buf[i]) / sizeof(audio_buf[i][0]))
      ->transferType(MODDMA::m2p)
      ->dstConn(MODDMA::DAC)
      ->attach_tc(&callback_d, &DataCallback_::operator())
      ->attach_err(&callback_e, &ErrorCallback_::operator());
  }
  bank_conf[0].channelNum(MODDMA::Channel_0);
  bank_conf[1].channelNum(MODDMA::Channel_1);

  debug("\r\n[MusicPlayer] Configured initial banks.");

  // Start DMA to DAC.
  if (!DMA.Setup(&bank_conf[0])) {
    error("[MusicPlayer] Error in initial DMA Setup()!");
    goto end;
  }

  // Configure and start DAC. Assume 24kHz for PCM (seems to work well @ this
  // speed).
  LPC_DAC->DACCNTVAL = static_cast<std::uint16_t>(
    kClockFreq / initial_speed / 2 / (file_info.rate ? file_info.rate : 24000));
  LPC_DAC->DACCTRL |= 0xC; // Start running DAC.

  debug("\r\n[MusicPlayer] DAC enabled.");

  DMA.Enable(&bank_conf[0]);

  debug("\r\n[MusicPlayer] DMA enabled.");

  // Start audio buffering loop.
  debug("\r\n[MusicPlayer] Starting audio buffering idle loop.");
  osSignalWait(EVENT_FLAG_AUDIO_LOAD, osWaitForever);
  while (more) {
    // debug("\r\n[MusicPlayer] Fetching more from file...");
    int next_bank = (curr_bank - 1 + kBankCount) % kBankCount;
    if (readBuffer_(file_info, more, audio_buf[next_bank])) {
      error("[MusicPlayer] Error fetching more from file %s!", file_info.name);
      goto end2;
    }
    // debug(" done.");
    osSignalWait(EVENT_FLAG_AUDIO_LOAD, osWaitForever);
  }
  debug("\r\n[MusicPlayer] Finished playing audio.");

end2:
  LPC_DAC->DACCTRL &= ~(0xC); // Stop running DAC.
  DMA.Disable(MODDMA::Channel_0);
  DMA.Disable(MODDMA::Channel_1);
end:
  deinitFile_(file_info);
}
