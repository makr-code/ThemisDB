/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            version.h                                          ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:53:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
 * Get the full version string (e.g., "1.4.0-alpha")
 * @return Version string from VERSION file
 */
inline const char* getVersionString() {
    return THEMIS_VERSION_STRING;
}

/**
 * Get major version number
 * @return Major version (e.g., 1 for version 1.4.0)
 */
inline int getMajorVersion() {
    return THEMIS_VERSION_MAJOR;
}

/**
 * Get minor version number
 * @return Minor version (e.g., 4 for version 1.4.0)
 */
inline int getMinorVersion() {
    return THEMIS_VERSION_MINOR;
}

/**
 * Get patch version number
 * @return Patch version (e.g., 0 for version 1.4.0)
 */
inline int getPatchVersion() {
    return THEMIS_VERSION_PATCH;
}

/**
 * Get numeric version number
 * @return Combined version (e.g., 10400 for version 1.4.0)
 */
inline int getVersionNumber() {
    return THEMIS_VERSION_NUMBER;
}

/**
 * Get edition string
 * @return Edition name (e.g., "COMMUNITY", "ENTERPRISE", "HYPERSCALER")
 */
inline const char* getEditionString() {
    return THEMIS_EDITION_STRING;
}

} // namespace version
} // namespace themis

#endif // THEMIS_VERSION_H
