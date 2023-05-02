/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief The main entrypoint for the program.

#include <array>
#include <string>
#include <string_view>
#include <chrono>

#include "WifiClient.hpp"
#include <mbed.h>
#include <mbed_error.h>

#include <FATFileSystem.h>
#include <SDBlockDevice.h>
#include <hal/spi_api.h>
#include <RTC.h>

#include "LCD_Control.hpp"
#include "MusicPlayer.h"
#include "audio_player.hpp"
#include "sonar.hpp"
#include "pinout.hpp"
#include "weather_data.hpp"

using namespace rb;

// ======================= Local Definitions =========================

namespace {
 
InterruptIn turnOff(rb::pinout::kBtn1, PullUp);
InterruptIn dispWeather(rb::pinout::kBtn2, PullUp);
bool alarm_on = false;
void alarmFunction( void )
{
    alarm_on = true;
    RTC::alarmOff();
}

bool disp_weather=false;
void
pb_dispweather()
{
  disp_weather = true;
}

#define snoozetime 15s
void
pb_turnoff()
{
  alarm_on = false;
}

void
alarm(weather_data* data)
{
  while(alarm_on)
  {
  play_alarm();
  if(Is_Snoozed())
  {
    ThisThread::sleep_for(snoozetime);
  }
  }
  Display_Weather(data);
  play_audio(data);
}

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
char payload[128] = "/v1/forecast.json?key=a9e3fb6a760c49699d625304232504&q=Atlanta&aqi=no";
const char* header = "Accept: application/xml";
char location[32];

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

int
updatetime()
{
  std::array<char, 2048> resp = {0};
  wifi.http_get_request(addr, payload, header, resp.data(), resp.size());
  std::string tzid = extractJsonStr(std::string_view{resp.data(), resp.size()}, "tz_id");
  char tz_payload[32];
  snprintf(tz_payload, 32, "/api/timezone/%s", tzid.c_str());
  resp = {0};
  wifi.http_get_request("worldtimeapi.org", tz_payload, "Accept: */*", resp.data(), resp.size());
  set_time(extractJsonInt(std::string_view{resp.data(), resp.size()}, "unixtime") + 
          extractJsonInt(std::string_view{resp.data(), resp.size()}, "raw_offset") + 
          extractJsonInt(std::string_view{resp.data(), resp.size()}, "dst_offset"));
  return 1;
}

BufferedSerial bt(rb::pinout::kBT_tx, rb::pinout::kBT_rx);

void bt_clear_buf()
{
  Timer t;
  t.start();
  char c;
  while(t.elapsed_time() < 1s){
      if(bt.readable()){
          bt.read(&c, 1);
          //pc.write(&c, 1); //DEBUG ONLY
      }
  }
}

void bt_print_connection_status()
{
  std::string resp;
  if(wifi.is_connected()){
      resp = "RoostaBoosta is currently connected to the internet!\r\n";
  }else{
      resp = "RoostaBoosta is currently disconnected from the internet.\r\nPress the \"Connect\" button to see available networks";
  }
  bt.write(resp.c_str(), resp.size());
}

void bt_print_cmd_unknown()
{
  std::string resp;
  resp = "Unknown Command\r\n";
  bt.write(resp.c_str(), resp.size());
}

void bt_network_connect()
{   
  std::string resp;
  char ap_list[256];
  int num;
  int cnt = 0;
  char c = '\0';
  resp = "Networks Available:\r\n";
  bt.write(resp.c_str(), resp.size());
  ThisThread::sleep_for(2s);
  wifi.scan(ap_list, 256);
  resp = ap_list;
  bt.write(resp.c_str(), resp.size());
  ThisThread::sleep_for(10s);
  resp = "\nPlease type in the SSID for the network you would like to connect to: \r\n";
  bt.write(resp.c_str(), resp.size());
  ThisThread::sleep_for(2s);
  char ssid[32] = {'\0'};
  bt_clear_buf();
  while(1){
    if(bt.readable())
    {
      num=bt.read(&c, 1);
      if(c == '\r')break;
      ssid[cnt] = c;
      cnt++;
    }
  }
  resp = "Please type the password for the network you would like to connect to: \r\n";
  bt.write(resp.c_str(), resp.size());
  char pwd[32] = {'\0'};
  cnt = 0;
  c = '\0';
  bt_clear_buf();
  while(1){
    if(bt.readable()){
      num=bt.read(&c, 1);
      if(num<1 || c == '\r')break;
      pwd[cnt] = c;
      cnt++;
    }
  }
  if(wifi.connect(ssid,pwd)){
      resp="Connected to network!\r\n";
  }else{
      resp="Failed to connect\r\n";
  }
  bt.write(resp.c_str(), resp.size());
}

void bt_set_alarm()
{
  std::string resp;
  resp = "Please type in your desired alarm time (HHMM) in 24hr time";
  bt.write(resp.c_str(), resp.size());
  bt_clear_buf();
  int num;
  Timer t;
  t.start();
  char hr[3];
  char min[3];
  int cnt = 0;
  char c = '\0';
  while(t.elapsed_time() < 10s){
    if(bt.readable())
    {
      num=bt.read(&c, 1);
      printf("%c", c);
      if(cnt < 2)hr[cnt]=c;
      else if(cnt>=2 && cnt<4)min[cnt-2]=c;
      if(num<1||c=='\r')break;
      //timestr[cnt] = c;
      cnt++;
    }
  }
  tm time = RTC::getDefaultTM();
  time.tm_min = atoi(min);
  time.tm_hour = atoi(hr);
  //printf("%s", timestr);
  char confirm[40];
  memset(confirm, '\0', 40);
  snprintf(confirm, 40, "Your alarm is confirmed for %d:%d\n", atoi(hr), atoi(min));
  bt.write(confirm, 40);
  RTC::alarm(&alarmFunction, time);
}

void bt_set_location()
{
  std::string resp;
  int num;
  resp = "Type in either a city name, US zip code, UK postcode, Canada postal code, IP address, or Latitude,Longitude (decimal degree)";
  bt.write(resp.c_str(), resp.size());
  bt_clear_buf();
  int cnt = 0;
  char c = '\0';
  while(1){
    if(bt.readable())
    {
      num=bt.read(&c, 1);
      if(num<1 || c=='\r')break;
      location[cnt] = c;
      cnt++;
    }
  }
  snprintf(payload, 128, "/v1/forecast.json?key=a9e3fb6a760c49699d625304232504&q=%s&aqi=no", location);
}

void bt_send_time(){
  std::string resp;
  resp = "Connected to internet and time set!\n";
  bt.write(resp.c_str(), resp.size());
  time_t seconds = time(NULL);
  char buffer[32] = {'\0'};
  strftime(buffer, 32, "%c\r\n", localtime(&seconds));
  bt.write("Current Time: ", 14);
  bt.write(buffer, 32);
}


void bt_api(){
  char c;
  int num;
  do
  {
    if(!bt.readable())ThisThread::yield();
      num = bt.read(&c, 1);
      if(num<1)ThisThread::yield();
      if(c == '\r' || c == '\n')continue;
      switch(c)
      {
        case 's':
          {
            bt_clear_buf();
            bt_print_connection_status();
          }
          break;
        case 'c':
          {
            bt_clear_buf();
            bt_network_connect();
          }
          break;
        case 'l':
          {
            bt_clear_buf();
            bt_set_location();
            if(wifi.is_connected())updatetime();
            bt_send_time();
          }
          break;
        case 'a':
          {
            bt_clear_buf();
            bt_set_alarm();
          }
          break;
        default:
          bt_clear_buf();
          bt_print_cmd_unknown();
        }
  }while(1);
}

} // namespace

// ====================== Global Definitions =========================\

Thread bt_thread;
Thread alarm_thread;

int
main()
{
  turnOff.fall(&pb_turnoff);
  dispWeather.fall(&pb_dispweather);
  // wifi
  bt_thread.start(bt_api);
  
  while(!wifi.is_connected()){
    ThisThread::sleep_for(1s);
  }

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
  weather_data  data_;
  weather_data* data = &data_;
  updateweather(data);
  while (true) {
    if(alarm_on){
      updateweather(data);
      alarm(data);
    }
    Display_Time(std::chrono::system_clock::from_time_t(time(NULL)));
    ThisThread::sleep_for(1s);
    if(disp_weather){
      updateweather(data);
      Display_Weather(data);
      ThisThread::sleep_for(10s);
      disp_weather=false;
    }
  }
}

