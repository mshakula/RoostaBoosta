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

// Disable global mbed namespace.
#ifndef MBED_NO_GLOBAL_USING_DIRECTIVE
#define MBED_NO_GLOBAL_USING_DIRECTIVE
#define MBED_NO_GLOBAL_USING_DIRECTIVE_DEFINED__
#endif // MBED_NO_GLOBAL_USING_DIRECTIVE

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <string_view>
#include <variant>

#include <platform/Callback.h>
#include <platform/Span.h>
#include <rtos/EventFlags.h>

#include "ErrorStatus.hpp"

// ======================= Public Interface ==========================

namespace rb {

/// \brief Helper handle that allows for serialization of a type T.
///
/// In order to actually use, class T should implement it in its entirety.
/// Contains a status of the serialization.
///
/// A serialization handle should be considered ephemeral and should not be
/// relied on to always be updated.
template<class T>
class HTTPSerializationHandle
{
  friend T;

 public:
  HTTPSerializationHandle(const T& obj) = delete;

  /// \brief Serialize the object into the buffer.
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
  /// \return The current serialization handle.
  HTTPSerializationHandle& serialize(mbed::Span<char> buffer) = delete;

  /// \brief Reset the serialization handle to beginning of stream.
  void reset() = delete;

  /// \brief Return true if the serialization handle is at the end of stream.
  bool eof() const = delete;

  /// \brief Return last error status.
  ///
  /// Common errors:
  /// - MBED_ERROR_CODE_ENODATA: Serialization has already completed.
  ErrorStatus fail() const = delete;

  /// \brief Return true if the serialization handle can still be read from.
  operator bool() const = delete;
};

/// \brief Common base for HTTPSerializationHandles to reduce boilerplate.
template<class T>
struct HTTPSerializationHandleBase
{
  HTTPSerializationHandleBase(const T& obj) : obj_{obj}, err_{} {}
  HTTPSerializationHandleBase(const HTTPSerializationHandleBase&) = default;
  HTTPSerializationHandleBase& operator=(const HTTPSerializationHandleBase&) =
    default;

  ErrorStatus fail() const { return err_; }

  const T&    obj_;
  ErrorStatus err_;
};

/// \brief Specialization for std::string_view.
///
/// \see HTTPSerializationHandle
template<>
class HTTPSerializationHandle<std::string_view> :
    private HTTPSerializationHandleBase<std::string_view>
{
 public:
  HTTPSerializationHandle(const std::string_view& obj) :
      HTTPSerializationHandleBase{obj},
      idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    std::size_t bytes_to_copy = std::min(buffer.size(), obj_.size() - idx_);
    if (bytes_to_copy <= 0) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
      return *this;
    }
    std::copy_n(obj_.data() + idx_, bytes_to_copy, buffer.begin());
    idx_ += bytes_to_copy;
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return idx_ >= obj_.size(); }

  operator bool() const { !eof() && !fail(); }

 private:
  std::size_t idx_;
};

/// \brief Helper to reduce boilerplate code for serialization.
template<class T>
struct HTTPSerializationBase
{
  /// \brief Return a new serialization handle for this object.
  HTTPSerializationHandle<T> get_serialization_handle() const
  {
    return HTTPSerializationHandle{*static_cast<T*>(this)};
  }

  /// \brief Serialize the object into a buffer.
  ///
  /// \see HTTPSerializationHandle::serialize
  HTTPSerializationHandle<T> serialize(mbed::Span<char> buffer) const
  {
    return get_serialization_handle().serialize(buffer);
  }

  /// \brief Serialize the object into a buffer.
  ///
  /// \see HTTPSerializationBase::serialize(mbed::Span<char>)
  HTTPSerializationHandle<T> serialize(char* buf, std::size_t size) const
  {
    return serialize(mbed::Span<char>{buf, size});
  }
};

/// \brief Struct representing an HTTP status code. See [Status
/// Code](https://tools.ietf.org/html/rfc2616#section-6.1.1)
class HTTPStatusCode : public HTTPSerializationBase<HTTPStatusCode>
{
  friend class HTTPSerializationHandle<HTTPStatusCode>;

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

  /// \brief Return the code for this request
  constexpr int code() const { return code_; }

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

