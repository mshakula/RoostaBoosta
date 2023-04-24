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

HTTPError
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

HTTPError
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

HTTPError
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

HTTPError
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

HTTPError
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

HTTPError
HTTPClient::Request(
  const HTTPRequest&                          request,
  HTTPResponse&                               response,
  std::chrono::microseconds                   timeout,
  mbed::Callback<HTTPError(mbed::Span<char>)> data_callback)
{
  HTTPError err = HTTPError::None;

  if (!request)
    return HTTPError::InvalidRequest;

  if (err = sendRequest(request))
    return err;

  if (err = registerDataCallback(data_callback))
    return err;

  if (err = waitForResponse(timeout))
    return err;

  return err;
}

#pragma endregion HTTPClient

} // namespace rb
