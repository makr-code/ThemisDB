/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            storage_interface.h                                ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     195                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <optional>
#include "utils/expected.h"

namespace themis {

// Forward declaration to avoid duplicate interface definitions
class IIndexManager;
/// Shared pointer type for IIndexManager
using IIndexManagerPtr = std::shared_ptr<IIndexManager>;

/**
 * @brief Interface for storage engine operations
 * 
 * Provides abstract interface for core storage operations
 * without depending on concrete storage implementations (RocksDB, etc.).
 */
class IStorageEngine {
public:
    virtual ~IStorageEngine() = default;
    
    /**
     * @brief Open/initialize the storage engine
     * 
     * @param db_path Path to database directory
     * @return Result<void> - success or error with details
     */
    virtual Result<void> open(const std::string& db_path) = 0;
    
    /**
     * @brief Close the storage engine
     */
    virtual void close() = 0;
    
    /**
     * @brief Put a key-value pair
     * 
     * @param key The key
     * @param value The value
     * @return Result<void> - success or error with details
     */
    virtual Result<void> put(const std::string& key, const std::string& value) = 0;
    
    /**
     * @brief Get a value by key
     * 
     * @param key The key
     * @return Result<std::string> - The value if found, or error with details
     */
    virtual Result<std::string> get(const std::string& key) = 0;
    
    /**
     * @brief Delete a key-value pair
     * 
     * @param key The key to delete
     * @return Result<void> - success or error with details
     */
    virtual Result<void> del(const std::string& key) = 0;

    /**
     * @brief Scan a key range [start_key, end_key).
     *
     * Iterates all keys in sorted order within the range and calls
     * @p callback for each key-value pair.  Iteration stops when
     * @p callback returns false or the end of the range is reached.
     *
     * The default implementation returns ERR_STORAGE_NOT_IMPLEMENTED.
     * Concrete engines that support range scans should override this.
     *
     * @param start_key  Inclusive lower bound (empty = beginning of keyspace).
     * @param end_key    Exclusive upper bound (empty = end of keyspace).
     * @param callback   Called for each key-value pair; return false to stop.
     * @return Result<void> – ok on success, error on failure.
     */
    virtual Result<void> scanRange(
        [[maybe_unused]] std::string_view start_key,
        [[maybe_unused]] std::string_view end_key,
        std::function<bool(std::string_view key, [[maybe_unused]] std::string_view value)> callback)
    {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "scanRange not implemented");
    }

    /**
     * @brief Scan all keys with a given prefix.
     *
     * Iterates all keys whose byte representation starts with @p prefix
     * and calls @p callback for each.  Iteration stops when @p callback
     * returns false or no more matching keys exist.
     *
     * @param prefix    Key prefix to match.
     * @param callback  Called for each key-value pair; return false to stop.
     * @return Result<void> – ok on success, error on failure.
     */
    virtual Result<void> scanPrefix(
        [[maybe_unused]] std::string_view prefix,
        std::function<bool(std::string_view key, [[maybe_unused]] std::string_view value)> callback)
    {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "scanPrefix not implemented");
    }

    /**
     * @brief A single key range for use with scanMultiRange().
     */
    struct ScanRange {
        std::string start_key;  ///< Inclusive lower bound (empty = beginning).
        std::string end_key;    ///< Exclusive upper bound (empty = end).
    };

    /**
     * @brief Scan multiple key ranges in a single call.
     *
     * Iterates all provided @p ranges in order, calling @p callback for
     * every key-value pair encountered.  Ranges are processed sequentially;
     * overlapping ranges may deliver duplicate entries.
     *
     * Returns false from @p callback to stop iteration over the current range
     * (and all subsequent ranges).
     *
     * The default implementation delegates each range to scanRange().
     *
     * @param ranges    List of {start_key, end_key} pairs to scan.
     * @param callback  Called for each key-value pair; return false to stop.
     * @return Result<void> – ok on success, error on first failure.
     */
    virtual Result<void> scanMultiRange(
        const std::vector<ScanRange>& ranges,
        std::function<bool(std::string_view key, std::string_view value)> callback)
    {
        for (const auto& r : ranges) {
            bool stop = false;
            auto res = scanRange(r.start_key, r.end_key,
                [&](std::string_view k, std::string_view v) -> bool {
                    if (!callback(k, v)) { stop = true; return false; }
                    return true;
                });
            if (!res.has_value()) return res;
            if (stop) break;
        }
        return OkVoid();
    }
};

/// Shared pointer type for IStorageEngine
using IStorageEnginePtr = std::shared_ptr<IStorageEngine>;

/**
 * @brief Factory interface for storage engines
 */
class IStorageEngineFactory {
public:
    virtual ~IStorageEngineFactory() = default;
    
    /**
     * @brief Create a storage engine instance
     * 
     * @param db_path Path to database directory
     * @return Shared pointer to storage engine
     */
    virtual IStorageEnginePtr create(const std::string& db_path) = 0;
};

} // namespace themis
