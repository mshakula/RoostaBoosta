/// \file HTTPSerializationHandle.ipp
/// \date 2023-04-30
/// \author mshakula (matvey@gatech.edu)
///
/// \brief Include implementation file for HTTPSerializationHandle template to
/// reduce clutter in HTTPClient.hpp.

#ifndef RB_HTTP_SERIALIZATION_HANDLE_IPP
#define RB_HTTP_SERIALIZATION_HANDLE_IPP

#ifndef __cplusplus
#error "HTTPSerializationHandle.ipp is a cxx-only header."
#endif // __cplusplus

#ifndef RB_HTTP_CLIENT_HPP
#error "HTTPSerializationHandle.ipp should only be included from HTTPClient.hpp"
#endif                    // RB_HTTP_CLIENT_HPP
#include "HTTPClient.hpp" // silence IDE warnings.

// ===================== Detail Implementation =======================

/// \brief Common base for HTTPSerializationHandles to reduce boilerplate.
template<class T>
struct HTTPSerializationHandleBase
{
  HTTPSerializationHandleBase(const T& obj, ErrorStatus err = {}) :
      obj_{obj},
      err_{err},
      gcount_{0}
  {
  }
  HTTPSerializationHandleBase(const HTTPSerializationHandleBase& o) :
      obj_{o.obj_},
      err_{o.err_},
      gcount_{o.gcount_}
  {
  }
  HTTPSerializationHandleBase& operator=(const HTTPSerializationHandleBase& o)
  {
    std::destroy_at(this);
    ::new (this) HTTPSerializationHandleBase{o.obj_, o.err_};
    return *this;
  }

  ErrorStatus fail() const { return err_; }

  std::size_t gcount() const { return gcount_; }

  const T&    obj_;
  ErrorStatus err_;
  std::size_t gcount_;
};

/// \brief Specialization for const char* to exhibit value semantics.
template<>
class HTTPSerializationHandle<const char*>
{
 public:
  HTTPSerializationHandle(const char* obj) :
      obj_{obj},
      err_{},
      ptr_{obj},
      gcount_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0};
      return *this;
    } else if (eof()) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        ptr_ - obj_};
      return *this;
    }

    gcount_ = 0;
    while (!eof() && gcount_ < buffer.size()) {
      buffer[gcount_++] = *ptr_++;
    }
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return *ptr_ == '\0'; }

  std::size_t gcount() const { return gcount_; }

  ErrorStatus fail() const { return err_; }

  operator bool() const { return !eof() && !fail(); }

 private:
  const char* obj_;
  ErrorStatus err_;
  const char* ptr_;
  std::size_t gcount_;
};

/// \brief Specialization for std::string_view to exhibit value semantics.
///
/// \see HTTPSerializationHandle
template<>
class HTTPSerializationHandle<std::string_view>
{
 public:
  HTTPSerializationHandle(const std::string_view& obj) :
      obj_{obj},
      err_{},
      idx_{0},
      gcount_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0};
      return *this;
    } else if (eof()) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
      return *this;
    }

    gcount_ =
      std::min(static_cast<std::size_t>(buffer.size()), obj_.size() - idx_);
    if (gcount_ == 0) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_};
      return *this;
    }
    std::copy_n(obj_.data() + idx_, gcount_, buffer.begin());
    idx_ += gcount_;
    return *this;
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return idx_ >= obj_.size(); }

  std::size_t gcount() const { return gcount_; }

  ErrorStatus fail() const { return err_; }

  operator bool() const { return !eof() && !fail(); }

 private:
  std::string_view obj_;
  ErrorStatus      err_;
  std::size_t      idx_;
  std::size_t      gcount_;
};

