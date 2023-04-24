/// \file HTTPClient.hpp
/// \date 2023-04-19
/// \author mshakula (matvey@gatech.edu)
///
/// \brief A minimalistic HTTP client interface implementing
/// [HTTP/1.1](https://tools.ietf.org/html/rfc2616).

#ifndef RB_HTTP_CLIENT_HPP
#define RB_HTTP_CLIENT_HPP

#ifndef __cplusplus
#error "HTTPClient.hpp is a cxx-only header."
#endif // __cplusplus

#include <cstddef>
#include <cstdint>

#include <chrono>
#include <span>
#include <string_view>

#include <platform/Callback.h>
#include <platform/Span.h>
#include <rtos/EventFlags.h>

// ======================= Public Interface ==========================

namespace rb {

/// \brief Enum representing an error from the HTTP client.
enum class HTTPError
{
  None = 0,
  InvalidRequest,
  Timeout
};

/// \brief Struct representing an HTTP status code. See [Status
/// Code](https://tools.ietf.org/html/rfc2616#section-6.1.1)
class HTTPStatusCode
{
  static constexpr std::string_view kDefaultReasonPhrase = "Unknown";

 public:
  /// \brief Construct an HTTPStatusCode from a code and reason phrase.
  constexpr HTTPStatusCode(
    int              code          = INVALID_,
    std::string_view reason_phrase = kDefaultReasonPhrase);

  constexpr HTTPStatusCode(const HTTPStatusCode&)            = default;
  constexpr HTTPStatusCode& operator=(const HTTPStatusCode&) = default;

  // Type queries.
  constexpr bool standard() const;
  constexpr bool informational() const;
  constexpr bool success() const;
  constexpr bool redirection() const;
  constexpr bool client_error() const;
  constexpr bool server_error() const;

  constexpr bool valid() const { return code_ != INVALID_; }
  constexpr      operator bool() const { return valid(); }

  /// \brief Return the reason string for this request.
  constexpr std::string_view reason() const;

  /// \brief Anonymous enum containing all standard HTTP status codes.
  enum
  {
    INVALID_ = 0,

    // 1xx Informational
    CONTINUE            = 100,
    SWITCHING_PROTOCOLS = 101,

    // 2xx Success
    OK                            = 200,
    CREATED                       = 201,
    ACCEPTED                      = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT                    = 204,
    RESET_CONTENT                 = 205,
    PARTIAL_CONTENT               = 206,

    // 3xx Redirection
    MULTIPLE_CHOICES   = 300,
    MOVED_PERMANENTLY  = 301,
    FOUND              = 302,
    SEE_OTHER          = 303,
    NOT_MODIFIED       = 304,
    USE_PROXY          = 305,
    TEMPORARY_REDIRECT = 307,

    // 4xx Client Error
    BAD_REQUEST                     = 400,
    UNAUTHORIZED                    = 401,
    PAYMENT_REQUIRED                = 402,
    FORBIDDEN                       = 403,
    NOT_FOUND                       = 404,
    METHOD_NOT_ALLOWED              = 405,
    NOT_ACCEPTABLE                  = 406,
    PROXY_AUTHENTICATION_REQUIRED   = 407,
    REQUEST_TIMEOUT                 = 408,
    CONFLICT                        = 409,
    GONE                            = 410,
    LENGTH_REQUIRED                 = 411,
    PRECONDITION_FAILED             = 412,
    REQUEST_ENTITY_TOO_LARGE        = 413,
    REQUEST_URI_TOO_LARGE           = 414,
    UNSUPPORTED_MEDIA_TYPE          = 415,
    REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED              = 417,

    // 5xx Server Error
    INTERNAL_SERVER_ERROR      = 500,
    NOT_IMPLEMENTED            = 501,
    BAD_GATEWAY                = 502,
    SERVICE_UNAVAILABLE        = 503,
    GATEWAY_TIMEOUT            = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
  };

 private:
  int              code_;
  std::string_view reason_phrase_;
};

/// \brief A structure containing an HTTP request method, detailed in
/// [Method](https://tools.ietf.org/html/rfc2616#section-5.1.1)
class HTTPMethod
{
 public:
  /// \brief Construct an HTTPMethod from a case-insensitive method string.
  constexpr HTTPMethod(std::string_view method = "");