    // 4xx Client ErrorStatus
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

    // 5xx Server ErrorStatus
    INTERNAL_SERVER_ERROR      = 500,
    NOT_IMPLEMENTED            = 501,
    BAD_GATEWAY                = 502,
    SERVICE_UNAVAILABLE        = 503,
    GATEWAY_TIMEOUT            = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
  };

 private:
  decltype(INVALID_) code_;
  std::string_view   reason_phrase_;
};

/// \brief A structure containing an HTTP request method, detailed in
/// [Method](https://tools.ietf.org/html/rfc2616#section-5.1.1)
class HTTPMethod : public HTTPSerializationBase<HTTPMethod>
{
  friend class HTTPSerializationHandle<HTTPStatusCode>;

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

  /// \brief Anonymous enum containing all standard HTTP methods.
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
  };

 private:
  decltype(EXTENSION_METHOD) code_;
  std::string_view           method_;
};

/// \brief A structure containing an HTTP
/// [request-header](https://tools.ietf.org/html/rfc2616#section-5.3),
struct HTTPRequestHeader : public HTTPSerializationBase<HTTPRequestHeader>
{
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
};

/// \brief A structure containing an HTTP
/// [response-header](https://tools.ietf.org/html/rfc2616#section-6.2),
struct HTTPResponseHeader : public HTTPSerializationBase<HTTPResponseHeader>
{
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
};

/// \brief A structure containing an HTTP
/// [general-header](https://tools.ietf.org/html/rfc2616#section-4.5).
struct HTTPGeneralHeader : public HTTPSerializationHandle<HTTPGeneralHeader>
{
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
};

/// \brief A structure containing an HTTP
/// [request-header](https://tools.ietf.org/html/rfc2616#section-7.1).
struct HTTPEntityHeader : public HTTPSerializationBase<HTTPEntityHeader>
{
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
  ErrorStatus fail() const;

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
};

/// \brief A structure containing an HTTP request payload.
///
/// This structure is a reference to the data. It does not actually own
/// anything.
///
/// \see [rfc2616e](https://tools.ietf.org/html/rfc2616#section-5) for standard
/// specification.
struct HTTPRequest : public HTTPSerializationBase<HTTPRequest>
{
  HTTPMethod        method;
  std::string_view  uri;
  HTTPGeneralHeader general_header;
  HTTPRequestHeader request_header;
  HTTPEntityHeader  entity_header;
  std::string_view  message_body;

  constexpr HTTPRequest() = default;

  constexpr HTTPRequest(const HTTPRequest&)            = default;
  constexpr HTTPRequest& operator=(const HTTPRequest&) = default;
  ~HTTPRequest()                                       = default;

  /// \brief Return if the request format is valid.
  bool   valid() const;
  inline operator bool() const { return valid(); }
};

/// \brief A structure containing an HTTP response payload.
///
/// This structure is a reference to the data. It does not actually own
/// anything.
///
/// \see [rfc2616e](https://tools.ietf.org/html/rfc2616#section-6) for standard
/// specification.
class HTTPResponse
{
  friend class HTTPClient;

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

 private:
  rtos::EventFlags event_flags_; // Event flags over cond variable for
                                 // signalling from ISR.
};

/// \brief A virtual interface for an HTTP/1.1 client.
///
/// Subclasses must implement the provided virtual functions to implement the
/// HTTP Request function.
class HTTPClient
{
 public:
  /// \brief Default constructor.
  HTTPClient() = default;

  HTTPClient(const HTTPClient&)            = default;
  HTTPClient& operator=(const HTTPClient&) = default;

  /// \brief Virtual destructor for polymorphism.
  virtual ~HTTPClient() = default;

