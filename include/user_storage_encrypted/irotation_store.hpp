/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            irotation_store.hpp                                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 18:44:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     129                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8e5567bf5e  2026-03-24  feat(user_storage_encrypted): v0.1.0 stdin key delivery, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "encryption_backend_interface.hpp"
#include "security_level.hpp"
#include <cstdint>
#include <string>
#include <fstream>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Persistence interface for KeyRotationScheduler state.
 *
 * Allows last_check_ms timestamps to survive process restarts.
 */
class IRotationStore {
public:
    virtual ~IRotationStore() = default;

    /// Persist the last-check timestamp for @p level.
    virtual Result<void> save(SecurityLevel level, int64_t last_check_ms) = 0;

    /// Load the last-check timestamp for @p level (returns 0 if not found).
    virtual Result<int64_t> load(SecurityLevel level) = 0;
};

/**
 * @brief No-op IRotationStore — useful for testing and ephemeral use.
 */
class NullRotationStore : public IRotationStore {
public:
    Result<void> save(SecurityLevel /*level*/, int64_t /*ts*/) override {
        return Result<void>();
    }
    Result<int64_t> load(SecurityLevel /*level*/) override {
        return Result<int64_t>(int64_t{0});
    }
};

/**
 * @brief File-backed IRotationStore using a JSON file for persistence.
 *
 * Thread-safe via an internal mutex.  File format:
 * @code{.json}
 * { "0": 1711234567890, "1": 1711234567891, ... }
 * @endcode
 * Keys are the numeric value of SecurityLevel cast to int.
 */
class FileRotationStore : public IRotationStore {
public:
    explicit FileRotationStore(std::string path) : path_(std::move(path)) {}

    Result<void> save(SecurityLevel level, int64_t last_check_ms) override {
        std::lock_guard<std::mutex> lk(mtx_);

        nlohmann::json j = load_json();
        j[std::to_string(static_cast<int>(level))] = last_check_ms;

        std::ofstream f(path_);
        if (!f) {
            return Result<void>::error("FileRotationStore: cannot open " + path_);
        }
        f << j.dump(2);
        if (!f) {
            return Result<void>::error("FileRotationStore: write failed " + path_);
        }
        return Result<void>();
    }

    Result<int64_t> load(SecurityLevel level) override {
        std::lock_guard<std::mutex> lk(mtx_);

        nlohmann::json j = load_json();
        std::string key = std::to_string(static_cast<int>(level));
        if (j.contains(key)) {
            return Result<int64_t>(j[key].get<int64_t>());
        }
        return Result<int64_t>(int64_t{0});
    }

private:
    nlohmann::json load_json() const {
        std::ifstream f(path_);
        if (!f) {
            return nlohmann::json::object();
        }
        try {
            nlohmann::json j;
            f >> j;
            return j;
        } catch (...) {
            return nlohmann::json::object();
        }
    }

    std::string path_;
    mutable std::mutex mtx_;
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