  constexpr HTTPMethod(const HTTPMethod&)            = default;
  constexpr HTTPMethod& operator=(const HTTPMethod&) = default;

  /// \brief Return the method string for this request.
  constexpr std::string_view method() const;

  /// \brief Check if method is property initialized.
  constexpr bool valid() const { return code_ != INVALID_; }
  constexpr      operator bool() const { return valid(); }

 private:
  enum
  {
    INVALID_,

    /// \brief From mdn web docs: "The CONNECT method establishes a tunnel to
    /// the server identified by the target resource."
    CONNECT,

    /// \brief From mdn web docs: "The DELETE method deletes the specified
    /// resource."
    DELETE,

    /// \brief From mdn web docs: "The GET method requests a representation of
    /// the specified resource. Requests using GET should only retrieve data."
    GET,

    /// \brief From mdn web docs: "The HEAD method asks for a response
    /// identical to a GET request, but without the response body."
    HEAD,

    /// \brief From mdn web docs: "The OPTIONS method describes the
    /// communication options for the target resource."
    OPTIONS,

    /// \brief From mdn web docs: "The POST method is used to submit an entity
    /// to the specified resource, often causing a change in state or side
    /// effects on the server."
    POST,

    /// \brief From mdn web docs: "The PUT method replaces all current
    /// representations of the target resource with the request payload."
    PUT,

    /// \brief From mdn web docs: "The TRACE method performs a message
    /// loop-back test along the path to the target resource."
    TRACE,

    /// \brief Some other method not listed above.
    EXTENSION_METHOD
  } code_;
  std::string_view method_;
};

/// \brief A structure containing an HTTP
/// [request-header](https://tools.ietf.org/html/rfc2616#section-5.3),
struct HTTPRequestHeader
{
 public:
  std::string_view accept;
  std::string_view accept_charset;
  std::string_view accept_encoding;
  std::string_view accept_language;
  std::string_view authorization;
  std::string_view expect;
  std::string_view from;
  std::string_view host;
  std::string_view if_match;
  std::string_view if_modified_since;
  std::string_view if_none_match;
  std::string_view if_range;
  std::string_view if_unmodified_since;
  std::string_view max_forwards;
  std::string_view proxy_authorization;
  std::string_view range;
  std::string_view referer;
  std::string_view te;
  std::string_view user_agent;

  /// \brief Return a handle to the matching standard header field, or nullptr
  /// if no match is found.
  ///
  /// \param tag The header field to match.
  constexpr std::string_view* getField(std::string_view tag);

  /// \brief return if the request has been fully read.
  bool eof() const;

  /// \brief Return if the request has failed to be read / the error code.
  HTTPError fail() const;

  /// \brief Reset the output stream to the beginning. Clears any error / eof
  /// flags.
  void reset() const;

  /// \brief Read the request into a buffer.
  ///
  /// Can be called multiple times to write the request in chunks without.
  /// Characters are extracted and stored until any of the following conditions
  /// occurs:
  /// - buffer.size() characters were extracted and stored.
  /// - The end of the request is reached, in which case eof() is set to true.
  /// - An error occurs, in which case fail() is set, and can be read.
  ///
  /// \param buffer The buffer to write to.
  ///
  /// \return The current request.
  HTTPRequestHeader& read(mbed::Span<char> buffer);

  /// \brief Read the request into a buffer.
  ///
  /// \see read(mbed::Span<char>).
  HTTPRequestHeader& read(char* buffer, std::size_t count)
  {
    return this->read(mbed::Span(buffer, count));
  }

 private:
};

/// \brief A structure containing an HTTP
/// [response-header](https://tools.ietf.org/html/rfc2616#section-6.2),
struct HTTPResponseHeader
{
 public:
  std::string_view accept_ranges;
  std::string_view age;
  std::string_view etag;
  std::string_view location;
  std::string_view proxy_authenticate;
  std::string_view retry_after;
  std::string_view server;
  std::string_view vary;
  std::string_view www_authenticate;

  /// \brief Return a handle to the matching standard header field, or nullptr
  /// if no match is found.
  ///
  /// \param tag The header field to match.
  constexpr std::string_view* getField(std::string_view tag);

  /// \brief return if the request has been fully read.
  bool eof() const;

  /// \brief Return if the request has failed to be read / the error code.
  HTTPError fail() const;

  /// \brief Reset the output stream to the beginning. Clears any error / eof
  /// flags.
  void reset() const;

