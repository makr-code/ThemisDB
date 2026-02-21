/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            file_utils.h                                       ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
