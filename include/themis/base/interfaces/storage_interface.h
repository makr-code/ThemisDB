#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
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
        std::string_view start_key,
        std::string_view end_key,
        std::function<bool(std::string_view key, std::string_view value)> callback)
    {
        (void)start_key; (void)end_key; (void)callback;
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
        std::string_view prefix,
        std::function<bool(std::string_view key, std::string_view value)> callback)
    {
        (void)prefix; (void)callback;
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "scanPrefix not implemented");
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
