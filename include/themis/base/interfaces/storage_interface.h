/**
 * @file storage_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=2, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    [[nodiscard]] virtual Result<void> open(const std::string& db_path) = 0;
    
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
    [[nodiscard]] virtual Result<void> put(const std::string& key, const std::string& value) = 0;
    
    /**
     * @brief Get a value by key
     * 
     * @param key The key
     * @return Result<std::string> - The value if found, or error with details
     */
    [[nodiscard]] virtual Result<std::string> get(const std::string& key) = 0;
    
    /**
     * @brief Delete a key-value pair
     * 
     * @param key The key to delete
     * @return Result<void> - success or error with details
     */
    [[nodiscard]] virtual Result<void> del(const std::string& key) = 0;

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
    [[nodiscard]] virtual Result<void> scanRange(
        [[maybe_unused]] std::string_view start_key,
        [[maybe_unused]] std::string_view end_key,
        [[maybe_unused]] std::function<bool(std::string_view key, std::string_view value)> callback)
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
    [[nodiscard]] virtual Result<void> scanPrefix(
        [[maybe_unused]] std::string_view prefix,
        [[maybe_unused]] std::function<bool(std::string_view key, std::string_view value)> callback)
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
    [[nodiscard]] virtual Result<void> scanMultiRange(
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
            if (!res.has_value()) {
              return res;
            }
            if (stop) {
              break;
            }
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
    [[nodiscard]] virtual IStorageEnginePtr create(const std::string& db_path) = 0;
};

} // namespace themis
