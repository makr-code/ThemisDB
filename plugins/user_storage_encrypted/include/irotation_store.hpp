/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            irotation_store.hpp                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15 07:10:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     165                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d8ee6d7cfe  2026-04-15  fix(user_storage_encrypted): repair broken merge artifact... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

// IRotationStore (get/put interface), NullRotationStore, and FileRotationStore
// are all defined in key_rotation_scheduler.hpp through irotation_store.hpp.
// This header makes them available from the plugins include path.
#include "encryption_backend_interface.hpp"
#include "security_level.hpp"
#include <cstdint>
#include <string>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Minimal key-value persistence interface for rotation state.
 *
 * The core protocol is string-key / string-value (get/put).  The
 * SecurityLevel-based convenience methods (save/load) are implemented
 * in this base class using the standard key format:
 *   "user_storage:rotation_state:{level_name}"  →  JSON {"last_check_ms": N}
 */
class IRotationStore {
public:
    virtual ~IRotationStore() = default;

    /**
     * @brief Read a value by string key.
     * @param key  Storage key
     * @param out  Value (set only when true is returned)
     * @return     true if the key existed
     */
    virtual bool get(const std::string& key, std::string& out) const = 0;

    /**
     * @brief Write a key-value pair.
     * @return true on success
     */
    virtual bool put(const std::string& key, const std::string& value) = 0;

    // -----------------------------------------------------------------------
    // SecurityLevel-based convenience wrappers (built on get/put above)
    // -----------------------------------------------------------------------

    /// Persist the last-check timestamp for @p level.
    Result<void> save(SecurityLevel level, int64_t last_check_ms) {
        nlohmann::json j;
        j["last_check_ms"] = last_check_ms;
        const std::string key =
            "user_storage:rotation_state:" + securityLevelToString(level);
        if (!put(key, j.dump())) {
            return Result<void>::error("IRotationStore::save: put failed for key " + key);
        }
        return Result<void>();
    }

    /// Load the last-check timestamp for @p level (returns 0 if not found).
    Result<int64_t> load(SecurityLevel level) const {
        const std::string key =
            "user_storage:rotation_state:" + securityLevelToString(level);
        std::string json_value;
        if (!get(key, json_value)) {
            return Result<int64_t>(int64_t{0});
        }
        try {
            auto j = nlohmann::json::parse(json_value);
            if (j.contains("last_check_ms")) {
                return Result<int64_t>(j["last_check_ms"].get<int64_t>());
            }
        } catch (...) {}
        return Result<int64_t>(int64_t{0});
    }
};

/**
 * @brief No-op IRotationStore — useful for ephemeral use (no persistence).
 */
class NullRotationStore : public IRotationStore {
public:
    bool get(const std::string& /*key*/, std::string& /*out*/) const override {
        return false;
    }
    bool put(const std::string& /*key*/, const std::string& /*value*/) override {
        return true;
    }
};

/**
 * @brief File-backed IRotationStore using a flat JSON file for persistence.
 *
 * Thread-safe via an internal mutex.  File format:
 * @code{.json}
 * {
 *   "user_storage:rotation_state:vs-nfd": "{\"last_check_ms\":1711234567890}",
 *   ...
 * }
 * @endcode
 */
class FileRotationStore : public IRotationStore {
public:
    explicit FileRotationStore(std::string path) : path_(std::move(path)) {}

    bool get(const std::string& key, std::string& out) const override {
        std::lock_guard<std::mutex> lk(mtx_);
        const auto j = load_json();
        if (!j.is_object() || !j.contains(key)) {
            return false;
        }
        out = j[key].get<std::string>();
        return true;
    }

    bool put(const std::string& key, const std::string& value) override {
        std::lock_guard<std::mutex> lk(mtx_);
        auto j = load_json();
        if (!j.is_object()) {
            j = nlohmann::json::object();
        }
        j[key] = value;
        std::ofstream f(path_);
        if (!f) {
            return false;
        }
        f << j.dump(2);
        return f.good();
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
            return j.is_object() ? j : nlohmann::json::object();
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
