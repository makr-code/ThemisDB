/**
 * @file query_engine_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/// @file query_engine_builder.h
/// @brief Builder pattern for QueryEngine construction with dependency injection
///
/// This builder enables flexible construction of QueryEngine instances with
/// various dependency configurations. It supports:
/// - Fluent API for setting dependencies
/// - Standard factory method for default configuration
/// - Validation of required dependencies before build
///
/// Failure behavior:
/// - build() throws std::runtime_error when required dependencies are missing.
/// - storage is optional at build time and may be bound later via QueryEngine
///   APIs; index manager is mandatory.
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
    
    /**
     * @brief @brief Set storage engine dependency @param storage Storage engine instance (can be nullptr for late binding).
     * @param[in] storage Input parameter.
     * @return Return value.
     * @details @return Reference to this builder for method chaining Implements withStorage without additional internal calls.
     */
    QueryEngineBuilder& withStorage(IStorageEnginePtr storage) {
        storage_ = storage;
        return *this;
    }
    
    /**
     * @brief @brief Set index manager dependency @param index_manager Index manager instance (required).
     * @param[in] index_manager Input parameter.
     * @return Return value.
     * @details @return Reference to this builder for method chaining Implements withIndexManager without additional internal calls.
     */
    QueryEngineBuilder& withIndexManager(IIndexManagerPtr index_manager) {
        index_manager_ = index_manager;
        return *this;
    }
    
    /**
     * @brief @brief Build the QueryEngine with configured dependencies.
     * @return Return value.
     * @throws std::runtime_error if an error occurs.
     * @details index_manager must be configured before calling build(). storage may be nullptr when late binding is intended. @throws std::runtime_error if required dependencies are not set @return Shared pointer to constructed QueryEngine Implements build without additional internal calls.
     */
    std::shared_ptr<query::QueryEngine> build() {
        if (!index_manager_) {
            throw std::runtime_error("QueryEngineBuilder: IndexManager is required");
        }
        
        // Note: storage_ can be nullptr for late binding via setStorage()
        return std::make_shared<query::QueryEngine>(storage_, index_manager_);
    }
    
    /**
     * @brief @brief Create a standard builder instance.
     * @return Return value.
     * @details Currently returns an empty builder equivalent to QueryEngineBuilder(). Callers must still configure index manager explicitly before build(). @return Empty builder to be configured with dependencies Implements standard without additional internal calls.
     */
    static QueryEngineBuilder standard() {
        QueryEngineBuilder builder = {};
        return builder;
    }
    
private:
    IStorageEnginePtr storage_;
    IIndexManagerPtr index_manager_;
};

} // namespace themis
