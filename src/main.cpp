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
#include "WifiClient.h"

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

namespace{

WifiClient wifi(p28, p27, p26);    
char ssid[32] = "test";     // enter WiFi router ssid inside the quotes
char pwd [32] = "test1234"; // enter WiFi router password inside the quotes
const char* addr = "api.weatherapi.com";
const char* payload = "/v1/forecast.json?key=a9e3fb6a760c49699d625304232504&q=Atlanta&aqi=no";
const char* header = "Accept: application/xml";

int 
startWifi()
{
    wifi.init();
    //char ap_list[256];
    //wifi.scan(ap_list, 256);
    wifi.connect(ssid,pwd);
    return 1;
}

int
searchChars(char* chars, const char* query, size_t chars_size, size_t query_size, int start_pos=0)
{
    bool flag;
    for(int i=start_pos; i<chars_size; i++)
    {
        flag=true;
        for(int j=0; j<query_size; j++)
        {
            if(chars[i+j]!=query[j])
            {
                flag=false;
                break;
            } 
        }
        if(flag)return i;
    }
    return -1;
}

int 
extractJsonInt(char* raw, const char* query, size_t raw_size, size_t query_size)
{
    int ind = searchChars(raw, query, raw_size, query_size, 0) + query_size + 2;
    return atoi(&raw[ind]);
}

std::string
extractJsonStr(char* raw, const char* query, size_t raw_size, size_t query_size){
    int ind = searchChars(raw, query, raw_size, query_size, 0) + query_size + 3;
    int len = searchChars(raw, "\"", raw_size, 1, ind) - ind;
    return std::string(raw, ind, len);
}

int
updateweather(weather_data* data)
{
    char resp[2048];
    memset(resp, '\0', sizeof(resp));
    wifi.http_get_request(addr, payload, header, resp, 2048);
    data->humidity             = extractJsonInt(resp, "humidity",             sizeof(resp), 8);
    data->precipitation_chance = extractJsonInt(resp, "daily_chance_of_rain", sizeof(resp), 20);
    data->temperature          = extractJsonInt(resp, "temp_f",               sizeof(resp), 6);
    data->wind_speed           = extractJsonInt(resp, "wind_mph",             sizeof(resp), 8);
    data->weather              = extractJsonStr(resp, "text",                 sizeof(resp), 4).c_str();
    
    return 1;
}

} //namespace

// ====================== Global Definitions =========================
int
main()
{
  //wifi
    printf("Starting demo...\n");
    startWifi();
    printf("Connected! Beginning HTTP get...\n");
    weather_data* data = (weather_data*)malloc(sizeof(weather_data));
    updateweather(data);
    printf("Humidity: %d%\n",             data->humidity);
    printf("Precipitation Chance: %d%\n", data->precipitation_chance);
    printf("Temperature: %d degrees F\n", data->temperature);
    printf("Wind Speed: %d mph\n",        data->wind_speed);
    printf("Weather Description: %s\n",   data->weather);
    printf("Finished.\n");
    
    
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
