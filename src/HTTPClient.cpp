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

#pragma region HTTPRequest

bool
HTTPRequest::valid() const
{
  // static_assert(false, "Not implemented");
  return true;
}

#pragma endregion HTTPRequest
#pragma region    HTTPResponse

#pragma endregion HTTPResponse
#pragma region    HTTPClient

ErrorStatus
HTTPClient::Request(
  const HTTPRequest&        request,
  HTTPResponse&             response,
  std::chrono::milliseconds timeout,
  mbed::Callback<void()>    rcv_callback)
{
  //   ErrorStatus   err;
  //   std::uint32_t os_ret;

  //   os_ret = response.event_flags_.set(RB_EVENT_FLAG_NETWORK_PACKET);
  //   if (os_ret & osFlagsError)
  //     return ErrorStatus{
  //       MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT, "Unable to clear flags",
  //       os_ret};

  //   if (!request)
  //     return ErrorStatus{MBED_ERROR_CODE_INVALID_FORMAT, "Invalid request"};

  //   auto input_callback = [this, rcv_callback, &response]() {
  //     // If there is a critical ISR rcv callback, call it.
  //     if (rcv_callback)
  //       rcv_callback();

  //     // Signal to calling thread that data is available.
  //     std::uint32_t os_ret =
  //     response.event_flags_.set(RB_EVENT_FLAG_NETWORK_PACKET); if (os_ret &
  //     osFlagsError) {
  //       ErrorStatus err{
  //         MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT, "Unable to set flags",
  //         os_ret};
  //       RB_ERROR(err);
  //     }
  //   };

  //   if ((err = registerInputCallback(input_callback)))
  //     goto err0;

  //   // Send request.
  //   if ((err = sendRequest(request)))
  //     goto err1;

  // err1:
  //   unregisterInputCallback();
  // err0:
  //   return ErrorStatus{};
  return ErrorStatus{};
}

#pragma endregion HTTPClient

} // namespace rb
