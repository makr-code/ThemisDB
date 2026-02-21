/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            file_utils.h                                       ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:58:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f234aa986  2026-01-22  Implement mTLS for secure shard communication (#785) ║
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
