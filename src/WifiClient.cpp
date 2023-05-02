/// \file WifiClient.cpp
/// \date 2023-04-26
/// \author Raymond Barrett (rbarrett38@gatech.edu)
/// \author mshakula (matvey@gatech.edu)
///
/// \brief Wifi client for ESP8266.

#define MBED_NO_GLOBAL_USING_DIRECTIVE

#include "WifiClient.hpp"

#include <ctime>

// #include "Endpoint.h"
#include <algorithm>
#include <ratio>
#include <string>

#include <mbed.h>
#include <rtos.h>

#include "ErrorStatus.hpp"

using namespace std::chrono_literals;

// ======================= Local Definitions =========================

namespace {

/// \brief Sends formatted string over serial port
///
/// \param fd file descriptor of serial port.
/// \param timeout timeout to wait writeable
/// \param fmt string to print, if this string inclues format specifiers the
/// additional arguments following fmt are formatted and inserted in the
/// resulting string replacing their respective specifiers.
///
/// \return ErrorStatus with value of number of bytes written or error code.
rb::ErrorStatus
sendCMD_(int fd, const char* fmt, ...)
{
  using namespace rb;

  constexpr std::chrono::milliseconds kTimeout{RB_WIFI_CLIENT_INTERNAL_TIMEOUT};

  ErrorStatus ret{};
  int         r;
  va_list     args;
  va_start(args, fmt);

  struct pollfd fds = {.fd = fd, .events = POLLOUT, .revents = 0};
  if ((r = poll(&fds, 1, kTimeout.count())) > 0) {
    r = vdprintf(fd, fmt, args);
    if (r < 0) {
      ret = ErrorStatus{errno, "Write Error", r};
    } else {
      ret.value = r;
    }
  } else if (r == 0) {
    ret = ErrorStatus{MBED_ERROR_CODE_ETIMEDOUT, "Timeout"};
  } else {
    ret = ErrorStatus{errno, "Poll Error", r};
    RB_ERROR_LOG(ret);
  }

  va_end(args);
  return ret;
}

/// \brief Sets up the esp8266 to be a wifi client.
///
/// \param fd file descriptor of serial port.
/// \param baud baud rate of serial port.
///
/// \return ErrorStatus with value of number of bytes written or error code.
rb::ErrorStatus
espSetup_(int fd)
{
  // clang-format off
  constexpr auto command =
R"===(
node.restart()
uart.setup(0, %d, 8, uart.PARITY_NONE, uart.STOPBITS_1, 0)
wifi.setmode(wifi.STATION)
wifi.eventmon.register(wifi.eventmon.STA_GOT_IP, function(T)
  print(T.IP)
end)
)===";
  // clang-format on

  return sendCMD_(fd, command, RB_WIFI_CLIENT_BAUD_RATE);
}

/// \brief Clear the file's buffer.
///
/// \param fd file descriptor of serial port.
/// \param disard_echo number of bytes to clear.
///
/// \return ErrorStatus with value of number of bytes cleared or error code.
rb::ErrorStatus
clearBuffer_(int fd, int len = -1)
{
  using namespace rb;

  constexpr std::chrono::milliseconds kTimeout{RB_WIFI_CLIENT_INTERNAL_TIMEOUT};

  ErrorStatus ret{};
  int         r;
  char        c;

  auto continue_read = [&]() {
    return (ret.value < len || len < 0) && !(c == '\r' || c == '>');
  };

  struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
  while ((ret.value < len || len < 0) &&
         (r = poll(&fds, 1, kTimeout.count())) > 0) {
    if ((r = read(fd, read_buf.data(), read_buf.size())) > 0) {
      ret.value += r;
    } else if (r == 0) {
      ret = ErrorStatus{
        MBED_ERROR_CODE_ENOTRECOVERABLE,
        "Reached serial eof, this should never happen."};
      RB_ERROR_LOG(ret);
      goto end;
    } else if (r == -1) {
      ret = rb::ErrorStatus{errno, "Read Error", r};
      RB_ERROR_LOG(ret);
      goto end;
    }
  }

  if (len >= 0 && ret.value < len && r == 0) {
    ret = rb::ErrorStatus{MBED_ERROR_CODE_ETIMEDOUT, "Timeout"};
  } else if (r < 0) {
    ret = rb::ErrorStatus{errno, "Poll Error", r};
    RB_ERROR_LOG(ret);
  }
end:
  return ret;
}

} // namespace

