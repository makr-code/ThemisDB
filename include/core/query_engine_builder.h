/*
 * ThemisDB | File: query_engine_builder.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 99
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    /// Returns an empty builder that can be configured with dependencies.
    /// In the current Phase 3 implementation, the builder must be explicitly
    /// configured with dependencies using withStorage() and withIndexManager().
    /// 
    /// @note Future phases will create actual default implementations. For now,
    ///       this is equivalent to QueryEngineBuilder() and exists for forward
    ///       compatibility.
    /// 
    /// @return Empty builder to be configured with dependencies
    static QueryEngineBuilder standard() {
        QueryEngineBuilder builder;
        // Phase 3: Return empty builder - caller must provide dependencies
        // Phase 4+: Will create default implementations automatically
        return builder;
    }
    
private:
    IStorageEnginePtr storage_;
    IIndexManagerPtr index_manager_;
};

} // namespace themis
