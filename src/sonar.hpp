/// \file sonar.hpp
/// \date 2023-04-28
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief Header for sonar control.

#ifndef SONAR_HPP
#define SONAR_HPP

#ifndef __cplusplus
#error "sonar.hpp is a cxx-only header."
#endif // __cplusplus

// ======================= Public Interface ==========================

/// \brief returns true if something within 10cm of sonar
///
bool
Is_Snoozed();

// ===================== Detail Implementation =======================

#endif // SONAR_HPP