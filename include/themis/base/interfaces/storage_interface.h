#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis {

/**
 * @brief Interface for index management operations
 * 
 * Provides abstract interface for managing database indexes
 * without depending on concrete index implementations.
 */
class IIndexManager {
public:
    virtual ~IIndexManager() = default;
    
    /**
     * @brief Create an index on specified fields
     * 
     * @param table_name Name of the table
     * @param field_name Name of the field to index
     * @param index_type Type of index (e.g., "btree", "hash", "vector")
     * @return true if index created successfully, false otherwise
     */
    virtual bool create_index(
        const std::string& table_name,
        const std::string& field_name,
        const std::string& index_type) = 0;
    
    /**
     * @brief Drop an existing index
     * 
     * @param table_name Name of the table
     * @param field_name Name of the indexed field
     * @return true if index dropped successfully, false otherwise
     */
    virtual bool drop_index(
        const std::string& table_name,
        const std::string& field_name) = 0;
    
    /**
     * @brief Check if an index exists
     * 
     * @param table_name Name of the table
     * @param field_name Name of the field
     * @return true if index exists, false otherwise
     */
    virtual bool has_index(
        const std::string& table_name,
        const std::string& field_name) const = 0;
};

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

} // namespace themis
