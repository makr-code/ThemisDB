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
 */
std::string calculateMD5(const std::string& file_path);

} // namespace utils
} // namespace themis
