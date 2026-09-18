/**
 * @file checksum_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
/**
 * @brief TBD: Describe calculateMD5.
 * @param[in] file_path Input parameter.
 * @return Return value.
 */
std::string calculateMD5(const std::string& file_path);

} // namespace utils
} // namespace themis