// ====================== Global Definitions =========================

namespace rb {

WifiClient::WifiClient(PinName tx, PinName rx) :
    HTTPClient{},
    serial_(tx, rx),
    fd_(bind_to_fd(&serial_))
{
  debug("\r\n[WifiClient] Initializing...");
  serial_.set_baud(RB_WIFI_CLIENT_BAUD_RATE);
  serial_.set_format(8, SerialBase::None, 1);
  reset();
  debug("\r\n[WifiClient] Initializing... done.");
}

void
WifiClient::reset()
{
  debug("\r\n[WifiClient] reset...");
  ErrorStatus err;
  if ((err = espSetup_(fd_))) {
    RB_ERROR(err);
  }
  if ((err = clearBuffer_(fd_))) {
    RB_ERROR(err);
  }
  debug("\r\n[WifiClient] reset.... done.");
}

bool
WifiClient::is_connected()
{
  return ip_ != INADDR_ANY;
}

struct in_addr
WifiClient::get_ip()
{
  return ip_;
}

ErrorStatus
WifiClient::connect(const char* ssid, const char* phrase)
{
  // clang-format off
  constexpr auto command =
R"===(
wifi.sta.config({ssid = "%s", pwd = "%s", auto = false, save = false})
wifi.sta.connect()
)===";
  // clang-format on

  ErrorStatus err;
  if ((err = sendCMD_(fd_, command, ssid.data(), phrase.data()))) {
    RB_ERROR_LOG(err);
    return err;
  }
  // wait for ip to return.
}

ErrorStatus
WifiClient::disconnect()
{
  // clang-format off
  constexpr auto command =
R"===(
wifi.sta.disconnect()
)===";
  // clang-format on

  ErrorStatus err;
  if ((err = sendCMD_(fd_, command))) {
    RB_ERROR_LOG(err);
    return err;
  }

  sendCMD_(fd_, "wifi.sta.disconnect()\r\n");
  clearBuffer_();
  mbed::Timer timer;
  timer.start();
  // make sure that wifi station has ip nil
  while (timer.elapsed_time() < _timeout) {
    memset(_ip, '\0', sizeof(_ip));
    sendCMD_(fd_, "print(wifi.sta.getip())\r\n");
    getreply(_ip, 3);
    // printf("%s\n", _ip); //DEBUG ONLY
    if (strcmp(_ip, "nil") == 0) {
      timer.stop();
      timer.reset();
      return 1;
    }
  }
  sendCMD_(fd_, "print(wifi.sta.getip())\r\n");
  getreply(_ip, 16);
  timer.stop();
  timer.reset();
  return 0;
}

std::vector<std::string>
WifiClient::scan(std::size_t num)
{
  // clang-format off
  constexpr auto command = 
R"===(
function listap(t)
  for bssid,v in pairs(t) do
    local ssid, rssi, authmode, channel = string.match(v, "([^,]+),([^,]+),([^,]+),([^,]*)")
    print(string.format("%32s\n",ssid))
  end
end
wifi.sta.getap(1, listap)
)===";
  // clang-format on

  ErrorStatus              err;
  std::vector<std::string> ssids;

  ssids.reserve(num);
  if ((err = sendCMD_(fd_, command))) {
    RB_ERROR_LOG(err);
    return ssids;
  }
  // GET REPLY.
}

int
WifiClient::http_get_request(
  const char* address,
  const char* payload,
  const char* header,
  char*       respBuffer,
  size_t      respBufferSize)
{
  sendCMD_(fd_, "sk=net.createConnection(net.TCP, 0)\r\n");
  clearBuffer_();
  sendCMD_(fd_, "sk:on(\"receive\", function(sck, c) print(c) end )\r\n");
  clearBuffer_();
  sendCMD_(fd_, "sk:connect(80,\"%s\")\r\n", address);
  clearBuffer_();
  sendCMD_(
    fd_,
    "sk:send(\"GET %s HTTP/1.1\\r\\nHost: %s\\r\\n%s\\r\\n\\r\\n\")\r\n",
    payload,
    address,
    header);
  getreply_json(respBuffer, respBufferSize);
  return 1;
}

} // namespace rb
