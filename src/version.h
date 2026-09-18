/**
 * @file version.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Version Information
 * ============================
 * Auto-generated version defines for runtime version checks.
 * This file provides version macros based on the VERSION file.
 */

#ifndef THEMIS_VERSION_H
#define THEMIS_VERSION_H

// Version string from VERSION file (e.g., "1.4.0" or "1.4.0-alpha")
#ifndef THEMIS_VERSION_STRING
#define THEMIS_VERSION_STRING "1.4.1-dev"
#endif

// Edition string (e.g., "COMMUNITY", "ENTERPRISE", "HYPERSCALER")
#ifndef THEMIS_EDITION_STRING
#define THEMIS_EDITION_STRING "COMMUNITY"
#endif

// Numeric version components
#ifndef THEMIS_VERSION_MAJOR
#define THEMIS_VERSION_MAJOR 1
#endif

#ifndef THEMIS_VERSION_MINOR
#define THEMIS_VERSION_MINOR 4
#endif

#ifndef THEMIS_VERSION_PATCH
#define THEMIS_VERSION_PATCH 1
#endif

// Combined version number (e.g., 10400 for version 1.4.0)
#define THEMIS_VERSION_NUMBER (THEMIS_VERSION_MAJOR * 10000 + THEMIS_VERSION_MINOR * 100 + THEMIS_VERSION_PATCH)

// Version check macros for compile-time checks
#define THEMIS_VERSION_AT_LEAST(major, minor, patch) \
    (THEMIS_VERSION_NUMBER >= ((major) * 10000 + (minor) * 100 + (patch)))

#define THEMIS_VERSION_LESS_THAN(major, minor, patch) \
    (THEMIS_VERSION_NUMBER < ((major) * 10000 + (minor) * 100 + (patch)))

namespace themis {
namespace version {

/**
 * @brief Get Version String.
 * @return Pointer to the result.
 * @details Implements getVersionString without additional internal calls.
 */
inline const char* getVersionString() {
    return THEMIS_VERSION_STRING;
}

/**
 * @brief Get Major Version.
 * @return Return value.
 * @details Implements getMajorVersion without additional internal calls.
 */
inline int getMajorVersion() {
    return THEMIS_VERSION_MAJOR;
}

/**
 * @brief Get Minor Version.
 * @return Return value.
 * @details Implements getMinorVersion without additional internal calls.
 */
inline int getMinorVersion() {
    return THEMIS_VERSION_MINOR;
}

/**
 * @brief Get Patch Version.
 * @return Return value.
 * @details Implements getPatchVersion without additional internal calls.
 */
inline int getPatchVersion() {
    return THEMIS_VERSION_PATCH;
}

/**
 * @brief Get Version Number.
 * @return Return value.
 * @details Implements getVersionNumber without additional internal calls.
 */
inline int getVersionNumber() {
    return THEMIS_VERSION_NUMBER;
}

/**
 * @brief Get Edition String.
 * @return Pointer to the result.
 * @details Implements getEditionString without additional internal calls.
 */
inline const char* getEditionString() {
    return THEMIS_EDITION_STRING;
}

} // namespace version
} // namespace themis

#endif // THEMIS_VERSION_H
