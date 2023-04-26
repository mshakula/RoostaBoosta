/// \file Error.hpp
/// \date 2023-04-26
/// \author mshakula (matvey@gatech.edu)
///
/// \brief A better error handling interface for the mbed mechanism.

#ifndef RB_ERROR_HPP
#define RB_ERROR_HPP

#ifndef __cplusplus
#error "Error.hpp is a C++ header file"
#endif // __cplusplus

#include <mbed_error.h>

// ======================= Public Interface ==========================

namespace rb {

/// \brief A wrapper for the individual parameters needed for mbed error.
#define RB_ERROR(x) MBED_ERROR1(x.status, x.message, x.value)

/// \brief A wrapper for the individual parameters needed for mbed warning.
#define RB_WARN(x) MBED_WARNING1(x.status, x.message, x.value)

/// \brief A wrapper for the individual parameters needed for mbed error.
/// reporting.
struct ErrorStatus
{
  mbed_error_status_t status;
  const char*         message;
  int                 value;

  constexpr ErrorStatus(
    mbed_error_code_t  code_    = static_cast<mbed_error_code_t>(MBED_SUCCESS),
    const char*        message_ = NULL,
    unsigned int       value_   = 0,
    mbed_module_type_t module_  = MBED_MODULE_APPLICATION,
    mbed_error_type_t  type_    = MBED_ERROR_TYPE_CUSTOM) :
      status{MAKE_MBED_ERROR(type_, module_, code_)},
      message{message_},
      value{value_}
  {
  }

  constexpr ErrorStatus(
    mbed_error_status_t status_,
    const char*         message_ = NULL,
    unsigned int        value_   = 0) :
      status{status_},
      message{message_},
      value{value_}
  {
  }

  constexpr ErrorStatus(const ErrorStatus&)            = default;
  constexpr ErrorStatus& operator=(const ErrorStatus&) = default;

  constexpr      operator bool() const { return status < 0; }
  constexpr bool operator==(const ErrorStatus& other) const
  {
    return status == other.status;
  }

  /// \brief Returns the error type.
  ///
  /// eg:
  /// - MBED_ERROR_TYPE_SYSTEM
  /// - MBED_ERROR_TYPE_POSIX
  /// - MBED_ERROR_TYPE_CUSTOM
  constexpr mbed_error_type_t type() const
  {
    return static_cast<mbed_error_type_t>(MBED_GET_ERROR_TYPE(status));
  }

  /// \brief Returns the error module.
  ///
  /// eg:
  /// - MBED_MODULE_APPLICATION
  /// - MBED_MODULE_UNKNOWN
  /// - MBED_MODULE_PLATFORM
  constexpr mbed_module_type_t module() const
  {
    return static_cast<mbed_module_type_t>(MBED_GET_ERROR_MODULE(status));
  }

  /// \brief Returns the error code.
  constexpr mbed_error_code_t code() const
  {
    return static_cast<mbed_error_code_t>(MBED_GET_ERROR_CODE(status));
  }
};

} // namespace rb

// ===================== Detail Implementation =======================

#endif // RB_ERROR_HPP