template<>
class HTTPSerializationHandle<HTTPStatusCode> :
    public HTTPSerializationHandleBase<HTTPStatusCode>
{
 public:
  HTTPSerializationHandle(const HTTPStatusCode& obj) :
      HTTPSerializationHandleBase{obj},
      idx_{0},
      req_{0},
      code_buffer_{0}
  {
    req_ = std::snprintf(
      code_buffer_.data(),
      code_buffer_.size(),
      "%d",
      static_cast<int>(obj_.code()));
    if (req_ >= code_buffer_.size())
      RB_ERROR(ErrorStatus(
        MBED_ERROR_CODE_ASSERTION_FAILED,
        "Failed to pre-serialize HTTPStatusCode. Required buffer size too "
        "large.",
        req_));
  }

  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus{
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0};
      return *this;
    } else if (idx_ >= req_) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_);
      return *this;
    }

    gcount_ = std::min(static_cast<std::size_t>(buffer.size()), req_ - idx_);
    std::memcpy(buffer.data(), code_buffer_.data() + idx_, gcount_);
    idx_ += gcount_;
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

template<>
class HTTPSerializationHandle<HTTPMethod> :
    public HTTPSerializationHandle<std::string_view>
{
 public:
  HTTPSerializationHandle(const HTTPMethod& obj) :
      HTTPSerializationHandle<std::string_view>{obj.method()}
  {
  }
};

template<>
class HTTPSerializationHandle<HTTPRequestHeader> :
    public HTTPSerializationHandleBase<HTTPRequestHeader>
{
 public:
  HTTPSerializationHandle(const HTTPRequestHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_idx_{0},
      child_handle_(
        std::in_place_index_t<0>{},
        HTTPSerializationHandle<const char*>("Accept: "))
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0);
      return *this;
    } else if (eof()) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
      return *this;
    }

    auto f = [this, &buffer](auto& field) {
      if (!field.serialize(buffer)) {
        if ((err_ = field.fail())) {
          gcount_ += field.gcount();
          return true; // error condition -- stop.
        } else {
          gcount_ += field.gcount();
          buffer = mbed::Span<char>{
            buffer.data() + field.gcount(), buffer.size() - field.gcount()};
          return false; // eof condition -- continue.
        }
      }
      gcount_ += field.gcount();
      return true; // wrote maximum bytes -- stop.
    };