  /// \brief Read the request into a buffer.
  ///
  /// Can be called multiple times to write the request in chunks without.
  /// Characters are extracted and stored until any of the following conditions
  /// occurs:
  /// - buffer.size() characters were extracted and stored.
  /// - The end of the request is reached, in which case eof() is set to true.
  /// - An error occurs, in which case fail() is set, and can be read.
  ///
  /// \param buffer The buffer to write to.
  ///
  /// \return The current request.
  HTTPResponseHeader& read(mbed::Span<char> buffer);

  /// \brief Read the request into a buffer.
  ///
  /// \see read(mbed::Span<char>).
  HTTPResponseHeader& read(char* buffer, std::size_t count)
  {
    return this->read(mbed::Span(buffer, count));
  }

 private:
};

/// \brief A structure containing an HTTP
/// [general-header](https://tools.ietf.org/html/rfc2616#section-4.5).
struct HTTPGeneralHeader
{
 public:
  std::string_view cache_control;
  std::string_view connection;
  std::string_view date;
  std::string_view pragma;
  std::string_view trailer;
  std::string_view transfer_encoding;
  std::string_view upgrade;
  std::string_view via;
  std::string_view warning;

  /// \brief Return a handle to the matching standard header field, or nullptr
  /// if no match is found.
  ///
  /// \param tag The header field to match.
  constexpr std::string_view* getField(std::string_view tag);

  /// \brief return if the request has been fully read.
  bool eof() const;

  /// \brief Return if the request has failed to be read / the error code.
  HTTPError fail() const;

  /// \brief Reset the output stream to the beginning. Clears any error / eof
  /// flags.
  void reset() const;

  /// \brief Read the request into a buffer.
  ///
  /// Can be called multiple times to write the request in chunks without.
  /// Characters are extracted and stored until any of the following conditions
  /// occurs:
  /// - buffer.size() characters were extracted and stored.
  /// - The end of the request is reached, in which case eof() is set to true.
  /// - An error occurs, in which case fail() is set, and can be read.
  ///
  /// \param buffer The buffer to write to.
  ///
  /// \return The current request.
  HTTPGeneralHeader& read(mbed::Span<char> buffer);

  /// \brief Read the request into a buffer.
  ///
  /// \see read(mbed::Span<char>).
  HTTPGeneralHeader& read(char* buffer, std::size_t count)
  {
    return this->read(mbed::Span(buffer, count));
  }

 private:
};

/// \brief A structure containing an HTTP
/// [request-header](https://tools.ietf.org/html/rfc2616#section-7.1).
struct HTTPEntityHeader
{
 public:
  std::string_view allow;
  std::string_view content_encoding;
  std::string_view content_language;
  std::string_view content_length;
  std::string_view content_location;
  std::string_view content_md5;
  std::string_view content_range;
  std::string_view content_type;
  std::string_view expires;
  std::string_view last_modified;
  std::string_view extension_header; //!<\brief Any extension header.

  /// \brief Return a handle to the matching standard header field, or nullptr
  /// if no match is found.
  ///
  /// \param tag The header field to match.
  constexpr std::string_view* getField(std::string_view tag);

  /// \brief return if the request has been fully read.
  bool eof() const;

  /// \brief Return if the request has failed to be read / the error code.
  HTTPError fail() const;

  /// \brief Reset the output stream to the beginning. Clears any error / eof
  /// flags.
  void reset() const;

  /// \brief Read the request into a buffer.
  ///
  /// Can be called multiple times to write the request in chunks without.
  /// Characters are extracted and stored until any of the following conditions
  /// occurs:
  /// - buffer.size() characters were extracted and stored.
  /// - The end of the request is reached, in which case eof() is set to true.
  /// - An error occurs, in which case fail() is set, and can be read.
  ///
  /// \param buffer The buffer to write to.
  ///
  /// \return The current request.
  HTTPEntityHeader& read(mbed::Span<char> buffer);

  /// \brief Read the request into a buffer.
  ///
  /// \see read(mbed::Span<char>).
  HTTPEntityHeader& read(char* buffer, std::size_t count)
  {
    return this->read(mbed::Span(buffer, count));
  }

