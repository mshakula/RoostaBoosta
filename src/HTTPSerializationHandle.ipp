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
      err_{err}
  {
  }
  HTTPSerializationHandleBase(const HTTPSerializationHandleBase& o) :
      obj_{o.obj_},
      err_{o.err_}
  {
  }
  HTTPSerializationHandleBase& operator=(const HTTPSerializationHandleBase& o)
  {
    std::destroy_at(this);
    ::new (this) HTTPSerializationHandleBase{o.obj_, o.err_};
    return *this;
  }

  ErrorStatus fail() const { return err_; }

  const T&    obj_;
  ErrorStatus err_;
};

/// \brief Specialization for std::string_view.
///
/// \see HTTPSerializationHandle
template<>
class HTTPSerializationHandle<std::string_view> :
    public HTTPSerializationHandleBase<std::string_view>
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
    std::size_t bytes_to_copy =
      std::min(static_cast<std::size_t>(buffer.size()), obj_.size() - idx_);
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

  operator bool() const { return !eof() && !fail(); }

 private:
  std::size_t idx_;
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
      static_cast<const int>(obj_));
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
    if (idx_ >= req_) {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA, "Serialization has already completed.", idx_);
      return *this;
    }

    auto to_write =
      std::min(static_cast<std::size_t>(buffer.size()), req_ - idx_);
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
  std::string_view get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Accept";
      case 1:
        return this->obj_.accept;
      case 2:
        return "Accept-Charset";
      case 3:
        return this->obj_.accept_charset;
      case 4:
        return "Accept-Encoding";
      case 5:
        return this->obj_.accept_encoding;
      case 6:
        return "Accept-Language";
      case 7:
        return this->obj_.accept_language;
      case 8:
        return "Authorization";
      case 9:
        return this->obj_.authorization;
      case 10:
        return "Expect";
      case 11:
        return this->obj_.expect;
      case 12:
        return "From";
      case 13:
        return this->obj_.from;
      case 14:
        return "Host";
      case 15:
        return this->obj_.host;
      case 16:
        return "If-Match";
      case 17:
        return this->obj_.if_match;
      case 18:
        return "If-Modified-Since";
      case 19:
        return this->obj_.if_modified_since;
      case 20:
        return "If-None-Match";
      case 21:
        return this->obj_.if_none_match;
      case 22:
        return "If-Range";
      case 23:
        return this->obj_.if_range;
      case 24:
        return "If-Unmodified-Since";
      case 25:
        return this->obj_.if_unmodified_since;
      case 26:
        return "Max-Forwards";
      case 27:
        return this->obj_.max_forwards;
      case 28:
        return "Proxy-Authorization";
      case 29:
        return this->obj_.proxy_authorization;
      case 30:
        return "Range";
      case 31:
        return this->obj_.range;
      case 32:
        return "Referer";
      case 33:
        return this->obj_.referer;
      case 34:
        return "TE";
      case 35:
        return this->obj_.te;
      case 36:
        return "User-Agent";
      case 37:
        return this->obj_.user_agent;
      default:
        return "";
    }
  }

  HTTPSerializationHandle(const HTTPRequestHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this, &buffer]() {
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
      while (child_idx_ <= 37) {
        if (f())
          return *this;
        child_handle_ =
          HTTPSerializationHandle<std::string_view>{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
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

template<>
class HTTPSerializationHandle<HTTPResponseHeader> :
    public HTTPSerializationHandleBase<HTTPResponseHeader>
{
 public:
  std::string_view get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Accept-Ranges";
      case 1:
        return this->obj_.accept_ranges;
      case 2:
        return "Age";
      case 3:
        return this->obj_.age;
      case 4:
        return "ETag";
      case 5:
        return this->obj_.etag;
      case 6:
        return "Location";
      case 7:
        return this->obj_.location;
      case 8:
        return "Proxy-Authenticate";
      case 9:
        return this->obj_.proxy_authenticate;
      case 10:
        return "Retry-After";
      case 11:
        return this->obj_.retry_after;
      case 12:
        return "Server";
      case 13:
        return this->obj_.server;
      case 14:
        return "Vary";
      case 15:
        return this->obj_.vary;
      case 16:
        return "WWW-Authenticate";
      case 17:
        return this->obj_.www_authenticate;
      default:
        return "";
    }
  }

  HTTPSerializationHandle(const HTTPResponseHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this, &buffer]() {
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
      while (child_idx_ <= 17) {
        if (f())
          return *this;
        child_handle_ =
          HTTPSerializationHandle<std::string_view>{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
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

template<>
class HTTPSerializationHandle<HTTPGeneralHeader> :
    public HTTPSerializationHandleBase<HTTPGeneralHeader>
{
 public:
  std::string_view get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Cache-Control";
      case 1:
        return this->obj_.cache_control;
      case 2:
        return "Connection";
      case 3:
        return this->obj_.connection;
      case 4:
        return "Date";
      case 5:
        return this->obj_.date;
      case 6:
        return "Pragma";
      case 7:
        return this->obj_.pragma;
      case 8:
        return "Trailer";
      case 9:
        return this->obj_.trailer;
      case 10:
        return "Transfer-Encoding";
      case 11:
        return this->obj_.transfer_encoding;
      case 12:
        return "Upgrade";
      case 13:
        return this->obj_.upgrade;
      case 14:
        return "Via";
      case 15:
        return this->obj_.via;
      case 16:
        return "Warning";
      case 17:
        return this->obj_.warning;
      default:
        return "";
    }
  }

  HTTPSerializationHandle(const HTTPGeneralHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this, &buffer]() {
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
      while (child_idx_ <= 17) {
        if (f())
          return *this;
        child_handle_ =
          HTTPSerializationHandle<std::string_view>{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
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

template<>
class HTTPSerializationHandle<HTTPEntityHeader> :
    public HTTPSerializationHandleBase<HTTPEntityHeader>
{
 public:
  std::string_view get_field(std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return "Allow";
      case 1:
        return this->obj_.allow;
      case 2:
        return "Content-Encoding";
      case 3:
        return this->obj_.content_encoding;
      case 4:
        return "Content-Language";
      case 5:
        return this->obj_.content_language;
      case 6:
        return "Content-Length";
      case 7:
        return this->obj_.content_length;
      case 8:
        return "Content-Location";
      case 9:
        return this->obj_.content_location;
      case 10:
        return "Content-MD5";
      case 11:
        return this->obj_.content_md5;
      case 12:
        return "Content-Range";
      case 13:
        return this->obj_.content_range;
      case 14:
        return "Content-Type";
      case 15:
        return this->obj_.content_type;
      case 16:
        return "Expires";
      case 17:
        return this->obj_.expires;
      case 18:
        return "Last-Modified";
      case 19:
        return this->obj_.last_modified;
      case 20:
        return this->obj_.extension_header;
      default:
        return "";
    }
  }

  HTTPSerializationHandle(const HTTPEntityHeader& obj) :
      HTTPSerializationHandleBase{obj},
      child_handle_{get_field(0)},
      child_idx_{0}
  {
  }
  HTTPSerializationHandle(const HTTPSerializationHandle&)            = default;
  HTTPSerializationHandle& operator=(const HTTPSerializationHandle&) = default;

  HTTPSerializationHandle& serialize(mbed::Span<char> buffer)
  {
    auto f = [this, &buffer]() {
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
      while (child_idx_ <= 20) {
        if (f())
          return *this;
        child_handle_ =
          HTTPSerializationHandle<std::string_view>{get_field(++child_idx_)};
      }
    } else {
      err_ = ErrorStatus(
        MBED_ERROR_CODE_ENODATA,
        "Serialization has already completed.",
        child_idx_);
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

#endif // RB_HTTP_SERIALIZATION_HANDLE_IPP