#ifdef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
#error "RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD already defined."
#endif
#define RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(                       \
  field_id, dependent_field, next_field)                                    \
  case field_id:                                                            \
    if (!dependent_field.empty() && f(std::get<field_id>(child_handle_))) { \
      return *this;                                                         \
    }                                                                       \
    child_handle_.emplace<field_id + 1>(                                    \
      HTTPSerializationHandle<std::decay_t<decltype(next_field)>>(          \
        next_field));                                                       \
    ++child_idx_;

    gcount_ = 0;
    switch (child_idx_) {
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(0, obj_.accept, obj_.accept);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(1, obj_.accept, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        2, obj_.accept, "Accept-Charset: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        3, obj_.accept_charset, obj_.accept_charset);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        4, obj_.accept_charset, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        5, obj_.accept_charset, "Accept-Encoding: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        6, obj_.accept_encoding, obj_.accept_encoding);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        7, obj_.accept_encoding, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        8, obj_.accept_encoding, "Accept-Language: ")

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        9, obj_.accept_language, obj_.accept_language);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        10, obj_.accept_language, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        11, obj_.accept_language, "Authorization: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        12, obj_.authorization, obj_.authorization);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        13, obj_.authorization, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        14, obj_.authorization, "Expect: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        15, obj_.expect, obj_.expect);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(16, obj_.expect, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(17, obj_.expect, "From: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(18, obj_.from, obj_.from);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(19, obj_.from, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(20, obj_.from, "Host: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(21, obj_.host, obj_.host);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(22, obj_.host, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(23, obj_.host, "If-Match: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        24, obj_.if_match, obj_.if_match);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(25, obj_.if_match, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        26, obj_.if_match, "If-Modified-Since: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        27, obj_.if_modified_since, obj_.if_modified_since);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        28, obj_.if_modified_since, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        29, obj_.if_modified_since, "If-None-Match: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        30, obj_.if_none_match, obj_.if_none_match);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        31, obj_.if_none_match, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        32, obj_.if_none_match, "If-Range: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        33, obj_.if_range, obj_.if_range);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(34, obj_.if_range, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        35, obj_.if_range, "If-Unmodified-Since: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        36, obj_.if_unmodified_since, obj_.if_unmodified_since);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        37, obj_.if_unmodified_since, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        38, obj_.if_unmodified_since, "Max-Forwards: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        39, obj_.max_forwards, obj_.max_forwards);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        40, obj_.max_forwards, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        41, obj_.max_forwards, "Proxy-Authorization: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        42, obj_.proxy_authorization, obj_.proxy_authorization);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        43, obj_.proxy_authorization, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        44, obj_.proxy_authorization, "Range: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(45, obj_.range, obj_.range);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(46, obj_.range, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(47, obj_.range, "Referer: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        48, obj_.referer, obj_.referer);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(49, obj_.referer, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(50, obj_.referer, "TE: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(51, obj_.te, obj_.te);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(52, obj_.te, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(53, obj_.te, "User-Agent: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        54, obj_.user_agent, obj_.user_agent);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(55, obj_.user_agent, "\r\n");
      case 56:
        if (!obj_.user_agent.empty() && f(std::get<56>(child_handle_))) {
          return *this;
        }
        ++child_idx_;
    }
    return *this;
#undef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 56; }

  operator bool() const { return !fail() && !eof(); }

 private:
  std::size_t child_idx_;
  std::variant<
    HTTPSerializationHandle<const char*>,      // Accept
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Accept-Charset
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Accept-Encoding
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Accept-Language
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Authorization
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Expect
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // From
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Host
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // If-Match
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // If-Modified-Since
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // If-None-Match
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // If-Range
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // If-Unmodified-Since
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Max-Forwards
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Proxy-Authorization
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Range
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Referer
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // TE
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // User-Agent
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>>      // crlf
    child_handle_;
};

template<>
class HTTPSerializationHandle<HTTPResponseHeader> :
    public HTTPSerializationHandleBase<HTTPResponseHeader>
{
 public:
  HTTPSerializationHandle(const HTTPResponseHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_idx_{0},
      child_handle_(
        std::in_place_index_t<0>{},
        HTTPSerializationHandle<const char*>("Accept-Ranges: "))
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0);
      return *this;
    } else if (eof()) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
      return *this;
    }

    auto f = [this, &buffer](auto& field) {
      if (!field.serialize(buffer)) {
        if ((err_ = field.fail())) {
          gcount_ += field.gcount();
          return true; // error condition -- stop.
        } else {
          gcount_ += field.gcount();
          buffer = mbed::Span<char>{
            buffer.data() + field.gcount(), buffer.size() - field.gcount()};
          return false; // eof condition -- continue.
        }
      }
      gcount_ += field.gcount();
      return true; // wrote maximum bytes -- stop.
    };

#ifdef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
#error "RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD already defined."
#endif
#define RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(                       \
  field_id, dependent_field, next_field)                                    \
  case field_id:                                                            \
    if (!dependent_field.empty() && f(std::get<field_id>(child_handle_))) { \
      return *this;                                                         \
    }                                                                       \
    child_handle_.emplace<field_id + 1>(                                    \
      HTTPSerializationHandle<std::decay_t<decltype(next_field)>>(          \
        next_field));                                                       \
    ++child_idx_;

    gcount_ = 0;
    switch (child_idx_) {
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        0, obj_.accept_ranges, obj_.accept_ranges);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        1, obj_.accept_ranges, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        2, obj_.accept_ranges, "Age: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(3, obj_.age, obj_.age);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(4, obj_.age, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(5, obj_.age, "Etag: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(6, obj_.etag, obj_.etag);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(7, obj_.etag, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(8, obj_.etag, "Location: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        9, obj_.location, obj_.location);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(10, obj_.location, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        11, obj_.location, "Proxy-Authenticate: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        12, obj_.proxy_authenticate, obj_.proxy_authenticate);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        13, obj_.proxy_authenticate, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        14, obj_.proxy_authenticate, "Retry-After: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        15, obj_.retry_after, obj_.retry_after);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        16, obj_.retry_after, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        17, obj_.retry_after, "Server: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        18, obj_.server, obj_.server);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(19, obj_.server, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(20, obj_.server, "Vary: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(21, obj_.vary, obj_.vary);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(22, obj_.vary, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        23, obj_.vary, "WWW-Authenticate: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        24, obj_.www_authenticate, obj_.www_authenticate);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        25, obj_.www_authenticate, "\r\n");
      case 26:
        if (!obj_.www_authenticate.empty() && f(std::get<26>(child_handle_))) {
          return *this;
        }
        ++child_idx_;
    }
    return *this;
#undef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 26; }

  operator bool() const { return !fail() && !eof(); }

 private:
  std::size_t child_idx_;
  std::variant<
    HTTPSerializationHandle<const char*>,      // Accept-Ranges
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Age
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Etag
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Location
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Proxy-Authenticate
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Retry-After
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Server
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Vary
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // WWW-Authenticate
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>>      // crlf
    child_handle_;
};

