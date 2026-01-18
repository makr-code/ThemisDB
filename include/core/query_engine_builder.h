/// @file query_engine_builder.h
/// @brief Builder pattern for QueryEngine construction with dependency injection
/// 
/// This builder enables flexible construction of QueryEngine instances with
/// various dependency configurations. It supports:
/// - Fluent API for setting dependencies
/// - Standard factory method for default configuration
/// - Validation of required dependencies before build
/// 
/// Example usage:
/// ```cpp
/// auto engine = QueryEngineBuilder::standard()
///     .withStorage(my_storage)
///     .withIndexManager(my_index_mgr)
///     .build();
/// ```

#pragma once

#include "query/query_engine.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace themis {

/// @brief Builder for constructing QueryEngine instances with dependency injection
/// 
/// This class implements the Builder pattern to simplify QueryEngine construction
/// with various dependency configurations. It validates that required dependencies
/// are provided before creating the QueryEngine.
class QueryEngineBuilder {
public:
    /// @brief Default constructor - creates empty builder
    QueryEngineBuilder() = default;
    
    /// @brief Set storage engine dependency
    /// @param storage Storage engine instance (can be nullptr for late binding)
    /// @return Reference to this builder for method chaining
    QueryEngineBuilder& withStorage(IStorageEnginePtr storage) {
        storage_ = storage;
        return *this;
    }
    
    /// @brief Set index manager dependency
    /// @param index_manager Index manager instance (required)
    /// @return Reference to this builder for method chaining
    QueryEngineBuilder& withIndexManager(IIndexManagerPtr index_manager) {
        index_manager_ = index_manager;
        return *this;
    }
    
    /// @brief Build the QueryEngine with configured dependencies
    /// 
    /// @throws std::runtime_error if required dependencies are not set
    /// @return Shared pointer to constructed QueryEngine
    std::shared_ptr<QueryEngine> build() {
        if (!index_manager_) {
            throw std::runtime_error("QueryEngineBuilder: IndexManager is required");
        }
        
        // Note: storage_ can be nullptr for late binding via setStorage()
        return std::make_shared<QueryEngine>(storage_, index_manager_);
    }
    
    /// @brief Create a builder pre-configured with standard dependencies
    /// 
    /// Creates a builder with default RocksDBWrapper and SecondaryIndexManager
    /// implementations. This is a convenience method for standard configurations.
    /// 
    /// @note This method is currently a placeholder - actual implementation
    ///       would create default concrete implementations
    /// 
    /// @return Builder configured with standard dependencies
    static QueryEngineBuilder standard() {
        QueryEngineBuilder builder;
        // TODO: Create default implementations when available
        // For now, return empty builder - caller must provide dependencies
        // Future: builder.withStorage(std::make_shared<StorageEngine>());
        // Future: builder.withIndexManager(std::make_shared<IndexManager>());
        return builder;
    }
    
private:
    IStorageEnginePtr storage_;
    IIndexManagerPtr index_manager_;
};

} // namespace themis
