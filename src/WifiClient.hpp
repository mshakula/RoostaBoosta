/// \file WifiClient.hpp
/// \date 2023-04-26
/// \author Raymond Barrett (rbarrett38@gatech.edu)
///
/// \brief WifI client for ESP8266.
///
/// \details Datasheet: http://www.electrodragon.com/w/Wi07c.

#ifndef RB_WIFICLIENT_HPP
#define RB_WIFICLIENT_HPP

#ifndef __cplusplus
#error "WifiClient.hpp is a cxx-only header."
#endif // __cplusplus

#include <mbed.h>

// ======================= Public Interface ==========================

namespace rb {

class WifiClient
{
  struct Handle
  {
    FILE*                 file;
    mbed::BufferedSerial* serial;
  };

 public:
  /// \brief Constructor
  ///
  /// \param tx mbed pin to use for tx line of Serial interface
  /// \param rx mbed pin to use for rx line of Serial interface
  /// \param reset reset pin of the wifi module ()
  /// \param baud the baudrate of the serial connection
  /// \param timeout the timeout of the serial connection
  WifiClient(
    PinName                   tx,
    PinName                   rx,
    PinName                   reset,
    int                       baud    = 9600,
    std::chrono::microseconds timeout = 5s);

  /// \brief Initialize the wifi hardware
  ///
  /// \return true if successful
  bool init();

  /// \brief Connect the wifi module to the specified ssid.
  ///
  /// \param ssid ssid of the network
  /// \param phrase WEP, WPA or WPA2 key
  ///
  /// \return true if successful
  bool connect(const char* ssid, const char* phrase);

  /// \brief Check connection to the access point
  ///
  /// \return true if successful
  bool is_connected();

  /// \brief Copy current IP address to buffer
  /// \param ip buffer to write IP to
  void get_ip(char* ip);

  /// \brief Disconnect the ESP8266 module from the access point
  ///
  /// \return true if successful
  bool disconnect();

  /// \brief Scan all access points and put them in aplist (limited by size
  /// param)
  ///
  /// \return true if successful
  int scan(char* aplist, int size);

  /// \brief Send a request.
  int http_get_request(
    const char* address,
    char* payload,
    const char* header,
    char*       respBuffer,
    size_t      respBufferSize);

  /// \brief Reset the wifi module
  bool reset();

  /// \brief Obtains the current instance of the ESP8266
  static WifiClient* getInstance() { return _inst; };

 private:
  /// \brief Discards echoed characters
  ///
  /// \return true if successful
  bool discardEcho();

  /// \brief Sends formatted string over serial port
  ///
  /// \param handle struct with buffer to check writeable and filestream to
  /// write to
  /// \param timeout timeout to wait writeable
  /// \param fmt string to print, if this string inclues format specifiers the
  /// additional arguments following fmt are formatted and inserted in the
  /// resulting string replacing their respective specifiers.
  int printCMD(
    Handle*                   handle,
    std::chrono::microseconds timeout,
    const char*               fmt,
    ...);

  /// \brief Flushes BufferedSerial Buffer
  void flushBuffer(int len = -1);

  /// \brief Gets reply from ESP8662
  /// \param resp optional buffer to store response
  /// \return 1 if successful
  int getreply(char* resp = 0, int size = 0);

  /// \brief Get reply from ESP8266 in json format.
  int getreply_json(char* resp, int size);

 protected:
  mbed::BufferedSerial _serial;
  mbed::DigitalOut     _reset_pin;

  Handle _handle;

  static WifiClient* _inst;

  // TODO WISHLIST: ipv6?
  // this requires nodemcu support
  char                      _ip[16] = "nil";
  int                       _baud;
  std::chrono::microseconds _timeout;
};

} // namespace rb

// ===================== Detail Implementation =======================

#endif