template<>
class HTTPSerializationHandle<HTTPGeneralHeader> :
    public HTTPSerializationHandleBase<HTTPGeneralHeader>
{
 public:
  HTTPSerializationHandle(const HTTPGeneralHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_idx_{0},
      child_handle_(
        std::in_place_index_t<0>{},
        HTTPSerializationHandle<const char*>("Cache-Control: "))
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0);
      return *this;
    } else if (eof()) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
      return *this;
    }

    auto f = [this, &buffer](auto& field) {
      if (!field.serialize(buffer)) {
        if ((err_ = field.fail())) {
          gcount_ += field.gcount();
          return true; // error condition -- stop.
        } else {
          gcount_ += field.gcount();
          buffer = mbed::Span<char>{
            buffer.data() + field.gcount(), buffer.size() - field.gcount()};
          return false; // eof condition -- continue.
        }
      }
      gcount_ += field.gcount();
      return true; // wrote maximum bytes -- stop.
    };

#ifdef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
#error "RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD already defined."
#endif
#define RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(                       \
  field_id, dependent_field, next_field)                                    \
  case field_id:                                                            \
    if (!dependent_field.empty() && f(std::get<field_id>(child_handle_))) { \
      return *this;                                                         \
    }                                                                       \
    child_handle_.emplace<field_id + 1>(                                    \
      HTTPSerializationHandle<std::decay_t<decltype(next_field)>>(          \
        next_field));                                                       \
    ++child_idx_;

    gcount_ = 0;
    switch (child_idx_) {
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        0, obj_.cache_control, obj_.cache_control);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        1, obj_.cache_control, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        2, obj_.cache_control, "Connection: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        3, obj_.connection, obj_.connection);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(4, obj_.connection, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        5, obj_.connection, "Date: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(6, obj_.date, obj_.date);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(7, obj_.date, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(8, obj_.date, "Pragma: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(9, obj_.pragma, obj_.pragma);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(10, obj_.pragma, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        11, obj_.pragma, "Trailer: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        12, obj_.trailer, obj_.trailer);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(13, obj_.trailer, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        14, obj_.trailer, "Transfer-Encoding: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        15, obj_.transfer_encoding, obj_.transfer_encoding);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        16, obj_.transfer_encoding, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        17, obj_.transfer_encoding, "Upgrade: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        18, obj_.upgrade, obj_.upgrade);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(19, obj_.upgrade, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(20, obj_.upgrade, "Via: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(21, obj_.via, obj_.via);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(22, obj_.via, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(23, obj_.via, "Warning: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        24, obj_.warning, obj_.warning);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(25, obj_.warning, "\r\n");
      case 26:
        if (!obj_.warning.empty() && f(std::get<26>(child_handle_))) {
          return *this;
        }
        ++child_idx_;
    }
    return *this;
#undef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 26; }

  operator bool() const { return !fail() && !eof(); }

 private:
  std::size_t child_idx_;
  std::variant<
    HTTPSerializationHandle<const char*>,      // Cache-Control
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Connection
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Date
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Pragma
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Trailer
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Transfer-Encoding
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Upgrade
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Via
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // Warning
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>>      // crlf
    child_handle_;
};

