/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            checksum_utils.h                                   ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:55:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     23                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 43c682e67  2026-02-07  feat: Add LLM deployment plugin with Ollama integration, ... ║
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
 */
std::string calculateMD5(const std::string& file_path);

} // namespace utils
} // namespace themis
