/// @file storage_interface.h
/// @brief Abstract interface for storage operations
/// 
/// This interface defines the contract for storage backends in ThemisDB.
/// It enables dependency inversion by allowing components to depend on
/// abstractions rather than concrete implementations.
/// 
/// Design Goals:
/// - Break circular dependencies between Storage ↔ Query ↔ Index
/// - Enable isolated unit testing with mock implementations
/// - Support alternative storage backends
/// - Maintain clear separation of concerns
/// 
/// @note This is a Phase 1 interface definition. Implementations will be
///       refactored in subsequent phases to use this interface.

#pragma once

#include "themis/base/export.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {

/// @brief Abstract interface for key-value storage operations
/// 
/// Defines the fundamental storage primitives for ThemisDB:
/// - Single key operations (Put, Get, Delete)
/// - Batch operations (WriteBatch)
/// - Range scans and iterations
/// - Transaction support
/// 
/// This interface intentionally has NO dependencies on:
/// - Query engine (no query evaluation)
/// - Index manager (no index operations)
/// - Security layer (encryption is orthogonal)
/// 
/// @note All methods must be thread-safe unless explicitly documented otherwise
class THEMIS_BASE_API IStorageEngine {
public:
    virtual ~IStorageEngine() = default;

    // ===== Basic Operations =====

    /// @brief Store a key-value pair
    /// @param key The key to store
    /// @param value The value to store
    /// @return true on success, false on failure
    virtual bool put(std::string_view key, std::string_view value) = 0;

    /// @brief Retrieve a value by key
    /// @param key The key to retrieve
    /// @return The value if found, std::nullopt otherwise
    virtual std::optional<std::string> get(std::string_view key) const = 0;

    /// @brief Delete a key-value pair
    /// @param key The key to delete
    /// @return true if the key existed and was deleted, false otherwise
    virtual bool del(std::string_view key) = 0;

    /// @brief Check if a key exists
    /// @param key The key to check
    /// @return true if the key exists, false otherwise
    virtual bool exists(std::string_view key) const = 0;

    // ===== Batch Operations =====

    /// @brief Execute multiple operations atomically
    /// 
    /// Operations are applied in order. If any operation fails,
    /// all operations are rolled back (atomicity guarantee).
    /// 
    /// @param operations Vector of operations to execute
    /// @return true on success, false on failure
    virtual bool executeBatch(
        const std::vector<std::pair<std::string, std::string>>& puts,
        const std::vector<std::string>& deletes) = 0;

    // ===== Range Queries =====

    /// @brief Callback function for range scans
    /// @param key The current key
    /// @param value The current value
    /// @return true to continue iteration, false to stop
    using ScanCallback = std::function<bool(std::string_view key, std::string_view value)>;

    /// @brief Scan all keys with a given prefix
    /// @param prefix The prefix to match
    /// @param callback Function called for each matching key-value pair
    virtual void scanPrefix(std::string_view prefix, ScanCallback callback) const = 0;

    /// @brief Scan keys in a range [start_key, end_key)
    /// @param start_key The start of the range (inclusive)
    /// @param end_key The end of the range (exclusive)
    /// @param callback Function called for each key-value pair in range
    virtual void scanRange(std::string_view start_key, std::string_view end_key, 
                          ScanCallback callback) const = 0;

    // ===== Transaction Support =====

    /// @brief Transaction handle for multi-operation atomicity
    class ITransaction {
    public:
        virtual ~ITransaction() = default;

        /// @brief Put a key-value pair in this transaction
        virtual bool put(std::string_view key, std::string_view value) = 0;

        /// @brief Get a value in the context of this transaction
        virtual std::optional<std::string> get(std::string_view key) const = 0;

        /// @brief Delete a key in this transaction
        virtual bool del(std::string_view key) = 0;

        /// @brief Commit the transaction
        /// @return true on success, false on conflict or failure
        virtual bool commit() = 0;

        /// @brief Rollback the transaction
        virtual void rollback() = 0;
    };

    /// @brief Begin a new transaction
    /// @return Transaction handle, or nullptr on failure
    virtual std::unique_ptr<ITransaction> beginTransaction() = 0;

    // ===== Maintenance Operations =====

    /// @brief Flush in-memory data to persistent storage
    virtual void flush() = 0;

    /// @brief Compact storage to reclaim space
    /// @param start_key Start of range to compact (optional)
    /// @param end_key End of range to compact (optional)
    virtual void compact(std::optional<std::string_view> start_key = std::nullopt,
                        std::optional<std::string_view> end_key = std::nullopt) = 0;

    /// @brief Get approximate storage size in bytes
    /// @return Size in bytes
    virtual uint64_t getApproximateSize() const = 0;

    /// @brief Get storage engine statistics
    /// @return Statistics as a JSON string
    virtual std::string getStatistics() const = 0;
};

/// @brief Factory interface for creating storage engines
/// 
/// Enables dependency injection of storage implementations
class THEMIS_BASE_API IStorageEngineFactory {
public:
    virtual ~IStorageEngineFactory() = default;

    /// @brief Create a new storage engine instance
    /// @param config Configuration string (implementation-specific)
    /// @return Storage engine instance, or nullptr on failure
    virtual std::unique_ptr<IStorageEngine> createStorageEngine(
        const std::string& config) = 0;
};

} // namespace themis
