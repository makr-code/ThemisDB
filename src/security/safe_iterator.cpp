/**
 * @file safe_iterator.cpp
 * @brief Bounds-safe iterator implementation.
 *
 * Out-of-line definitions for SafeIterator diagnostics and
 * bounds-check error reporting.
 */

#include "security/safe_iterator.h"

// Try to include spdlog if available for better logging
#ifdef THEMIS_HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace themis::security {
namespace SafeIterator {

// Note: Most SafeIterator functionality is header-only (template-based)
// This file provides explicit instantiations and any runtime initialization if needed.

// InvalidationDetector is primarily managed through its constructor/destructor
// which are defined inline in the header.

// AdvanceSafe::advance and related functions are template-based
// and are implicitly instantiated when used with specific iterator types.

// RangeValidator is a class template and follows the same pattern.

// BoundsChecker functions are all static template methods,
// implicitly instantiated when called.

// All safety checks and logging are performed within the template methods.
// This provides zero-overhead abstraction for the common case where
// the iterator operations are valid.

}  // namespace SafeIterator
}  // namespace themis::security