template<>
class HTTPSerializationHandle<HTTPEntityHeader> :
    public HTTPSerializationHandleBase<HTTPEntityHeader>
{
 public:
  HTTPSerializationHandle(const HTTPEntityHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_idx_{0},
      child_handle_(
        std::in_place_index_t<0>{},
        HTTPSerializationHandle<const char*>("Allow"))
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0);
      return *this;
    } else if (eof()) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
      return *this;
    }

    auto f = [this, &buffer](auto& field) {
      if (!field.serialize(buffer)) {
        if ((err_ = field.fail())) {
          gcount_ += field.gcount();
          return true; // error condition -- stop.
        } else {
          gcount_ += field.gcount();
          buffer = mbed::Span<char>{
            buffer.data() + field.gcount(), buffer.size() - field.gcount()};
          return false; // eof condition -- continue.
        }
      }
      gcount_ += field.gcount();
      return true; // wrote maximum bytes -- stop.
    };

#ifdef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
#error "RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD already defined."
#endif
#define RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(                       \
  field_id, dependent_field, next_field)                                    \
  case field_id:                                                            \
    if (!dependent_field.empty() && f(std::get<field_id>(child_handle_))) { \
      return *this;                                                         \
    }                                                                       \
    child_handle_.emplace<field_id + 1>(                                    \
      HTTPSerializationHandle<std::decay_t<decltype(next_field)>>(          \
        next_field));                                                       \
    ++child_idx_;

    gcount_ = 0;
    switch (child_idx_) {
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(0, obj_.allow, obj_.allow);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(1, obj_.allow, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        2, obj_.allow, "Content-Encoding: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        3, obj_.content_encoding, obj_.content_encoding);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        4, obj_.content_encoding, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        5, obj_.content_encoding, "Content-Language: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        6, obj_.content_language, obj_.content_language);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        7, obj_.content_language, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        8, obj_.content_language, "Content-Length: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        9, obj_.content_length, obj_.content_length);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        10, obj_.content_length, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        11, obj_.content_length, "Content-Location: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        12, obj_.content_location, obj_.content_location);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        13, obj_.content_location, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        14, obj_.content_location, "Content-MD5: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        15, obj_.content_md5, obj_.content_md5);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        16, obj_.content_md5, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        17, obj_.content_md5, "Content-Range: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        18, obj_.content_range, obj_.content_range);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        19, obj_.content_range, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        20, obj_.content_range, "Content-Type: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        21, obj_.content_type, obj_.content_type);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        22, obj_.content_type, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        23, obj_.content_type, "Expires: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        24, obj_.expires, obj_.expires);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(25, obj_.expires, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        26, obj_.expires, "Last-Modified: ");

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        27, obj_.last_modified, obj_.last_modified);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        28, obj_.last_modified, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        29, obj_.last_modified, obj_.extension_header);

      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(
        30, obj_.extension_header, "\r\n");
      case 31:
        if (!obj_.last_modified.empty() && f(std::get<31>(child_handle_))) {
          return *this;
        }
        ++child_idx_;
    }

    return *this;
