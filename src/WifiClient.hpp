/// \file WifiClient.hpp
/// \date 2023-04-26
/// \author Raymond Barrett (rbarrett38@gatech.edu)
/// \author mshakula (matvey@gatech.edu)
///
/// \brief WifI client for ESP8266.
///
/// Module Datasheet: http://www.electrodragon.com/w/Wi07c.

#ifndef RB_WIFICLIENT_HPP
#define RB_WIFICLIENT_HPP

#ifndef __cplusplus
#error "WifiClient.hpp is a cxx-only header."
#endif // __cplusplus

#include <string>
#include <string_view>
#include <vector>

#include <PinNames.h>
#include <drivers/BufferedSerial.h>

#include <rtos/EventFlags.h>
#include <rtos/Semaphore.h>

#include "HTTPClient.hpp"

// ======================= Public Interface ==========================

namespace rb {

/// \brief A implementation of the HTTPClient interface for the ESP8266 serial
/// lua module.
class WifiClient : public HTTPClient
{
  constexpr static std::size_t kMaxRequests = 1;

 public:
  /// \brief Constructor
  ///
  /// \param tx mbed pin to use for tx line of Serial interface
  /// \param rx mbed pin to use for rx line of Serial interface
  /// \param reset reset pin of the wifi module ()
  /// \param baud the baudrate of the serial connection
  WifiClient(PinName tx, PinName r);

  /// \brief Reset the wifi module
  void reset();

  /// \brief Connect the wifi module to the specified ssid.
  ///
  /// \param ssid ssid of the network
  /// \param phrase WEP, WPA or WPA2 key
  ///
  /// \return Error status with error that occurred, if any.
  ErrorStatus connect(const char* ssid, const char* phrase);

  /// \brief Disconnect the ESP8266 module from the access point
  ///
  /// \return true if successful
  ErrorStatus disconnect();

  /// \brief Check connection to the access point
  bool is_connected();

  /// \brief Get the IP address of the ESP8266 module
  const char* get_ip();

  /// \brief Scan all access points and return them.
  ///
  /// \param num number of access points to scan.
  ///
  /// \return vector of access points. if the vector is empty, then an error has
  /// occurred.
  std::vector<std::string> scan(std::size_t num);

  /// \see HTTPClient::Request
  virtual HTTPResponsePromise Request(
    const HTTPRequest&        request,
    HTTPResponse&             response,
    std::chrono::milliseconds send_timeout,
    mbed::Callback<void()>    rcv_callback) override;

 private:
  /// \see HTTPClient::drop
  virtual void drop(int req) override;

  /// \see HTTPClient::read
  virtual ErrorStatus read(int req, mbed::Span<char>) override;

  /// \see HTTPClient::wait
  virtual ErrorStatus wait(int req, std::chrono::milliseconds timeout) override;

 private:
  mbed::BufferedSerial serial_;
  int                  fd_;
  std::array<char, 32> ip_;

  struct ActiveRequest
  {
    int                    req_id;
    mbed::Callback<void()> rcv_callback;
    rtos::EventFlags       flags;
  };

  rtos::Semaphore req_sem_; // guard the active request.
  ActiveRequest   request_; // the active request.

  std::array<char, 512> send_buffer_;
};

} // namespace rb

// ===================== Detail Implementation =======================

#endif