  /// \brief Perform a generic thread-blocking HTTP request.
  ///
  /// Currently, this is a blocking API, however it may be changed to be
  /// non-blocking in the future... seems like not too hard with the current
  /// implementation.
  ///
  /// \param request The request to send.
  /// \param response The response object to read into.
  /// \param timeout The timeout for the request.
  /// \param rcv_callback The callback to call when data is received. May be
  /// called from ISR context. Usually, this should be left empty, and
  /// data_callback / header_callback should be used instead. Only useful in the
  /// case of unbuffered transports, where data must be caught in the ISR. In
  /// the case of an mbed::BufferedSerial underlying transport, this is not
  /// necessary. Useful if need to read data into buffer defined at Request
  /// time, not Client creation time.
  ///
  /// \return Non-zero error code on failure.
  ErrorStatus Request(
    const HTTPRequest&        request,
    HTTPResponse&             response,
    std::chrono::milliseconds timeout =
      std::chrono::milliseconds{HTTP_CLIENT_DEFAULT_TIMEOUT},
    mbed::Callback<void()> rcv_callback = {});

 protected:
  /// \brief Send a request over the underlying transport. May be blocking.
  ///
  /// \param request The request to send.
  ///
  /// \return Non-zero error code on failure.
  virtual ErrorStatus sendRequest(const HTTPRequest& request) = 0;

  /// \brief Abort receive operation.
  virtual void abort() = 0;

  /// \brief Return the amount of available bytes to read.
  virtual std::size_t available() const = 0;

  /// \brief Read into buffer from the underlying transport.
  ///
  /// \param buffer The buffer to read into. Reads at most buffer.size() bytes.
  ///
  /// \return If no error, ret.value will be the number of bytes read.
  virtual ErrorStatus read(mbed::Span<char> buffer) = 0;

  /// \brief Read into buffer from the underlying transport.
  ///
  /// \see read(mbed::Span<char>, std::size_t&).
  inline ErrorStatus read(char* buffer, std::size_t count)
  {
    return this->read(mbed::Span(buffer, count));
  }

  /// \brief Register callback to call whenever data is available. Can be called
  /// multiple times.
  ///
  /// \param input_callback The callback to add. May run in ISR context.
  ///
  /// \return Non-zero error code on failure.
  virtual ErrorStatus registerInputCallback(
    mbed::Callback<void()> input_callback) = 0;

  /// \brief Unregister callback to call whensever data is available.
  ///
  /// \see registerInputCallback(mbed::Callback<ErrorStatus(HTTPResponse&)>).
  virtual void unregisterInputCallback() = 0;
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
      return "Internal Server ErrorStatus";
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
      return reason_phrase_.empty() ? "Unknown" : reason_phrase_;
  }
}

template<>
class HTTPSerializationHandle<HTTPStatusCode> :
    private HTTPSerializationHandleBase<HTTPStatusCode>
{
 public:
  HTTPSerializationHandle(const HTTPStatusCode& obj) :
      HTTPSerializationHandleBase{obj},
      idx_{0},
      req_{0},
      code_buffer_{0}
  {
    req_ = std::snprintf(code_buffer_.data(), code_buffer_.size(), "%d", obj_);
    if (req_ >= code_buffer_.size())
      RB_ERROR(ErrorStatus{
        MBED_ERROR_CODE_ASSERTION_FAILED,
        "Failed to pre-serialize HTTPStatusCode. Required buffer size too "
        "large.",
        req_});
  }

  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (idx_ >= req_) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
      return *this;
    }

    auto to_write = std::min(req_ - idx_, buffer.size());
    std::memcpy(buffer.data(), code_buffer_.data() + idx_, to_write);
    idx_ += to_write;
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return idx_ == req_; }

  operator bool() const { return !fail() && !eof(); }

 private:
  std::size_t         idx_;
  std::size_t         req_;
  std::array<char, 6> code_buffer_;
};

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

  static_assert(HTTPMethod("get").method() == "GET");
  static_assert(HTTPMethod("pUt").method() == "PUT");
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

template<>
class HTTPSerializationHandle<HTTPMethod> :
    public HTTPSerializationHandle<std::string_view>
{
 public:
  HTTPSerializationHandle(const HTTPMethod& obj) :
      HTTPSerializationHandle<std::string_view>{obj.method()},
  {
  }
};

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

