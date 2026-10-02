/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validator.cpp                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     39                                             ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file validator.cpp
 * @brief RocksDB-backed documentation database validation.
 *
 * Validates that the target path is a real RocksDB directory containing the
 * required manifest metadata before the builder accepts it as a valid database.
 */

#include "validator.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace themis {
namespace tools {

bool Validator::validate(const std::string& db_path) {
    if (db_path.empty()) {
        return false;
    }

    std::filesystem::path db_dir(db_path);
    std::error_code ec;

    if (!std::filesystem::exists(db_dir, ec) || !std::filesystem::is_directory(db_dir, ec)) {
        return false;
    }

    const std::filesystem::path current_path = db_dir / "CURRENT";
    if (!std::filesystem::exists(current_path, ec) || !std::filesystem::is_regular_file(current_path, ec)) {
        return false;
    }

    std::ifstream current_stream(current_path);
    if (!current_stream) {
        return false;
    }

    std::string manifest_name;
    std::getline(current_stream, manifest_name);
    if (manifest_name.empty()) {
        return false;
    }

    const std::filesystem::path manifest_path = db_dir / manifest_name;
    if (!std::filesystem::exists(manifest_path, ec) || !std::filesystem::is_regular_file(manifest_path, ec)) {
        return false;
    }

    std::ifstream manifest_stream(manifest_path);
    return manifest_stream.good();
}

} // namespace tools
} // namespace themis
