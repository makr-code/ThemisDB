/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            checksum_utils.h                                   ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:47:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>

namespace themis {
namespace utils {

/**
 * @brief Calculate SHA256 checksum of a file
 * @param file_path Path to file
 * @return Hex-encoded SHA256 checksum or empty string on error
 */
std::string calculateSHA256(const std::string& file_path);

/**
 * @brief Calculate MD5 checksum of a file
 * @param file_path Path to file
 * @return Hex-encoded MD5 checksum or empty string on error
 *
 * @deprecated MD5 is cryptographically broken (collision attacks, CWE-327).
 *   Use calculateSHA256() for all new code.  This function is retained only
 *   for backward-compatible verification of legacy checksums (read-only use).
 *   New code MUST NOT use MD5 for integrity or authentication purposes.
 */
[[deprecated("MD5 is cryptographically broken (CWE-327). Use calculateSHA256() instead.")]]
std::string calculateMD5(const std::string& file_path);

} // namespace utils
} // namespace themis
