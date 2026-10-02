/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validator.h                                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     44                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file validator.h
 * @brief Database validator interface for RocksDB structure checks.
 *
 * Validates the expected on-disk layout of a ThemisDB documentation database so
 * callers can reject non-existent or corrupted database directories before they
 * attempt to read or write content.
 */

#pragma once

#include <string>

namespace themis {
namespace tools {

class Validator {
public:
    /**
     * @brief Validate that a database path contains a readable RocksDB manifest.
     * @param db_path Path to the database directory to inspect.
     * @return true when the directory exists and contains a valid CURRENT manifest
     *         entry that resolves to a readable manifest file; false otherwise.
     * @note This is a structural integrity check only. It does not attempt to open
     *       the full RocksDB instance or verify every column family.
     */
    [[nodiscard]] static bool validate(const std::string& db_path);
};

} // namespace tools
} // namespace themis
