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

// #include <lwip/sockets.h> // <arpa/inet.h> replacement

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

/// \brief Read the return of a command.
///
/// \param fd file descriptor of serial port.
/// \param len the amount of bytes to skip, if -1 skip all available bytes.
/// \param buf buffer to read into, if nullptr read into a temporary buffer and
/// discard
/// \param timeout timeout to wait readable
/// \param delim the delimiter to stop reading at, if '\0' read until timeout
///
/// \return ErrorStatus with value of number of bytes cleared or error code.
rb::ErrorStatus
readCMD_(
  const int                 fd,
  const int                 len = -1,
  char*                     buf = nullptr,
  std::chrono::milliseconds timeout =
    std::chrono::milliseconds{RB_WIFI_CLIENT_INTERNAL_TIMEOUT},
  char delim = '\0')
{
  using namespace rb;

  ErrorStatus   ret{};
  int           poll_ret = 0;
  int           read_ret = 0;
  char          c;
  struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};

  if (buf && len < 0) {
    ret = ErrorStatus{MBED_ERROR_CODE_EINVAL, "Invalid specified length."};
    RB_ERROR_LOG(ret);
    return ret;
  }

  while ((ret.value < len || len < 0) &&
         (poll_ret = poll(&fds, 1, timeout.count())) > 0) {
    read_ret = read(fd, &c, 1);
    if (read_ret > 0) {
      ret.value += read_ret;
      if (buf != nullptr) {
        *buf++ = c;
      }
      if (c == delim)
        goto end;
    } else if (read_ret == 0) {
      ret = ErrorStatus{
        MBED_ERROR_CODE_ENOTRECOVERABLE,
        "Reached serial eof, this should never happen."};
      RB_ERROR_LOG(ret);
      goto end;
    } else if (read_ret < 0) {
      ret = ErrorStatus{errno, "Read Error", read_ret};
      RB_ERROR_LOG(ret);
      goto end;
    }
  }

  if (len >= 0 && ret.value < len && poll_ret == 0) {
    ret = ErrorStatus{MBED_ERROR_CODE_ETIMEDOUT, "Timeout"};
  } else if (poll_ret < 0) {
    ret = ErrorStatus{errno, "Poll Error", poll_ret};
    RB_ERROR_LOG(ret);
  }
end:
  return ret;
}

/// \brief Sets up the esp8266 to be a wifi client.
///
/// \param fd file descriptor of serial port.
/// \param baud baud rate of serial port.
///
/// \return ErrorStatus with value of number of bytes written or error code.
rb::ErrorStatus
resetESP_(int fd)
{
  using namespace rb;

  // clang-format off
  constexpr auto command =
R"===(
node.restart()
uart.setup(0, %d, 8, uart.PARITY_NONE, uart.STOPBITS_1, 0)
wifi.setmode(wifi.STATION)
wifi.eventmon.register(wifi.eventmon.STA_GOT_IP, function(T)
  print(T.IP)
end)
print('\0')
)===";
  // clang-format on

  ErrorStatus err;
  if ((err = sendCMD_(fd, command, RB_WIFI_CLIENT_BAUD_RATE))) {
    RB_ERROR(err);
  }
  if ((err = readCMD_(fd))) {
    RB_ERROR(err);
  }
  return err;
}

} // namespace

// ====================== Global Definitions =========================

