/// \file HTTPClient.cpp
/// \date 2023-04-23
/// \author mshakula (matvey@gatech.edu)
///
/// \brief A minimalistic HTTP client interface implementing
/// [HTTP/1.1](https://tools.ietf.org/html/rfc2616) with a streaming interface.
///
/// A large chunk of this specific streamable implementation is only possible
/// thanks to the open source code of
/// [nanoprintf](https://github.com/charlesnicholson/nanoprintf).

#include "HTTPClient.hpp"

// ======================= Local Definitions =========================

namespace {

} // namespace

// ====================== Global Definitions =========================

namespace rb {

#pragma region HTTPRequestHeader

bool
HTTPRequestHeader::eof() const
{
  static_assert(false, "Not implemented");
}

ErrorStatus
HTTPRequestHeader::fail() const
{
  static_assert(false, "Not implemented");
}

void
HTTPRequestHeader::reset() const
{
  static_assert(false, "Not implemented");
}

HTTPRequestHeader&
HTTPRequestHeader::read(mbed::Span<char> buffer)
{
  static_assert(false, "Not implemented");
}

#pragma endregion HTTPRequestHeader
#pragma region    HTTPResponseHeader

bool
HTTPResponseHeader::eof() const
{
  static_assert(false, "Not implemented");
}

ErrorStatus
HTTPResponseHeader::fail() const
{
  static_assert(false, "Not implemented");
}

void
HTTPResponseHeader::reset() const
{
  static_assert(false, "Not implemented");
}

HTTPResponseHeader&
HTTPResponseHeader::read(mbed::Span<char> buffer)
{
  static_assert(false, "Not implemented");
}

#pragma endregion HTTPResponseHeader
#pragma region    HTTPGeneralHeader

bool
HTTPGeneralHeader::eof() const
{
  static_assert(false, "Not implemented");
}

ErrorStatus
HTTPGeneralHeader::fail() const
{
  static_assert(false, "Not implemented");
}

void
HTTPGeneralHeader::reset() const
{
  static_assert(false, "Not implemented");
}

HTTPGeneralHeader&
HTTPGeneralHeader::read(mbed::Span<char> buffer)
{
  static_assert(false, "Not implemented");
}

#pragma endregion HTTPGeneralHeader
#pragma region    HTTPEntityHeader

bool
HTTPEntityHeader::eof() const
{
  static_assert(false, "Not implemented");
}

ErrorStatus
HTTPEntityHeader::fail() const
{
  static_assert(false, "Not implemented");
}

void
HTTPEntityHeader::reset() const
{
  static_assert(false, "Not implemented");
}

HTTPEntityHeader&
HTTPEntityHeader::read(mbed::Span<char> buffer)
{
  static_assert(false, "Not implemented");
}

#pragma endregion HTTPEntityHeader
#pragma region    HTTPRequest

bool
HTTPRequest::valid() const
{
  static_assert(false, "Not implemented");
}

bool
HTTPRequest::eof() const
{
  static_assert(false, "Not implemented");
}

ErrorStatus
HTTPRequest::fail() const
{
  static_assert(false, "Not implemented");
}

void
HTTPRequest::reset() const
{
  static_assert(false, "Not implemented");
}

HTTPRequest&
HTTPRequest::read(mbed::Span<char> buffer)
{
  static_assert(false, "Not implemented");
}

#pragma endregion HTTPRequest
#pragma region    HTTPResponse

#pragma endregion HTTPResponse
#pragma region    HTTPClient

ErrorStatus
HTTPClient::Request(
  const HTTPRequest&                         request,
  HTTPResponse&                              response,
  std::chrono::milliseconds                  timeout,
  mbed::Callback<ErrorStatus(HTTPResponse&)> data_callback,
  mbed::Callback<ErrorStatus(HTTPResponse&)> header_callback,
  mbed::Callback<void()>                     rcv_callback)
{
  ErrorStatus   err;
  std::uint32_t os_ret = 0;

  if ((os_ret = event_flags_.clear(EVENT_FLAG_NETWORK_PACKET)) & osFlagsError)
    return ErrorStatus{
      MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT, "Unable to clear flags", os_ret};

  if (!request)
    return ErrorStatus{MBED_ERROR_CODE_INVALID_FORMAT, "Invalid request"};

  // Capturing by reference is okay, since the callback is only called
  // within the scope of this function or in an ISR during its lifetime.
  auto input_callback = [this, &rcv_callback]() {
    // If there is a critical ISR rcv callback, call it.
    if (rcv_callback)
      rcv_callback();

    // Signal to calling thread that data is available.
    std::uint32_t os_ret;
    if ((os_ret = event_flags_.set(EVENT_FLAG_NETWORK_PACKET)) & osFlagsError) {
      ErrorStatus err{
        MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT, "Unable to set flags", os_ret};
      RB_ERROR(err);
    }
  };

  if ((err = registerInputCallback(input_callback)))
    goto err0;

  // Send request.
  if ((err = sendRequest(request)))
    goto err1;

  // Wait for data to be available... do processing in this thread.
  {
    bool read_status = false;
    do {
      os_ret = event_flags_.wait_any(
        EVENT_FLAG_NETWORK_PACKET,
        timeout.count() ? timeout.count() : osWaitForever);
      if (os_ret & osFlagsError) {
        if (os_ret == osErrorTimeout) {
          err = {
            MBED_ERROR_CODE_TIME_OUT, "Timeout has occurred waiting for data."};
          goto err1;
        } else {
          err = {
            MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT,
            "Unable to wait for flags",
            os_ret};
          RB_ERROR(err);
        }
      }
    } while (???);
  }

err2:
  abort();
err1:
  unregisterInputCallback();
err0:
  return ErrorStatus{};
}

#pragma endregion HTTPClient

} // namespace rb
