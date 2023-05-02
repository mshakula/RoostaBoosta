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

#define MBED_NO_GLOBAL_USING_DIRECTIVE

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
    err_{},
    client_{&client},
    req_id_{0}
{
}

HTTPResponsePromise::HTTPResponsePromise(HTTPResponsePromise&& other) :
    obj_{other.obj_},
    err_{other.err_},
    client_{other.client_},
    req_id_{other.req_id_}
{
  other.req_id_ = 0;
}

HTTPResponsePromise&
HTTPResponsePromise::operator=(HTTPResponsePromise&& other)
{
  if (this != &other) {
    obj_          = other.obj_;
    err_          = other.err_;
    client_       = other.client_;
    req_id_       = other.req_id_;
    other.req_id_ = 0;
  }
  return *this;
}

HTTPResponsePromise::~HTTPResponsePromise()
{
  drop();
  req_id_ = 0;
}

HTTPResponsePromise&
HTTPResponsePromise::wait(std::chrono::milliseconds timeout)
{
  if (req_id_) {
    err_ = client_->wait(req_id_, timeout);
  }
  return *this;
}

void
HTTPResponsePromise::drop()
{
  if (req_id_) {
    client_->drop(req_id_);
  }
}

HTTPResponsePromise&
HTTPResponsePromise::read(mbed::Span<char> buffer)
{
  if (req_id_) {
    err_ = client_->read(req_id_, buffer);
  } else {
    err_ = ErrorStatus{
      MBED_ERROR_CODE_INVALID_ARGUMENT,
      "Attempted to read from a null HTTPResponsePromise."};
  }
  return *this;
}

#pragma endregion HTTPRequestResponse
#pragma region    HTTPClient

#pragma endregion HTTPClient

} // namespace rb