 private:
};

/// \brief A structure containing an HTTP request payload.
///
/// This structure is a reference to the data. It does not actually own
/// anything.
///
/// \see [rfc2616e](https://tools.ietf.org/html/rfc2616#section-5) for standard
/// specification.
class HTTPRequest
{
 public:
  HTTPMethod        method;
  std::string_view  uri;
  HTTPGeneralHeader general_header;
  HTTPRequestHeader request_header;
  HTTPEntityHeader  entity_header;
  std::string_view  message_body;

 public:
  constexpr HTTPRequest() = default;

  constexpr HTTPRequest(const HTTPRequest&)            = default;
  constexpr HTTPRequest& operator=(const HTTPRequest&) = default;
  ~HTTPRequest()                                       = default;

  /// \brief Return if the request format is valid.
  bool   valid() const;
  inline operator bool() const { return valid(); }

  /// \brief return if the request has been fully read.
  bool eof() const;

  /// \brief Return if the request has failed to be read / the error code.
  HTTPError fail() const;

  /// \brief Reset the output stream to the beginning. Clears any error / eof
  /// flags.
  void reset() const;

  /// \brief Read the request into a buffer.
  ///
  /// Can be called multiple times to write the request in chunks without.
  /// Characters are extracted and stored until any of the following conditions
  /// occurs:
  /// - buffer.size() characters were extracted and stored.
  /// - The end of the request is reached, in which case eof() is set to true.
  /// - An error occurs, in which case fail() is set, and can be read.
  ///
  /// \param buffer The buffer to write to.
  ///
  /// \return The current request.
  HTTPRequest& read(mbed::Span<char> buffer);

  /// \brief Read the request into a buffer.
  ///
  /// \see read(mbed::Span<char>).
  HTTPRequest& read(char* buffer, std::size_t count)
  {
    return this->read(mbed::Span(buffer, count));
  }

 private:
};

/// \brief A structure containing an HTTP response payload.
///
/// \see [rfc2616e](https://tools.ietf.org/html/rfc2616#section-6) for standard
/// specification.
struct HTTPResponse
{
 public:
  HTTPStatusCode     status_code;
  HTTPGeneralHeader  general_header;
  HTTPResponseHeader response_header;
  HTTPEntityHeader   entity_header;
  std::string_view   message_body;

  HTTPResponse() = default;

