/**
 * @file storage_move_semantics.h
 * @brief Storage module classes with explicit move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <mutex>

namespace themis {
namespace storage {

/// Forward declarations
namespace rocksdb_internal {
    class DB;
    struct Options;
}

/**
 * @brief Index cursor with explicit move semantics
 * 
 * Manages iteration over index entries with proper ownership semantics.
 */
class IndexCursor {
private:
    void* cursor_handle_ = nullptr;
    std::string index_name_;
    bool is_valid_ = false;
    size_t position_ = 0;

public:
    IndexCursor() = default;

    /**
     * @brief Move constructor - transfers cursor handle
     */
    IndexCursor(IndexCursor&& other) noexcept
        : cursor_handle_(other.cursor_handle_),
          index_name_(std::move(other.index_name_)),
          is_valid_(other.is_valid_),
          position_(other.position_) {
        other.cursor_handle_ = nullptr;
        other.is_valid_ = false;
        other.position_ = 0;
    }

    /**
     * @brief Move assignment operator - transfers cursor handle
     */
    IndexCursor& operator=(IndexCursor&& other) noexcept {
        if (this != &other) {
            // Clean up existing handle
            if (cursor_handle_ != nullptr) {
                // TODO: Call appropriate cleanup function
            }
            cursor_handle_ = other.cursor_handle_;
            index_name_ = std::move(other.index_name_);
            is_valid_ = other.is_valid_;
            position_ = other.position_;
            other.cursor_handle_ = nullptr;
            other.is_valid_ = false;
            other.position_ = 0;
        }
        return *this;
    }

    IndexCursor(const IndexCursor&) = delete;
    IndexCursor& operator=(const IndexCursor&) = delete;

    ~IndexCursor() {
        if (cursor_handle_ != nullptr) {
            // Cleanup handled by move semantics
        }
    }

    bool next() noexcept;
    bool isValid() const noexcept { return is_valid_; }
    size_t getPosition() const noexcept { return position_; }
};

/**
 * @brief Index builder with explicit move semantics
 */
class IndexBuilder {
private:
    std::string index_name_;
    std::vector<std::string> key_columns_;
    std::vector<std::string> value_columns_;
    void* builder_handle_ = nullptr;
    size_t entry_count_ = 0;

public:
    IndexBuilder() = default;

    /**
     * @brief Move constructor - transfers builder state
     */
    IndexBuilder(IndexBuilder&& other) noexcept
        : index_name_(std::move(other.index_name_)),
          key_columns_(std::move(other.key_columns_)),
          value_columns_(std::move(other.value_columns_)),
          builder_handle_(other.builder_handle_),
          entry_count_(other.entry_count_) {
        other.builder_handle_ = nullptr;
        other.entry_count_ = 0;
    }

    /**
     * @brief Move assignment operator
     */
    IndexBuilder& operator=(IndexBuilder&& other) noexcept {
        if (this != &other) {
            if (builder_handle_ != nullptr) {
                // TODO: Call appropriate cleanup
            }
            index_name_ = std::move(other.index_name_);
            key_columns_ = std::move(other.key_columns_);
            value_columns_ = std::move(other.value_columns_);
            builder_handle_ = other.builder_handle_;
            entry_count_ = other.entry_count_;
            other.builder_handle_ = nullptr;
            other.entry_count_ = 0;
        }
        return *this;
    }

    IndexBuilder(const IndexBuilder&) = delete;
    IndexBuilder& operator=(const IndexBuilder&) = delete;

    ~IndexBuilder() = default;

    void addEntry(const std::string& key, const std::string& value);
    IndexCursor build();
    size_t getEntryCount() const noexcept { return entry_count_; }
};

/**
 * @brief Column store with explicit move semantics
 */
class ColumnStore {
private:
    std::string store_name_;
    std::vector<std::string> column_names_;
    void* store_handle_ = nullptr;

public:
    ColumnStore() = default;

    /**
     * @brief Move constructor - transfers store handle
     */
    ColumnStore(ColumnStore&& other) noexcept
        : store_name_(std::move(other.store_name_)),
          column_names_(std::move(other.column_names_)),
          store_handle_(other.store_handle_) {
        other.store_handle_ = nullptr;
    }

    /**
     * @brief Move assignment operator
     */
    ColumnStore& operator=(ColumnStore&& other) noexcept {
        if (this != &other) {
            if (store_handle_ != nullptr) {
                // TODO: Call appropriate cleanup
            }
            store_name_ = std::move(other.store_name_);
            column_names_ = std::move(other.column_names_);
            store_handle_ = other.store_handle_;
            other.store_handle_ = nullptr;
        }
        return *this;
    }

    ColumnStore(const ColumnStore&) = delete;
    ColumnStore& operator=(const ColumnStore&) = delete;

    ~ColumnStore() = default;

    void addColumn(const std::string& name);
    size_t getColumnCount() const noexcept { return column_names_.size(); }
};

/**
 * @brief RocksDB wrapper handle with explicit move semantics
 * 
 * Manages low-level database handle with proper resource cleanup.
 * NOT suitable for concurrent move operations.
 */
class RocksDBHandleWrapper {
private:
    rocksdb_internal::DB* db_ = nullptr;
    std::string db_path_;
    std::unique_ptr<rocksdb_internal::Options> options_;
    bool is_open_ = false;

public:
    RocksDBHandleWrapper() = default;

    /**
     * @brief Move constructor - transfers database handle
     * 
     * @param[in,out] other Source handle (will be empty after move)
     * 
     * @post this->db_ = old other.db_
     * @post other.db_ = nullptr
     * @post other.is_open_ = false
     * 
     * Exception safety: noexcept
     * 
     * Thread-safety: NOT thread-safe for concurrent access
     */
    RocksDBHandleWrapper(RocksDBHandleWrapper&& other) noexcept
        : db_(other.db_),
          db_path_(std::move(other.db_path_)),
          options_(std::move(other.options_)),
          is_open_(other.is_open_) {
        other.db_ = nullptr;
        other.is_open_ = false;
    }

    /**
     * @brief Move assignment operator - transfers database handle
     * 
     * Closes existing database before taking ownership of new handle.
     * 
     * @param[in,out] other Source handle (will be empty after move)
     * @return Reference to this
     * 
     * @post this->db_ = old other.db_
     * @post other.db_ = nullptr
     * 
     * Exception safety: noexcept
     * 
     * Thread-safety: NOT thread-safe for concurrent access
     */
    RocksDBHandleWrapper& operator=(RocksDBHandleWrapper&& other) noexcept {
        if (this != &other) {
            if (db_ != nullptr && is_open_) {
                close();
            }
            db_ = other.db_;
            db_path_ = std::move(other.db_path_);
            options_ = std::move(other.options_);
            is_open_ = other.is_open_;
            other.db_ = nullptr;
            other.is_open_ = false;
        }
        return *this;
    }

    RocksDBHandleWrapper(const RocksDBHandleWrapper&) = delete;
    RocksDBHandleWrapper& operator=(const RocksDBHandleWrapper&) = delete;

    ~RocksDBHandleWrapper() {
        if (db_ != nullptr && is_open_) {
            close();
        }
    }

    /// Close the database
    void close() noexcept;

    /// Check if database is open
    bool isOpen() const noexcept { return is_open_; }

    /// Get database path
    const std::string& getPath() const { return db_path_; }
};

}  // namespace storage
}  // namespace themis