template<>
class HTTPSerializationHandle<HTTPRequestHeader> :
    public HTTPSerializationHandleBase<HTTPRequestHeader>
{
  using namespace std::literals::string_view_literals;

 public:
  HTTPSerializationHandle(const HTTPRequestHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  const auto& get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Accept"sv;
      case 1:
        return this->obj_.accept;
      case 2:
        return "Accept-Charset"sv;
      case 3:
        return this->obj_.accept_charset;
      case 4:
        return "Accept-Encoding"sv;
      case 5:
        return this->obj_.accept_encoding;
      case 6:
        return "Accept-Language"sv;
      case 7:
        return this->obj_.accept_language;
      case 8:
        return "Authorization"sv;
      case 9:
        return this->obj_.authorization;
      case 10:
        return "Expect"sv;
      case 11:
        return this->obj_.expect;
      case 12:
        return "From"sv;
      case 13:
        return this->obj_.from;
      case 14:
        return "Host"sv;
      case 15:
        return this->obj_.host;
      case 16:
        return "If-Match"sv;
      case 17:
        return this->obj_.if_match;
      case 18:
        return "If-Modified-Since"sv;
      case 19:
        return this->obj_.if_modified_since;
      case 20:
        return "If-None-Match"sv;
      case 21:
        return this->obj_.if_none_match;
      case 22:
        return "If-Range"sv;
      case 23:
        return this->obj_.if_range;
      case 24:
        return "If-Unmodified-Since"sv;
      case 25:
        return this->obj_.if_unmodified_since;
      case 26:
        return "Max-Forwards"sv;
      case 27:
        return this->obj_.max_forwards;
      case 28:
        return "Proxy-Authorization"sv;
      case 29:
        return this->obj_.proxy_authorization;
      case 30:
        return "Range"sv;
      case 31:
        return this->obj_.range;
      case 32:
        return "Referer"sv;
      case 33:
        return this->obj_.referer;
      case 34:
        return "TE"sv;
      case 35:
        return this->obj_.te;
      case 36:
        return "User-Agent"sv;
      case 37:
        return this->obj_.user_agent;
      default:
        return ""sv;
    }
  }

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this]() {
      if (!child_handle_.serialize(buffer)) {
        if ((err_ = child_handle_.fail())) {
          return true; // error condition -- stop.
        } else {
          return false; // eof condition -- continue.
        }
      }
      return true; // wrote maximum bytes -- stop.
    };

    if (child_idx_ <= 37) {
      while (child_idx <= 37) {
        if (f())
          return *this;
        child_handle_ = HTTPSerializationHandle{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
    }
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 37; }

  operator bool() const { return !fail() && !eof(); }

 private:
  HTTPSerializationHandle<std::string_view> child_handle_;
  std::size_t                               child_idx_;
};

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

template<>
class HTTPSerializationHandle<HTTPResponseHeader> :
    public HTTPSerializationHandleBase<HTTPResponseHeader>
{
 public:
  HTTPSerializationHandle(const HTTPResponseHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  const auto& get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Accept-Ranges"sv;
      case 1:
        return this->obj_.accept_ranges;
      case 2:
        return "Age"sv;
      case 3:
        return this->obj_.age;
      case 4:
        return "ETag"sv;
      case 5:
        return this->obj_.etag;
      case 6:
        return "Location"sv;
      case 7:
        return this->obj_.location;
      case 8:
        return "Proxy-Authenticate"sv;
      case 9:
        return this->obj_.proxy_authenticate;
      case 10:
        return "Retry-After"sv;
      case 11:
        return this->obj_.retry_after;
      case 12:
        return "Server"sv;
      case 13:
        return this->obj_.server;
      case 14:
        return "Vary"sv;
      case 15:
        return this->obj_.vary;
      case 16:
        return "WWW-Authenticate"sv;
      case 17:
        return this->obj_.www_authenticate;
      default:
        return ""sv;
    }
  }

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this]() {
      if (!child_handle_.serialize(buffer)) {
        if ((err_ = child_handle_.fail())) {
          return true; // error condition -- stop.
        } else {
          return false; // eof condition -- continue.
        }
      }
      return true; // wrote maximum bytes -- stop.
    };

    if (child_idx_ <= 17) {
      while (child_idx <= 17) {
        if (f())
          return *this;
        child_handle_ = HTTPSerializationHandle{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
    }
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 17; }

  operator bool() const { return !fail() && !eof(); }

 private:
  HTTPSerializationHandle<std::string_view> child_handle_;
  std::size_t                               child_idx_;
};

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