#undef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 31; }

  operator bool() const { return !fail() && !eof(); }

 private:
  std::size_t child_idx_;
  std::variant<
    HTTPSerializationHandle<const char*>,      // accept
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // content-encoding
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // content-language
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // content-length
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // content-location
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // content-md5
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // content-range
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // content-type
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // expires
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<const char*>,      // last-modified
    HTTPSerializationHandle<std::string_view>, //
    HTTPSerializationHandle<const char*>,      // crlf
    HTTPSerializationHandle<std::string_view>, // extension-header
    HTTPSerializationHandle<const char*>>      // crlf
    child_handle_;
};

template<>
class HTTPSerializationHandle<HTTPRequest> :
    public HTTPSerializationHandleBase<HTTPRequest>
{
  HTTPSerializationHandle(const HTTPRequest& obj) :
      HTTPSerializationHandleBase{obj},
      child_idx_{0},
      child_handle_{HTTPSerializationHandle<decltype(obj_.method)>(obj_.method)}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle& o) :
      HTTPSerializationHandleBase{o.obj_},
      child_idx_{o.child_idx_},
      child_handle_{o.child_handle_}
  {
  }
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    if (buffer.size() == 0) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_EINVAL, "Buffer size must be greater than 0.", 0);
      return *this;
    } else if (eof()) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
      return *this;
    }

    auto f = [this, &buffer](auto& field) {
      if (!field.serialize(buffer)) {
        if ((err_ = field.fail())) {
          gcount_ += field.gcount();
          return true; // error condition -- stop.
        } else {
          gcount_ += field.gcount();
          buffer = mbed::Span<char>{
            buffer.data() + field.gcount(), buffer.size() - field.gcount()};
          return false; // eof condition -- continue.
        }
      }
      gcount_ += field.gcount();
      return true; // wrote maximum bytes -- stop.
    };

#ifdef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
#error "RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD already defined."
#endif
#define RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(field_id, next_field) \
  case field_id:                                                           \
    if (f(std::get<field_id>(child_handle_))) {                            \
      return *this;                                                        \
    }                                                                      \
    child_handle_.emplace<field_id + 1>(                                   \
      HTTPSerializationHandle<std::decay_t<decltype(next_field)>>(         \
        next_field));                                                      \
    ++child_idx_;

    gcount_ = 0;
    switch (child_idx_) {
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(0, " ");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(1, obj_.uri);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(2, " ");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(3, obj_.version);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(4, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(5, obj_.general_header);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(6, obj_.request_header);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(7, obj_.entity_header);
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(8, "\r\n");
      RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD(9, obj_.message_body);
      case 10:
        if (f(std::get<10>(child_handle_))) {
          return *this;
        }
        ++child_idx_;
    }

    return *this;
#undef RB_HTTP_SERIALIZATION_HANDLE_SERIALIZE_FIELD
  }

  void reset() { *this = HTTPSerializationHandle{obj_}; }

  bool eof() const { return child_idx_ > 10; }

  operator bool() const { return !fail() && !eof(); }

 private:
  std::size_t child_idx_;
  std::variant<
    HTTPSerializationHandle<HTTPMethod>,
    HTTPSerializationHandle<const char*>,       // space
    HTTPSerializationHandle<std::string_view>,  // uri
    HTTPSerializationHandle<const char*>,       // space
    HTTPSerializationHandle<std::string_view>,  // version
    HTTPSerializationHandle<const char*>,       // crlf
    HTTPSerializationHandle<HTTPGeneralHeader>, // general-header
    HTTPSerializationHandle<HTTPRequestHeader>, // request-header
    HTTPSerializationHandle<HTTPEntityHeader>,  // entity-header
    HTTPSerializationHandle<const char*>,       // crlf
    HTTPSerializationHandle<std::string_view>>  // body
    child_handle_;
};

#endif // RB_HTTP_SERIALIZATION_HANDLE_IPP
