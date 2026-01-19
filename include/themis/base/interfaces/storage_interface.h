#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

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
     * @return true if opened successfully, false otherwise
     */
    virtual bool open(const std::string& db_path) = 0;
    
    /**
     * @brief Close the storage engine
     */
    virtual void close() = 0;
    
    /**
     * @brief Put a key-value pair
     * 
     * @param key The key
     * @param value The value
     * @return true if successful, false otherwise
     */
    virtual bool put(const std::string& key, const std::string& value) = 0;
    
    /**
     * @brief Get a value by key
     * 
     * @param key The key
     * @return The value if found, empty optional otherwise
     */
    virtual std::optional<std::string> get(const std::string& key) = 0;
    
    /**
     * @brief Delete a key-value pair
     * 
     * @param key The key to delete
     * @return true if successful, false otherwise
     */
    virtual bool del(const std::string& key) = 0;
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