namespace rb {

WifiClient::WifiClient(PinName tx, PinName rx) :
    HTTPClient{},
    serial_(tx, rx),
    fd_(bind_to_fd(&serial_)),
    ip_{0},
    req_sem_{kMaxRequests},
    request_{}
{
  debug("\r\n[WifiClient] Initializing...");
  serial_.set_baud(RB_WIFI_CLIENT_BAUD_RATE);
  serial_.set_format(8, mbed::BufferedSerial::None, 1);
  reset();
  debug("\r\n[WifiClient] Initializing... done.");
}

void
WifiClient::reset()
{
  debug("\r\n[WifiClient] reset...");
  ErrorStatus err;
  if ((err = resetESP_(fd_))) {
    RB_ERROR(err);
  }
  debug("\r\n[WifiClient] reset.... done.");
}

bool
WifiClient::is_connected()
{
  return ip_[0] != 0;
}

const char*
WifiClient::get_ip()
{
  return ip_.data();
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

  ErrorStatus          err;
  std::array<char, 32> ip_buf = {0};
  char*                end;

  err = sendCMD_(fd_, command, ssid, phrase);
  if (err) {
    RB_ERROR_LOG(err);
    return err;
  }

  err = readCMD_(
    fd_,
    ip_buf.size(),
    ip_buf.data(),
    std::chrono::milliseconds{RB_WIFI_CLIENT_CONNECTION_TIMEOUT});
  if (err) {
    RB_ERROR_LOG(err);
    return err;
  }

  auto verify_ip = [](std::array<char, 32>& buf) {
    char* end         = nullptr;
    int   part_count  = 0;
    int   digit_count = 0;
    for (auto c : buf) {
      if (part_count == 4 && std::isspace(c)) {
        end = &c;
        return end;
      }
      if (c == '.') {
        continue;
        digit_count = 0;
        if (++part_count > 4) {
          return end;
        }
      }
      if (!std::isdigit(c)) {
        return end;
      }
      if (++digit_count > 3) {
        return end;
      }
    }
    return end;
  };

  end = verify_ip(ip_buf);
  if (!end) {
    err = ErrorStatus{MBED_ERROR_CODE_EINVAL, "Invalid IP address"};
    RB_ERROR_LOG(err);
    return err;
  }
  *end = '\0';
  std::strcpy(ip_.data(), ip_buf.data());
  return err;
}

ErrorStatus
WifiClient::disconnect()
{
  // clang-format off
  constexpr auto command =
R"===(
wifi.sta.disconnect()
print('\0')
)===";
  // clang-format on

  ErrorStatus err;
  if ((err = sendCMD_(fd_, command))) {
    RB_ERROR_LOG(err);
    return err;
  }
  if ((err = readCMD_(fd_))) {
    RB_ERROR_LOG(err);
    return err;
  }
  ip_ = {0};
  return err;
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
  print('\0')
end
wifi.sta.getap(1, listap)
)===";
  // clang-format on

  ErrorStatus              err;
  std::vector<std::string> ssids;
  std::array<char, 32>     buf;

  ssids.reserve(num);
  if ((err = sendCMD_(fd_, command))) {
    RB_ERROR_LOG(err);
    return ssids;
  }

  while (num-- > 0) {
    err = readCMD_(
      fd_,
      buf.size(),
      buf.data(),
      std::chrono::milliseconds{RB_WIFI_CLIENT_CONNECTION_TIMEOUT},
      '\n');
    if (err) {
      RB_ERROR_LOG(err);
      return ssids;
    }

    if (err.value == 0) {
      continue;
    } else if (err.value > 0 && err.value < buf.size()) {
      buf[err.value] = '\0';
      ssids.emplace_back(buf.data());
    } else {
      err = ErrorStatus{
        MBED_ERROR_CODE_ASSERTION_FAILED, "err returned invalid value."};
      RB_ERROR(err);
    }
  }
  return ssids;
}

HTTPResponsePromise
WifiClient::Request(
  const HTTPRequest&        request,
  HTTPResponse&             response,
  std::chrono::milliseconds send_timeout,
  mbed::Callback<void()>    rcv_callback)
{
  static int req_id = 0;

  // clang-format off
  constexpr auto command =
R"===(
sk=net.createConnection(net.TCP, 0)
sk:on("receive", function(sck, c) print(c) print('\0') end )
sk:connect(80,"%s")
sk:send("%s", function(sent) print('\0') end)
)===";
  // clang-format on

  HTTPResponsePromise promise{response, *this};
  ErrorStatus         err;

  if (!req_sem_.try_acquire_for(send_timeout)) {
    err = ErrorStatus{
      MBED_ERROR_CODE_TIME_OUT, "Failed to acquire request semaphore."};
    PromiseGetError(promise) = err;
    RB_ERROR_LOG(err);
    return promise;
  }

  auto test_error = [&](ErrorStatus err) {
    if (err) {
      PromiseGetError(promise) = err;
      RB_ERROR_LOG(err);
      req_sem_.release();
      return true;
    }
    return false;
  };

  { // serialize request.
    mbed::Span buf{send_buffer_.data(), send_buffer_.size()};
    auto       handle        = request.get_serialization_handle();
    int        written_count = 0;
    while (handle.serialize(buf)) {
      written_count += handle.gcount();
      buf =
        mbed::Span{buf.data() + handle.gcount(), buf.size() - handle.gcount()};
    }
    if (test_error(handle.fail()))
      return promise;
    send_buffer_[written_count] = '\0';
  }

  if (test_error(sendCMD_(fd_, command, send_buffer_.data())))
    return promise;
  if (test_error(readCMD_(fd_, -1, nullptr, send_timeout, '\0')))
    return promise;

  // successfully sent -- can assign promise id.
  request_.rcv_callback = rcv_callback;
  auto callback         = [this]() {
    ErrorStatus err;
    if (request_.rcv_callback) {
      request_.rcv_callback();
    }
    // signal waiting thread.
    if (request_.flags.set(RB_EVENT_FLAG_NETWORK_PACKET) & osFlagsError) {
      err = ErrorStatus{
        MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT, "Failed to set event flag."};
      RB_ERROR(err);
    }
  };
  serial_.sigio(callback);
  PromiseGetReqID(promise) = request_.req_id = ++req_id;
  return promise;
}