  constexpr HTTPResponse(const HTTPResponse&)            = default;
  constexpr HTTPResponse& operator=(const HTTPResponse&) = default;
  ~HTTPResponse()                                        = default;
};

/// \brief A virtual interface for an HTTP/1.1 client.
class HTTPClient
{
 public:
  /// \brief Perform a generic HTTP request.
  HTTPError Request(
    const HTTPRequest&        request,
    HTTPResponse&             response,
    std::chrono::microseconds timeout = std::chrono::microseconds{10'000},
    mbed::Callback<HTTPError(mbed::Span<char>)> data_callback = {});

 protected:
  virtual HTTPError sendRequest(const HTTPRequest& request) = 0;

  virtual HTTPError waitForResponse(std::chrono::microseconds timeout) = 0;

  virtual HTTPError registerDataCallback(
    mbed::Callback<HTTPError(mbed::Span<char>)> data_callback) = 0;

  rtos::EventFlags event_flags_;
};

} // namespace rb

// ===================== Detail Implementation =======================

namespace rb {

#pragma region HTTPStatusCode

constexpr HTTPStatusCode::HTTPStatusCode(
  int              code,
  std::string_view reason_phrase) :
    code_(code),
    reason_phrase_(reason_phrase)
{
}

constexpr bool
HTTPStatusCode::standard() const
{
  return informational() || success() || redirection() || client_error() ||
    server_error();
}

constexpr bool
HTTPStatusCode::informational() const
{
  return code_ >= CONTINUE && code_ <= SWITCHING_PROTOCOLS;
}

constexpr bool
HTTPStatusCode::success() const
{
  return code_ >= OK && code_ <= TEMPORARY_REDIRECT;
}

constexpr bool
HTTPStatusCode::redirection() const
{
  return code_ >= MULTIPLE_CHOICES && code_ <= TEMPORARY_REDIRECT;
}

constexpr bool
HTTPStatusCode::client_error() const
{
  return code_ >= BAD_REQUEST && code_ <= EXPECTATION_FAILED;
}

constexpr bool
HTTPStatusCode::server_error() const
{
  return code_ >= INTERNAL_SERVER_ERROR && code_ <= HTTP_VERSION_NOT_SUPPORTED;
}

constexpr std::string_view
HTTPStatusCode::reason() const
{
  switch (code_) {
    case INVALID_:
      return "Invalid Status Code";
    case CONTINUE:
      return "Continue";
    case SWITCHING_PROTOCOLS:
      return "Switching Protocols";
    case OK:
      return "OK";
    case CREATED:
      return "Created";
    case ACCEPTED:
      return "Accepted";
    case NON_AUTHORITATIVE_INFORMATION:
      return "Non-Authoritative Information";
    case NO_CONTENT:
      return "No Content";
    case RESET_CONTENT:
      return "Reset Content";
    case PARTIAL_CONTENT:
      return "Partial Content";
    case MULTIPLE_CHOICES:
      return "Multiple Choices";
    case MOVED_PERMANENTLY:
      return "Moved Permanently";
    case FOUND:
      return "Found";
    case SEE_OTHER:
      return "See Other";
    case NOT_MODIFIED:
      return "Not Modified";
    case USE_PROXY:
      return "Use Proxy";
    case TEMPORARY_REDIRECT:
      return "Temporary Redirect";
    case BAD_REQUEST:
      return "Bad Request";
    case UNAUTHORIZED:
      return "Unauthorized";
    case PAYMENT_REQUIRED:
      return "Payment Required";
    case FORBIDDEN:
      return "Forbidden";
    case NOT_FOUND:
      return "Not Found";
    case METHOD_NOT_ALLOWED:
      return "Method Not Allowed";
    case NOT_ACCEPTABLE:
      return "Not Acceptable";
    case PROXY_AUTHENTICATION_REQUIRED:
      return "Proxy Authentication Required";
    case REQUEST_TIMEOUT:
      return "Request Timeout";
    case CONFLICT:
      return "Conflict";
    case GONE:
      return "Gone";
    case LENGTH_REQUIRED:
      return "Length Required";
    case PRECONDITION_FAILED:
      return "Precondition Failed";
    case REQUEST_ENTITY_TOO_LARGE:
      return "Request Entity Too Large";
    case REQUEST_URI_TOO_LARGE:
      return "Request URI Too Large";
    case UNSUPPORTED_MEDIA_TYPE:
      return "Unsupported Media Type";
    case REQUESTED_RANGE_NOT_SATISFIABLE:
      return "Requested Range Not Satisfiable";
    case EXPECTATION_FAILED:
      return "Expectation Failed";
    case INTERNAL_SERVER_ERROR:
      return "Internal Server Error";
    case NOT_IMPLEMENTED:
      return "Not Implemented";
    case BAD_GATEWAY:
      return "Bad Gateway";
    case SERVICE_UNAVAILABLE:
      return "Service Unavailable";
    case GATEWAY_TIMEOUT:
      return "Gateway Timeout";
    case HTTP_VERSION_NOT_SUPPORTED:
      return "HTTP Version Not Supported";
    default:
      return reason_phrase_;
  }
}

#pragma endregion HTTPStatusCode
#pragma region    HTTPMethod

constexpr HTTPMethod::HTTPMethod(std::string_view method) :
    code_(method.empty() ? INVALID_ : EXTENSION_METHOD),
    method_(method)
{
  if (code_) {
    constexpr auto iequals =
      [](std::string_view a, std::string_view b) constexpr {
        constexpr auto tolower = [](char c) constexpr {
          return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
        };

        if (a.size() != b.size())
          return false;
        for (std::size_t i = 0; i < a.size(); ++i)
          if (tolower(a[i]) != tolower(b[i])) {
            return false;
          }
        return true;
      };

    if (iequals(method_, "GET")) {
      code_ = GET;
    } else if (iequals(method_, "POST")) {
      code_ = POST;
    } else if (iequals(method_, "PUT")) {
      code_ = PUT;
    } else if (iequals(method_, "DELETE")) {
      code_ = DELETE;
    } else if (iequals(method_, "HEAD")) {
      code_ = HEAD;
    } else if (iequals(method_, "OPTIONS")) {
      code_ = OPTIONS;
    } else if (iequals(method_, "CONNECT")) {
      code_ = CONNECT;
    } else if (iequals(method_, "TRACE")) {
      code_ = TRACE;
    }
  }
}

constexpr std::string_view
HTTPMethod::method() const
{
  switch (code_) {
    case INVALID_:
      return "";
    case GET:
      return "GET";
    case POST:
      return "POST";
    case PUT:
      return "PUT";
    case DELETE:
      return "DELETE";
    case HEAD:
      return "HEAD";
    case OPTIONS:
      return "OPTIONS";
    case CONNECT:
      return "CONNECT";
    case TRACE:
      return "TRACE";
    default:
      return method_;
  }
}

#ifndef NDEBUG
static_assert(HTTPMethod("get").method() == "GET");
static_assert(HTTPMethod("pUt").method() == "PUT");
#endif // NDEBUG

#pragma endregion HTTPMethod
#pragma region    HTTPRequestHeader

constexpr std::string_view*
HTTPRequestHeader::getField(std::string_view tag)
{
  if (tag == "Accept")
    return &accept;
  else if (tag == "Accept-Charset")
    return &accept_charset;
  else if (tag == "Accept-Encoding")
    return &accept_encoding;
  else if (tag == "Accept-Language")
    return &accept_language;
  else if (tag == "Authorization")
    return &authorization;
  else if (tag == "Expect")
    return &expect;
  else if (tag == "From")
    return &from;
  else if (tag == "Host")
    return &host;
  else if (tag == "If-Match")
    return &if_match;
  else if (tag == "If-Modified-Since")
    return &if_modified_since;
  else if (tag == "If-None-Match")
    return &if_none_match;
  else if (tag == "If-Range")
    return &if_range;
  else if (tag == "If-Unmodified-Since")
    return &if_unmodified_since;
  else if (tag == "Max-Forwards")
    return &max_forwards;
  else if (tag == "Proxy-Authorization")
    return &proxy_authorization;
  else if (tag == "Range")
    return &range;
  else if (tag == "Referer")
    return &referer;
  else if (tag == "TE")
    return &te;
  else if (tag == "User-Agent")
    return &user_agent;
  else
    return nullptr;
}

#pragma endregion HTTPRequestHeader
#pragma region    HTTPResponseHeader

constexpr std::string_view*
HTTPResponseHeader::getField(std::string_view tag)
{
  if (tag == "Accept-Ranges")
    return &accept_ranges;
  else if (tag == "Age")
    return &age;
  else if (tag == "ETag")
    return &etag;
  else if (tag == "Location")
    return &location;
  else if (tag == "Proxy-Authenticate")
    return &proxy_authenticate;
  else if (tag == "Retry-After")
    return &retry_after;
  else if (tag == "Server")
    return &server;
  else if (tag == "Vary")
    return &vary;
  else if (tag == "WWW-Authenticate")
    return &www_authenticate;
  else
    return nullptr;
}

#pragma endregion HTTPResponseHeader
#pragma region    HTTPGeneralHeader

constexpr std::string_view*
HTTPGeneralHeader::getField(std::string_view tag)
{
  if (tag == "Cache-Control")
    return &cache_control;
  if (tag == "Connection")
    return &connection;
  if (tag == "Date")
    return &date;
  if (tag == "Pragma")
    return &pragma;
  if (tag == "Trailer")
    return &trailer;
  if (tag == "Transfer-Encoding")
    return &transfer_encoding;
  if (tag == "Upgrade")
    return &upgrade;
  if (tag == "Via")
    return &via;
  if (tag == "Warning")
    return &warning;
  return nullptr;
}

#pragma endregion HTTPGeneralHeader
#pragma region    HTTPEntityHeader

constexpr std::string_view*
HTTPEntityHeader::getField(std::string_view tag)
{
  if (tag == "Allow") {
    return &allow;
  } else if (tag == "Content-Encoding") {
    return &content_encoding;
  } else if (tag == "Content-Language") {
    return &content_language;
  } else if (tag == "Content-Length") {
    return &content_length;
  } else if (tag == "Content-Location") {
    return &content_location;
  } else if (tag == "Content-MD5") {
    return &content_md5;
  } else if (tag == "Content-Range") {
    return &content_range;
  } else if (tag == "Content-Type") {
    return &content_type;
  } else if (tag == "Expires") {
    return &expires;
  } else if (tag == "Last-Modified") {
    return &last_modified;
  } else {
    return nullptr;
  }
}

#pragma endregion HTTPEntityHeader

} // namespace rb

#endif // RB_HTTP_CLIENT_HPP