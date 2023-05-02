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

#include <cmsis_os2.h>

// ======================= Local Definitions =========================

namespace {

} // namespace

// ====================== Global Definitions =========================

namespace rb {

#pragma region HTTPRequestResponse

/// \brief Construct a new HTTPRequestPromise.
HTTPResponsePromise::HTTPResponsePromise(
  HTTPResponse& obj,
  HTTPClient&   client) :
    obj_{&obj},
    client_{&client},
    err_{}
{
  osEventFlagsAttr_t attr = {
    .name      = "HTTPResponseEventFlag",
    .attr_bits = 0,
    .cb_mem    = &flag_mem_,
    .cb_size   = sizeof(flag_mem_)};
  flag_id_ = osEventFlagsNew(&attr);
}

HTTPResponsePromise::HTTPResponsePromise(HTTPResponsePromise&& other) :
    obj_{other.obj_},
    client_{other.client_},
    err_{other.err_},
    flag_id_{other.flag_id_}
{
  std::memcpy(&flag_mem_, &other.flag_mem_, sizeof(flag_mem_));
  other.flag_id_ = nullptr;
}

HTTPResponsePromise&
HTTPResponsePromise::operator=(HTTPResponsePromise&& other)
{
  if (this != &other) {
    obj_     = other.obj_;
    client_  = other.client_;
    err_     = other.err_;
    flag_id_ = other.flag_id_;
    std::memcpy(&flag_mem_, &other.flag_mem_, sizeof(flag_mem_));
    other.flag_id_ = nullptr;
  }
  return *this;
}

HTTPResponsePromise::~HTTPResponsePromise()
{
  if (flag_id_ != nullptr) {
    if (osEventFlagsDelete(flag_id_) != osOK) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_FAILED_OPERATION, "Failed to delete event flag."};
      RB_ERROR(err_); // fatal error in dtor -- this is a bug.
    }
  }
}

HTTPResponsePromise&
HTTPResponsePromise::wait(std::chrono::milliseconds timeout)
{
  std::uint32_t os_ret = osEventFlagsWait(
    flag_id_,
    RB_EVENT_FLAG_NETWORK_PACKET,
    osFlagsWaitAny,
    timeout.count() > 0 ? timeout.count() : osWaitForever);
  if (os_ret & osFlagsError) {
    if (os_ret & osFlagsErrorTimeout) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ETIMEDOUT,
        "Request timed out with timeout = ",
        timeout.count()};
    } else {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_RTOS_EVENT_FLAGS_EVENT,
        "Unknown flags error while waiting for response."};
    };
  }
  return *this;
}

void
HTTPResponsePromise::drop()
{
  client_->client_->unregisterInputCallback();
  client_->drop();
}

std::size_t
HTTPResponsePromise::available() const
{
  return client_->available(req_id_);
}

HTTPResponsePromise&
HTTPResponsePromise::read(mbed::Span<char> buffer)
{
  err_ = client_->read(req_id_, buffer);
  return *this;
}

#pragma endregion HTTPRequestResponse
#pragma region    HTTPClient

#pragma endregion HTTPClient

} // namespace rb