void
WifiClient::drop(int req)
{
  // clang-format off
  constexpr auto command =
R"===(
sk:close()
print('\0')
)===";
  // clang-format on

  ErrorStatus err;

  if (request_.req_id != req) {
    err = ErrorStatus{MBED_ERROR_CODE_EINVAL, "Invalid request id."};
    RB_ERROR(err);
  }

  if ((err = sendCMD_(fd_, command))) {
    RB_ERROR(err);
  }
  if ((err = readCMD_(fd_))) {
    RB_ERROR(err);
  }

  request_.req_id = 0;
  if (request_.flags.clear() & osFlagsError) {
    err = ErrorStatus{
      MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT, "Failed to clear event flag."};
    RB_ERROR(err);
  }
  serial_.sigio(mbed::Callback<void()>{}); // disable callback.
  req_sem_.release();
}

// std::size_t
// WifiClient::available(int req)
// {
//   ErrorStatus err;
//   int         ret;

//   if (request_.req_id != req) {
//     err = ErrorStatus{MBED_ERROR_CODE_EINVAL, "Invalid request id."};
//     RB_ERROR(err);
//   }

//   struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
//   int           r;
//   if ((r = poll(&fds, 1, 0)) >= 0) {

//   } else {
//     err = ErrorStatus(errno, "Poll error", r);
//     RB_ERROR(err);
//   }
//   return ret;
// }

ErrorStatus
WifiClient::read(int req, mbed::Span<char> buf)
{
  ErrorStatus err;

  if (request_.req_id != req) {
    err = ErrorStatus{MBED_ERROR_CODE_EINVAL, "Invalid request id."};
    RB_ERROR_LOG(err);
    return err;
  }

  err =
    readCMD_(fd_, buf.size(), buf.data(), std::chrono::milliseconds{0}, '\0');
  if (err) {
    if (err.code() == MBED_ERROR_CODE_TIME_OUT) {
      auto val = err.value;
      err      = ErrorStatus{MBED_SUCCESS, "No data available.", val};
    } else {
      RB_ERROR_LOG(err);
    }
  }
  return err;
}

ErrorStatus
WifiClient::wait(int req, std::chrono::milliseconds timeout)
{
  ErrorStatus err;

  if (request_.req_id != req) {
    err = ErrorStatus{MBED_ERROR_CODE_EINVAL, "Invalid request id."};
    RB_ERROR_LOG(err);
    return err;
  }

  std::uint32_t os_ret = request_.flags.wait_any(
    RB_EVENT_FLAG_NETWORK_PACKET,
    timeout.count() > 0 ? timeout.count() : osWaitForever);
  if (os_ret & osFlagsError) {
    if (os_ret & osFlagsErrorTimeout) {
      err = ErrorStatus{
        MBED_ERROR_CODE_TIME_OUT, "Timeout with timeout = ", timeout.count()};
    } else {
      err = ErrorStatus{
        MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT,
        "Unknown flags error while waiting for response."};
      RB_ERROR_LOG(err);
    }
  }
  return err;
}

} // namespace rb
