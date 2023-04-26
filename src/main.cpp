/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief The main entrypoint for the program.

#include <array>
#include <string>
#include <string_view>

#include "WifiClient.hpp"
#include <mbed.h>
#include <mbed_error.h>

#include <FATFileSystem.h>
#include <SDBlockDevice.h>
#include <hal/spi_api.h>

#include "LCD_Control.hpp"
#include "MusicPlayer.h"
#include "audio_player.hpp"
#include "pinout.hpp"
#include "weather_data.hpp"

using namespace rb;

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

WifiClient wifi(rb::pinout::kWifi_tx, rb::pinout::kWifi_rx, NC);
char       ssid[32] = "test";    // enter WiFi router ssid inside the quotes
char       pwd[32] = "test1234"; // enter WiFi router password inside the quotes
const char* addr   = "api.weatherapi.com";
const char* payload =
  "/v1/forecast.json?key=a9e3fb6a760c49699d625304232504&q=Atlanta&aqi=no";
const char* header = "Accept: application/xml";

int
startWifi()
{
  wifi.init();
  // char ap_list[256];
  // wifi.scan(ap_list, 256);
  wifi.connect(ssid, pwd);
  return 1;
}

int
extractJsonInt(std::string_view raw, std::string_view query)
{
  constexpr auto npos = std::string_view::npos;

  auto query_index = raw.find(query, 0);
  if (query_index == npos)
    MBED_ERROR(MBED_ERROR_INVALID_DATA_DETECTED, "Query not found");
  auto body_index = query_index + query.size() + 2;

  return std::atoi(&raw[body_index]);
}

std::string
extractJsonStr(std::string_view raw, std::string_view query)
{
  constexpr auto npos = std::string_view::npos;

  auto query_index = raw.find(query, 0);
  if (query_index == npos)
    MBED_ERROR(MBED_ERROR_INVALID_DATA_DETECTED, "Query not found");
  auto body_index = query_index + query.size() + 3;
  auto end_index  = raw.find("\"", body_index);

  return std::string(&raw[body_index], end_index - body_index);
}

int
updateweather(weather_data* data)
{
  std::array<char, 2048> resp = {0};

  debug("\r\n[updateweather] Getting request...");
  wifi.http_get_request(addr, payload, header, resp.data(), resp.size());
  debug(" done.");

  debug("\r\n[updateweather] RESP DUNMP: %s\r\n============\r\n", resp.data());

  debug("\r\n[updateweather] Parsing response...");
  debug("\r\n\t humidity...");
  data->humidity =
    extractJsonInt(std::string_view{resp.data(), resp.size()}, "humidity");
  debug("\r\n\t precip...");
  data->precipitation_chance = extractJsonInt(
    std::string_view{resp.data(), resp.size()}, "daily_chance_of_rain");
  debug("\r\n\t temperature...");
  data->temperature =
    extractJsonInt(std::string_view{resp.data(), resp.size()}, "temp_f");
  debug("\r\n\t wind_speed...");
  data->wind_speed =
    extractJsonInt(std::string_view{resp.data(), resp.size()}, "wind_mph");
  debug("\r\n\t weather...");
  data->weather =
    extractJsonStr(std::string_view{resp.data(), resp.size()}, "text");
  debug(" done.");

  return 1;
}

} // namespace

// ====================== Global Definitions =========================
int
main()
{
  // wifi
  printf("Starting demo...\n");
  startWifi();
  printf("Connected! Beginning HTTP get...\n");
  weather_data  data_;
  weather_data* data = &data_;
  updateweather(data);

  debug("\r\n\t[main] Weather Data: {");
  debug("\r\n\tHumidity: %d%", data->humidity);
  debug("\r\n\tPrecipitation Chance: %d%", data->precipitation_chance);
  debug("\r\n\tTemperature: %d degrees F", data->temperature);
  debug("\r\n\tWind Speed: %d mph", data->wind_speed);
  debug("\r\n\tWeather Description: %s", data->weather.c_str());
  debug("\r\n\tFinished.");
  debug("\r\n}");

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
  while (true) {
    Display_Weather(data);
    ThisThread::sleep_for(1s);
    play_audio(data);
    ThisThread::sleep_for(10s);
  }
}
