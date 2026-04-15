/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            file_utils.h                                       ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     49                                             ║
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
 * @brief Read the entire contents of a file into a string
 * 
 * This function reads a file in binary mode and returns its contents as a string.
 * Commonly used for reading certificate files, configuration files, etc.
 * 
 * @param path Path to the file to read
 * @return String containing the file contents
 * @throws std::runtime_error if the file cannot be opened
 * 
 * @example
 * ```cpp
 * std::string cert = readFileContents("/path/to/certificate.pem");
 * std::string config = readFileContents("/etc/config.yaml");
 * ```
 */
std::string readFileContents(const std::string& path);

} // namespace utils
} // namespace themis