template<>
class HTTPSerializationHandle<HTTPGeneralHeader> :
    public HTTPSerializationHandleBase<HTTPGeneralHeader>
{
 public:
  HTTPSerializationHandle(const HTTPGeneralHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  const auto& get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Cache-Control"sv;
      case 1:
        return this->obj_.cache_control;
      case 2:
        return "Connection"sv;
      case 3:
        return this->obj_.connection;
      case 4:
        return "Date"sv;
      case 5:
        return this->obj_.date;
      case 6:
        return "Pragma"sv;
      case 7:
        return this->obj_.pragma;
      case 8:
        return "Trailer"sv;
      case 9:
        return this->obj_.trailer;
      case 10:
        return "Transfer-Encoding"sv;
      case 11:
        return this->obj_.transfer_encoding;
      case 12:
        return "Upgrade"sv;
      case 13:
        return this->obj_.upgrade;
      case 14:
        return "Via"sv;
      case 15:
        return this->obj_.via;
      case 16:
        return "Warning"sv;
      case 17:
        return this->obj_.warning;
      default:
        return ""sv;
    }
  }

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this]() {
      if (!child_handle_.serialize(buffer)) {
        if ((err_ = child_handle_.fail())) {
          return true; // error condition -- stop.
        } else {
          return false; // eof condition -- continue.
        }
      }
      return true; // wrote maximum bytes -- stop.
    };

    if (child_idx_ <= 17) {
      while (child_idx <= 17) {
        if (f())
          return *this;
        child_handle_ = HTTPSerializationHandle{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
    }
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 17; }

  operator bool() const { return !fail() && !eof(); }

 private:
  HTTPSerializationHandle<std::string_view> child_handle_;
  std::size_t                               child_idx_;
};

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

template<>
class HTTPSerializationHandle<HTTPEntityHeader> :
    public HTTPSerializationHandleBase<HTTPEntityHeader>
{
 public:
  HTTPSerializationHandle(const HTTPEntityHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  const auto& get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Allow"sv;
      case 1:
        return this->obj_.allow;
      case 2:
        return "Content-Encoding"sv;
      case 3:
        return this->obj_.content_encoding;
      case 4:
        return "Content-Language"sv;
      case 5:
        return this->obj_.content_language;
      case 6:
        return "Content-Length"sv;
      case 7:
        return this->obj_.content_length;
      case 8:
        return "Content-Location"sv;
      case 9:
        return this->obj_.content_location;
      case 10:
        return "Content-MD5"sv;
      case 11:
        return this->obj_.content_md5;
      case 12:
        return "Content-Range"sv;
      case 13:
        return this->obj_.content_range;
      case 14:
        return "Content-Type"sv;
      case 15:
        return this->obj_.content_type;
      case 16:
        return "Expires"sv;
      case 17:
        return this->obj_.expires;
      case 18:
        return "Last-Modified"sv;
      case 19:
        return this->obj_.last_modified;
      case 20:
        return this->obj_.extension_header;
      default:
        return ""sv;
    }
  }

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this]() {
      if (!child_handle_.serialize(buffer)) {
        if ((err_ = child_handle_.fail())) {
          return true; // error condition -- stop.
        } else {
          return false; // eof condition -- continue.
        }
      }
      return true; // wrote maximum bytes -- stop.
    };

    if (child_idx_ <= 20) {
      while (child_idx <= 20) {
        if (f())
          return *this;
        child_handle_ = HTTPSerializationHandle{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
    }
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 20; }

  operator bool() const { return !fail() && !eof(); }

 private:
  HTTPSerializationHandle<std::string_view> child_handle_;
  std::size_t                               child_idx_;
};

#pragma endregion HTTPEntityHeader

} // namespace rb

// Undefine MBED_NO_GLOBAL_USING_DIRECTIVE if it was defined in this header.
#ifdef MBED_NO_GLOBAL_USING_DIRECTIVE_DEFINED__
#undef MBED_NO_GLOBAL_USING_DIRECTIVE
#undef MBED_NO_GLOBAL_USING_DIRECTIVE_DEFINED__
#endif // MBED_NO_GLOBAL_USING_DIRECTIVE_DEFINED__

#endif // RB_HTTP_CLIENT_HPP
